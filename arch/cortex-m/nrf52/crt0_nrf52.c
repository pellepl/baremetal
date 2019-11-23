/*
	This file contains the entry point (Reset_Handler) of your firmware project.
	The reset handled initializes the RAM and calls system library initializers as well as
	the platform-specific initializer and the main() function.
*/

extern void *__etext, *__data_start__, *__data_end__;
extern void *__bss_start__, *__bss_end__;
extern void *__StackTop;

#define NULL ((void *)0)

void Reset_Handler(void);
void Default_Handler(void);
extern void CRYPTOCELL_IRQHandler(void);

#if defined(NRF52840_XXAA) || defined(NRF52833_XXAA)
#define ONLY40_33(x) x
#else
#define ONLY40_33(x) NULL
#endif

#ifdef DEBUG_DEFAULT_INTERRUPT_HANDLERS
void __attribute__ ((weak)) NMI_Handler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void NMI_Handler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) HardFault_Handler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void HardFault_Handler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) MemoryManagement_Handler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void MemoryManagement_Handler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) BusFault_Handler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void BusFault_Handler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) UsageFault_Handler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void UsageFault_Handler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) SVC_Handler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void SVC_Handler();
	asm("bkpt 255");
};

void __attribute__((weak)) DebugMon_Handler()
{
    //If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
    //Define the following function in your code to handle it:
    //	extern "C" void SVC_Handler();
    asm("bkpt 255");
};

void __attribute__ ((weak)) PendSV_Handler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void PendSV_Handler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) SysTick_Handler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void SysTick_Handler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) POWER_CLOCK_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void POWER_CLOCK_IRQHandler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) RADIO_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void RADIO_IRQHandler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) UARTE0_UART0_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void UARTE0_UART0_IRQHandler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQHandler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) SPIM1_SPIS1_TWIM1_TWIS1_SPI1_TWI1_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void SPIM1_SPIS1_TWIM1_TWIS1_SPI1_TWI1_IRQHandler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) NFCT_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void NFCT_IRQHandler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) GPIOTE_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void GPIOTE_IRQHandler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) SAADC_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void SAADC_IRQHandler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) TIMER0_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void TIMER0_IRQHandler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) TIMER1_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void TIMER1_IRQHandler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) TIMER2_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void TIMER2_IRQHandler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) RTC0_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void RTC0_IRQHandler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) TEMP_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void TEMP_IRQHandler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) RNG_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void RNG_IRQHandler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) ECB_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void ECB_IRQHandler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) CCM_AAR_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void CCM_AAR_IRQHandler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) WDT_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void WDT_IRQHandler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) RTC1_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void RTC1_IRQHandler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) QDEC_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void QDEC_IRQHandler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) COMP_LPCOMP_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void COMP_LPCOMP_IRQHandler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) SWI0_EGU0_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void SWI0_EGU0_IRQHandler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) SWI1_EGU1_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void SWI1_EGU1_IRQHandler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) SWI2_EGU2_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void SWI2_EGU2_IRQHandler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) SWI3_EGU3_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void SWI3_EGU3_IRQHandler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) SWI4_EGU4_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void SWI4_EGU4_IRQHandler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) SWI5_EGU5_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void SWI5_EGU5_IRQHandler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) TIMER3_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void TIMER3_IRQHandler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) TIMER4_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void TIMER4_IRQHandler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) PWM0_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void PWM0_IRQHandler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) PDM_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void PDM_IRQHandler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) MWU_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void MWU_IRQHandler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) PWM1_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void PWM1_IRQHandler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) PWM2_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void PWM2_IRQHandler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) SPIM2_SPIS2_SPI2_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void SPIM2_SPIS2_SPI2_IRQHandler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) RTC2_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void RTC2_IRQHandler();
	asm("bkpt 255");
};

void __attribute__ ((weak)) I2S_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void I2S_IRQHandler();
	asm("bkpt 255");
};
void __attribute__ ((weak))FPU_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void FPU_IRQHandler();
	asm("bkpt 255");
};
#if defined(NRF52840_XXAA) || defined(NRF52833_XXAA)
void __attribute__ ((weak)) USBD_IRQHandler(void) { asm("bkpt 255"); };
void __attribute__ ((weak)) UARTE1_IRQHandler(void) { asm("bkpt 255"); };
void __attribute__ ((weak)) QSPI_IRQHandler(void) { asm("bkpt 255"); };
void __attribute__ ((weak)) CRYPTOCELL_IRQHandler(void) { asm("bkpt 255"); };
void __attribute__ ((weak)) SPIM3_IRQHandler(void) { asm("bkpt 255"); };
void __attribute__ ((weak)) PWMM3_IRQHandler(void) { asm("bkpt 255"); };
#endif
#else
void NMI_Handler(void)                                   __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void)                             __attribute__((weak, alias("Default_Handler")));
void MemoryManagement_Handler(void)                      __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void)                              __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void)                            __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void)                                   __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void)                              __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)                                __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void)                               __attribute__((weak, alias("Default_Handler")));
void POWER_CLOCK_IRQHandler(void)                        __attribute__((weak, alias("Default_Handler")));
void RADIO_IRQHandler(void)                              __attribute__((weak, alias("Default_Handler")));
void UARTE0_UART0_IRQHandler(void)                       __attribute__((weak, alias("Default_Handler")));
void SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQHandler(void)  __attribute__((weak, alias("Default_Handler")));
void SPIM1_SPIS1_TWIM1_TWIS1_SPI1_TWI1_IRQHandler(void)  __attribute__((weak, alias("Default_Handler")));
void NFCT_IRQHandler(void)                               __attribute__((weak, alias("Default_Handler")));
void GPIOTE_IRQHandler(void)                             __attribute__ ((weak, alias ("Default_Handler")));
void SAADC_IRQHandler(void)                              __attribute__ ((weak, alias ("Default_Handler")));
void TIMER0_IRQHandler(void)                             __attribute__ ((weak, alias ("Default_Handler")));
void TIMER1_IRQHandler(void)                             __attribute__ ((weak, alias ("Default_Handler")));
void TIMER2_IRQHandler(void)                             __attribute__ ((weak, alias ("Default_Handler")));
void RTC0_IRQHandler(void)                               __attribute__ ((weak, alias ("Default_Handler")));
void TEMP_IRQHandler(void)                               __attribute__ ((weak, alias ("Default_Handler")));
void RNG_IRQHandler(void)                                __attribute__ ((weak, alias ("Default_Handler")));
void ECB_IRQHandler(void)                                __attribute__ ((weak, alias ("Default_Handler")));
void CCM_AAR_IRQHandler(void)                            __attribute__ ((weak, alias ("Default_Handler")));
void WDT_IRQHandler(void)                                __attribute__ ((weak, alias ("Default_Handler")));
void RTC1_IRQHandler(void)                               __attribute__ ((weak, alias ("Default_Handler")));
void QDEC_IRQHandler(void)                               __attribute__ ((weak, alias ("Default_Handler")));
void COMP_LPCOMP_IRQHandler(void)                        __attribute__ ((weak, alias ("Default_Handler")));
void SWI0_EGU0_IRQHandler(void)                          __attribute__ ((weak, alias ("Default_Handler")));
void SWI1_EGU1_IRQHandler(void)                          __attribute__ ((weak, alias ("Default_Handler")));
void SWI2_EGU2_IRQHandler(void)                          __attribute__ ((weak, alias ("Default_Handler")));
void SWI3_EGU3_IRQHandler(void)                          __attribute__ ((weak, alias ("Default_Handler")));
void SWI4_EGU4_IRQHandler(void)                          __attribute__ ((weak, alias ("Default_Handler")));
void SWI5_EGU5_IRQHandler(void)                          __attribute__ ((weak, alias ("Default_Handler")));
void TIMER3_IRQHandler(void)                             __attribute__ ((weak, alias ("Default_Handler")));
void TIMER4_IRQHandler(void)                             __attribute__ ((weak, alias ("Default_Handler")));
void PWM0_IRQHandler(void)                               __attribute__ ((weak, alias ("Default_Handler")));
void PDM_IRQHandler(void)                                __attribute__ ((weak, alias ("Default_Handler")));
void MWU_IRQHandler(void)                                __attribute__ ((weak, alias ("Default_Handler")));
void PWM1_IRQHandler(void)                               __attribute__ ((weak, alias ("Default_Handler")));
void PWM2_IRQHandler(void)                               __attribute__ ((weak, alias ("Default_Handler")));
void SPIM2_SPIS2_SPI2_IRQHandler(void)                   __attribute__ ((weak, alias ("Default_Handler")));
void RTC2_IRQHandler(void)                               __attribute__ ((weak, alias ("Default_Handler")));
void I2S_IRQHandler(void)                                __attribute__ ((weak, alias ("Default_Handler")));
void FPU_IRQHandler(void)                                __attribute__ ((weak, alias ("Default_Handler")));
#if defined(NRF52840_XXAA) || defined(NRF52833_XXAA)
void USBD_IRQHandler(void)                               __attribute__ ((weak, alias ("Default_Handler")));
void UARTE1_IRQHandler(void)                             __attribute__ ((weak, alias ("Default_Handler")));
void QSPI_IRQHandler(void)                               __attribute__ ((weak, alias ("Default_Handler")));
void CRYPTOCELL_IRQHandler(void)                         __attribute__ ((weak, alias ("Default_Handler")));
void SPIM3_IRQHandler(void)                              __attribute__ ((weak, alias ("Default_Handler")));
void PWMM3_IRQHandler(void)                              __attribute__ ((weak, alias ("Default_Handler")));
#endif
#endif

void * g_pfnVectors[0x100] __attribute__ ((section (".isr_vector"))) = 
{
	&__StackTop,
	&Reset_Handler,
	&NMI_Handler,
	&HardFault_Handler,
	&MemoryManagement_Handler,
	&BusFault_Handler,
	&UsageFault_Handler,
	NULL,
	NULL,
	NULL,
	NULL,
	&SVC_Handler,
	&DebugMon_Handler,
	NULL,
	&PendSV_Handler,
	&SysTick_Handler,
	&POWER_CLOCK_IRQHandler,
	&RADIO_IRQHandler,
	&UARTE0_UART0_IRQHandler,
	&SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQHandler,
	&SPIM1_SPIS1_TWIM1_TWIS1_SPI1_TWI1_IRQHandler,
	&NFCT_IRQHandler,
	&GPIOTE_IRQHandler,
	&SAADC_IRQHandler,
	&TIMER0_IRQHandler,
	&TIMER1_IRQHandler,
	&TIMER2_IRQHandler,
	&RTC0_IRQHandler,
	&TEMP_IRQHandler,
	&RNG_IRQHandler,
	&ECB_IRQHandler,
	&CCM_AAR_IRQHandler,
	&WDT_IRQHandler,
	&RTC1_IRQHandler,
	&QDEC_IRQHandler,
	&COMP_LPCOMP_IRQHandler,
	&SWI0_EGU0_IRQHandler,
	&SWI1_EGU1_IRQHandler,
	&SWI2_EGU2_IRQHandler,
	&SWI3_EGU3_IRQHandler,
	&SWI4_EGU4_IRQHandler,
	&SWI5_EGU5_IRQHandler,
	&TIMER3_IRQHandler,
	&TIMER4_IRQHandler,
	&PWM0_IRQHandler,
	&PDM_IRQHandler,
	NULL,
	NULL,
	&MWU_IRQHandler,
	&PWM1_IRQHandler,
	&PWM2_IRQHandler,
	&SPIM2_SPIS2_SPI2_IRQHandler,
	&RTC2_IRQHandler,
	&I2S_IRQHandler,
	&FPU_IRQHandler,
	ONLY40_33(&USBD_IRQHandler),
	ONLY40_33(&UARTE1_IRQHandler),
	ONLY40_33(&QSPI_IRQHandler),
	ONLY40_33(&CRYPTOCELL_IRQHandler),
	ONLY40_33(&SPIM3_IRQHandler),
	NULL,
	ONLY40_33(&PWMM3_IRQHandler),
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

#include "nrf52.h"

int main(void);

void __attribute__((naked, noreturn)) Reset_Handler()
{
	SCB->VTOR = (uint32_t)(intptr_t)g_pfnVectors;
	void **src, **dst;
	for (src = &__etext, dst = &__data_start__; dst != &__data_end__; src++, dst++)
		*dst = *src;

	for (dst = &__bss_start__; dst != &__bss_end__; dst++)
		*dst = 0;

	SystemInit();
	(void)main();
	for (;;) ;
}

void __attribute__((naked)) Default_Handler()
{
	for (;;) ;
}
