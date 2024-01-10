#ifndef _BOARD_DISP_H_
#define _BOARD_DISP_H_

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

#define MSB_FIRST (0 << 2)
#define LSB_FIRST (1 << 2)
#define CPHA_LEAD (0 << 1)
#define CPHA_TRAIL (1 << 1)
#define CPOL_HIGH (0 << 0)
#define CPOL_LOW (1 << 0)
#define BOARD_DISP_SPI_BUS_CFG  (MSB_FIRST | CPHA_TRAIL | CPOL_LOW) // spi mode 3

#define BOARD_DISP_SPI_BUS_FREQ (0x14000000) // 32 MHz

#endif // _BOARD_DISP_H_
