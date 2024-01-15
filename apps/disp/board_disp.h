#ifndef _BOARD_DISP_H_
#define _BOARD_DISP_H_

// #define BOARD_BUTTON_GPIO_PIN ((const uint16_t[BOARD_BUTTON_COUNT]){11, 12, 24, 25})
// #define BOARD_LED_GPIO_PIN ((const uint16_t[BOARD_LED_COUNT]){13, 14, 15, 16})

#define BOARD_DISP_TEARING_PIN (14)
#define BOARD_DISP_SPI_CSN_PIN (18)  // dedicated QSPI pin
#define BOARD_DISP_SPI_SCK_PIN (19)  // dedicated QSPI pin
#define BOARD_DISP_SPI_MOSI_PIN (23) // dedicated QSPI pin
#define BOARD_DISP_DATACOM_PIN (16)
#define BOARD_DISP_RESETN_PIN (13)
#define BOARD_DISP_SPI_MISO_PIN (21)     // dedicated QSPI pin
#define BOARD_DISP_SPI_res1_PIN (22)     // dedicated QSPI pin
#define BOARD_DISP_SPI_res2_PIN (32 + 0) // dedicated QSPI pin

#define BOARD_DISP_SPI_BUS NRF_SPIM3 // the one single spi bus having 32MHz capabilities
#define BOARD_DISP_SPI_BUS_IRQ_HANDLER_FUNC SPIM3_IRQHandler

#define MSB_FIRST (0 << 0)
#define LSB_FIRST (1 << 0)
#define CPHA_LEAD (0 << 1)
#define CPHA_TRAIL (1 << 1)
#define CPOL_HIGH (0 << 2)
#define CPOL_LOW (1 << 2)
#define BOARD_DISP_SPI_BUS_CFG  (MSB_FIRST | CPHA_TRAIL | CPOL_LOW) // spi mode 3

#define BOARD_DISP_SPI_BUS_FREQ (0x14000000)
/*
K125 0x02000000 125 kbps
K250 0x04000000 250 kbps
K500 0x08000000 500 kbps
M1 0x10000000 1 Mbps
M2 0x20000000 2 Mbps
M4 0x40000000 4 Mbps
M8 0x80000000 8 Mbps
M16 0x0A000000 16 Mbps
M32 0x14000000 32 Mbps
*/

#endif // _BOARD_DISP_H_
