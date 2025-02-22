#include <stddef.h>
#include "cpu.h"
#include "cli.h"
#include "disp.h"
#include "minio.h"
#include "assert.h"

#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_gpio.h"
#include "stm32f1xx_ll_i2c.h"
#include "stm32f1xx_ll_dma.h"

#define SSD1306_I2C_ADDR 0x3C

static struct
{
    volatile bool dma_transfer_complete;
    volatile bool dma_transfer_error;
    volatile bool screen_update;
    volatile bool page_cmd;
    volatile uint8_t page_nbr;
    uint8_t gfx_buffer[DISP_W * DISP_H / 8];
    disp_cb_t done_cb;
} me;

static void i2c_dma_init(void);

static void i2c_write_bytes_dma(uint8_t addr, const uint8_t *data, uint16_t size)
{
    ASSERT(me.dma_transfer_complete);
    me.dma_transfer_complete = false; // Reset flag

    // Ensure DMA is disabled before configuring
    LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_6);

    // Set memory address and data length
    LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_6, (uint32_t)data);
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_6, size);

    // Clear DMA flags
    LL_DMA_ClearFlag_TC6(DMA1);
    LL_DMA_ClearFlag_HT6(DMA1);
    LL_DMA_ClearFlag_TE6(DMA1);

    uint8_t tries = 5;
    while (--tries > 0)
    {
        // Generate Start Condition
        volatile int tmo = 0x1000;
        LL_I2C_GenerateStartCondition(I2C1);
        while (--tmo && !LL_I2C_IsActiveFlag_SB(I2C1))
            ;
        if (tmo > 0)
        {
            // Send device address (write mode)
            LL_I2C_TransmitData8(I2C1, addr << 1);
            while (--tmo && !LL_I2C_IsActiveFlag_ADDR(I2C1))
                ;
            LL_I2C_ClearFlag_ADDR(I2C1);
        }
        if (tmo == 0)
            continue;
        else
            break;
    }
    if (tries == 0)
    {
        LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_I2C1);
        LL_AHB1_GRP1_DisableClock(LL_AHB1_GRP1_PERIPH_DMA1);
        me.dma_transfer_error = true;
        me.dma_transfer_complete = true;
        printf("error dma i2c disp\n");
        return;
    }

    // Enable DMA for I2C TX
    LL_I2C_EnableDMAReq_TX(I2C1);
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_6);
    me.dma_transfer_error = false;
}

static void tx_next_page(void)
{
    static uint8_t data[5 + 128];
    if (me.page_cmd)
    {
        if (me.page_nbr >= 8)
        {
            me.screen_update = false;
            if (me.done_cb)
                me.done_cb(0);
            return;
        }
        data[0] = 0x00;               // command mode indictor
        data[1] = 0xb0 + me.page_nbr; // Set page address
        data[2] = 0x00;               // Set lower column address (0x00 to 0x0F)
        data[3] = 0x10;               // Set higher column address (0x10 to 0x1F for the full 128px)
        i2c_write_bytes_dma(SSD1306_I2C_ADDR, data, 4);
        me.page_cmd = false;
    }
    else
    {
        data[4] = 0x40;                                                    // Data command
        memcpy(&data[5], &me.gfx_buffer[me.page_nbr * 128], 128);          // Copy 128 bytes from the buffer for this page
        i2c_write_bytes_dma(SSD1306_I2C_ADDR, &data[4], sizeof(data) - 4); // Send the page data
        me.page_nbr++;
        me.page_cmd = true;
    }
    if (me.done_cb && me.dma_transfer_error)
        me.done_cb(-1);
}

void disp_init(void)
{
    i2c_dma_init();
}

void disp_set_enabled(bool enable, disp_cb_t done_cb)
{
    ASSERT(me.dma_transfer_complete);
    me.done_cb = done_cb;
    if (enable)
    {
        const uint8_t init_cmds[] = {
            0,          // command mode indictor
            0xAE,       // Display OFF
            0xD5, 0x80, // Set display clock divide ratio/oscillator frequency
            0xA8, DISP_H - 1, // Set multiplex ratio (1 to 64) -> 0x3F for 128x64
            0xD3, 0x00,       // Set display offset to 0
            0x40,             // Set start line to 0

            0x8D, 0x14, // Enable charge pump

            0x20, 0x00, // Memory mode: Page Addressing Mode **(IMPORTANT)**
            0xA1,       // Set segment remap: column 127 is mapped to SEG0
            0xC8,       // Set COM scan direction (remapped)
            0xDA, 0x12, // Set COM pins hardware configuration (0x12 for 128x64)
            0x81, 0xCF, // Set contrast (0x00 - 0xFF)
            0xD9, 0xf1, // Set pre-charge period
            0xDB, 0x40, // Set VCOMH deselect level
            0xA4,       // Entire display ON (resume to RAM content)
            0xA6,       // Set normal (not inverted) display mode
            0xAF        // Display ON
        };
        i2c_write_bytes_dma(SSD1306_I2C_ADDR, init_cmds, sizeof(init_cmds));
    }
    else
    {
        const uint8_t cmds[] = {
            0x00,       // command mode indictor
            0x8d, 0x10, // Disable charge pump
            0xae,       // Display OFF
        };
        i2c_write_bytes_dma(SSD1306_I2C_ADDR, cmds, sizeof(cmds));
    }

    cpu_halt(100);
}

bool disp_screen_update(disp_cb_t done_cb)
{
    if (me.screen_update || !me.dma_transfer_complete)
        return false;
    me.screen_update = true;
    me.page_cmd = true;
    me.page_nbr = 0;
    me.done_cb = done_cb;

    tx_next_page();
    return true;
}

uint8_t *disp_gbuf(void)
{
    return me.gfx_buffer;
}

static void i2c_dma_init(void)
{

    // Enable clocks for I2C, GPIO, and DMA
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOB);
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);

    // Configure PB6 (SCL) and PB7 (SDA) as Open-Drain
    LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_6, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetPinOutputType(GPIOB, LL_GPIO_PIN_6, LL_GPIO_OUTPUT_OPENDRAIN);
    LL_GPIO_SetPinSpeed(GPIOB, LL_GPIO_PIN_6, LL_GPIO_SPEED_FREQ_HIGH);

    LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_7, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetPinOutputType(GPIOB, LL_GPIO_PIN_7, LL_GPIO_OUTPUT_OPENDRAIN);
    LL_GPIO_SetPinSpeed(GPIOB, LL_GPIO_PIN_7, LL_GPIO_SPEED_FREQ_HIGH);

    // Configure I2C1
    LL_I2C_Disable(I2C1);
    LL_I2C_SetClockSpeedMode(I2C1, LL_I2C_CLOCK_SPEED_FAST_MODE);
    LL_I2C_SetPeriphClock(I2C1, 8000000);
    LL_I2C_ConfigSpeed(I2C1, 8000000, 400000, LL_I2C_DUTYCYCLE_2);
    LL_I2C_Enable(I2C1);

    // Configure DMA for I2C TX (Channel 6)
    LL_DMA_ConfigTransfer(DMA1, LL_DMA_CHANNEL_6,
                          LL_DMA_DIRECTION_MEMORY_TO_PERIPH |
                              LL_DMA_MODE_NORMAL |
                              LL_DMA_PERIPH_NOINCREMENT |
                              LL_DMA_MEMORY_INCREMENT |
                              LL_DMA_PDATAALIGN_BYTE |
                              LL_DMA_MDATAALIGN_BYTE |
                              LL_DMA_PRIORITY_HIGH);

    // Set DMA addresses
    LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_6, (uint32_t)&I2C1->DR);

    // Enable DMA transfer complete interrupt
    LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_6);

    // Enable DMA1 Channel 6 interrupt in NVIC
    NVIC_SetPriority(DMA1_Channel6_IRQn, 1);
    NVIC_EnableIRQ(DMA1_Channel6_IRQn);

    me.dma_transfer_complete = true;
}

void DMA1_Channel6_IRQHandler(void);
void DMA1_Channel6_IRQHandler(void)
{
    if (LL_DMA_IsActiveFlag_TC6(DMA1))
    {
        // Clear transfer complete flag
        LL_DMA_ClearFlag_TC6(DMA1);

        // Disable DMA channel
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_6);

        // Wait for last byte transfer
        while (!LL_I2C_IsActiveFlag_BTF(I2C1))
            ;

        // Generate STOP condition
        LL_I2C_GenerateStopCondition(I2C1);

        // Set completion flag
        me.dma_transfer_complete = true;

        if (me.screen_update)
        {
            tx_next_page();
        }
        else if (me.done_cb)
        {
            me.done_cb(me.dma_transfer_error ? -1 : 0);
        }
    }
}

static int cli_disp_init(int argc, const char **argv)
{
    disp_init();
    return 0;
}
CLI_FUNCTION(cli_disp_init, "disp_init", ":");
static int cli_disp_ena(int argc, const char **argv)
{
    disp_set_enabled(argc == 0 || strtol(argv[0], NULL, 0), NULL);
    return 0;
}
CLI_FUNCTION(cli_disp_ena, "disp_ena", "<x>:");
static int cli_disp_upd(int argc, const char **argv)
{
    memset(me.gfx_buffer, argc == 0 ? 0x00 : strtol(argv[0], NULL, 0), sizeof(me.gfx_buffer));
    return disp_screen_update(NULL) ? 0 : -1;
}
CLI_FUNCTION(cli_disp_upd, "disp_upd", "<x>:");
