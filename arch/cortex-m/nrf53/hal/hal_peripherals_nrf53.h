/* Copyright (c) 2020 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */


#if defined(NRF5340_XXAA_APPLICATION)
#include "nrf5340_application_peripherals.h"
#define NRF_P0          NRF_P0_S
#define NRF_P1          NRF_P1_S
#define NRF_UARTE0      NRF_UARTE0_S
#elif defined(NRF5340_XXAA_NETWORK)
#include "nrf5340_network_peripherals.h"
#define NRF_P0          NRF_P0_NS
#define NRF_P1          NRF_P1_NS
#define NRF_UARTE0      NRF_UARTE0_NS
#else
#error Neither NRF5340_XXAA_NETWORK nor NRF5340_XXAA_APPLICATION is defined
#endif

