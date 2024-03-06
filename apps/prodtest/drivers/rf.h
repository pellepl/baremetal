#ifndef _RF_H_
#define _RF_H_

#include "ble_dtm.h"

void rf_enable(void);

void rf_disable(void);

/**
 * @param phy LE_PHY_1M, LE_PHY_2M, LE_PHY_LE_CODED_S8, LE_PHY_LE_CODED_S2
 */
void rf_dtm_set_phy(uint8_t phy);

/**
 * @param channel BLE_CH_MIN 0 .. BLE_CH_MAX 39
 * @param length 0 .. 37
 * @param pattern DTM_PKT_PRBS9, DTM_PKT_0X0F, DTM_PKT_0X55, DTM_PKT_0XFF
 * @param power RADIO_TXPOWER_TXPOWER_*dBm
 */
int rf_dtm_tx(uint8_t channel, uint8_t length, uint8_t pattern, int8_t power);

/**
 * @param channel BLE_CH_MIN 0 .. BLE_CH_MAX 39
 */
int rf_dtm_rx(uint8_t channel);
int rf_dtm_end(void);


#endif // _RF_H_