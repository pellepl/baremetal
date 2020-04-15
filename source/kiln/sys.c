#include "cpu.h"
#include "stm32f1xx.h"
#include "minio.h"
#include "disp.h"
#include "assert.h"
#include "stm32f1xx_ll_system.h"
#include "stm32f1xx_ll_iwdg.h"
#include "uart_driver.h"
#include "stm32f1xx_ll_rcc.h"

void HardFault_Handler(void);
void hard_fault_handler_c(unsigned int * hardfault_args);
void app_shutdown_from_hardfault(void);
void app_shutdown_from_hardfault_done(void);
void sys_force_dump(void) {
    __disable_irq();
    SCB->CCR |= 0x10; // enable div by zero exception
    volatile int a = 1;
    volatile int b = 0;
    a = a/b;
}

__attribute__ (( weak  )) void app_shutdown_from_hardfault(void) {}
__attribute__ (( weak  )) void app_shutdown_from_hardfault_done(void) {}
__attribute__(( naked )) void HardFault_Handler(void) {
    asm volatile (
    "tst   lr, #4 \n\t"
    "ite   eq \n\t"
    "mrseq r0, msp \n\t"
    "mrsne r0, psp \n\t"
    "b     hard_fault_handler_c \n\t"
    );
}

__attribute__(( section (".nobss") )) static struct {
    uint32_t magic;
    uint32_t reset_valid;
    reset_reason_t reset_reason;
    uint32_t bootcnt;
    uint32_t unexpected_reset;
} nobss;

void sys_init(void) {
    int ram_virgin = 0;
    int reset_valid = 0;
    if (nobss.magic != 0xd0decaed) {
        nobss.magic = 0xd0decaed;
        nobss.bootcnt = 0;
    } else {
        nobss.bootcnt++;
        nobss.unexpected_reset = nobss.reset_valid != 0xcafec01a;
    }
    nobss.reset_valid = 0;
    uint32_t reset_reas = RCC->CSR;
    RCC->CSR |= (1<<24);
    if (ram_virgin) printf("Cold boot\n");
    if (!reset_valid) printf("Unexpected reset\n");
    if (reset_reas & (1<<26)) nobss.reset_reason = RR_PIN;
    if (reset_reas & (1<<27)) nobss.reset_reason = RR_PORPDR;
    if (reset_reas & (1<<28)) nobss.reset_reason = RR_SW;
    if (reset_reas & (1<<29)) nobss.reset_reason = RR_IWDG;
    if (reset_reas & (1<<30)) nobss.reset_reason = RR_WWDG;
    if (reset_reas & (1<<31)) nobss.reset_reason = RR_LOW_POWER;
}

reset_reason_t sys_reset_reason(uint32_t *bootcnt, uint32_t *unexpected) {
    if (bootcnt) *bootcnt = nobss.bootcnt;
    if (unexpected) *unexpected = nobss.unexpected_reset;
    return nobss.reset_reason;
}

void sys_reset(void) {
    nobss.reset_valid = 0xcafec01a;
    cpu_reset();
}

void sys_watchdog_start(uint32_t ms) {
    uint32_t ticks = (40*ms) / 256;
    LL_DBGMCU_APB1_GRP1_FreezePeriph(DBGMCU_CR_DBG_IWDG_STOP);
    LL_IWDG_EnableWriteAccess(IWDG);
    LL_IWDG_SetPrescaler(IWDG, LL_IWDG_PRESCALER_256);
    if (ticks > 0xfff) ticks = 0xfff;
    LL_IWDG_SetReloadCounter(IWDG, ticks);
    LL_IWDG_ReloadCounter(IWDG);
    LL_IWDG_Enable(IWDG);
    while (!LL_IWDG_IsReady(IWDG));
}

void sys_watchdog_feed(void) {
    LL_IWDG_ReloadCounter(IWDG);
}

volatile void *stack_pointer;
volatile unsigned int stacked_r0;
volatile unsigned int stacked_r1;
volatile unsigned int stacked_r2;
volatile unsigned int stacked_r3;
volatile unsigned int stacked_r12;
volatile unsigned int stacked_lr;
volatile unsigned int stacked_pc;
volatile unsigned int stacked_psr;

void hard_fault_handler_c(unsigned int * hardfault_args) {
    app_shutdown_from_hardfault();

    stacked_r0 = ((unsigned long) hardfault_args[0]);
    stacked_r1 = ((unsigned long) hardfault_args[1]);
    stacked_r2 = ((unsigned long) hardfault_args[2]);
    stacked_r3 = ((unsigned long) hardfault_args[3]);

    stacked_r12 = ((unsigned long) hardfault_args[4]);
    stacked_lr = ((unsigned long) hardfault_args[5]);
    stacked_pc = ((unsigned long) hardfault_args[6]);
    stacked_psr = ((unsigned long) hardfault_args[7]);

    uint32_t bfar = SCB->BFAR;
    uint32_t cfsr = SCB->CFSR;
    uint32_t hfsr = SCB->HFSR;
    uint32_t dfsr = SCB->DFSR;
    uint32_t afsr = SCB->AFSR;

    printf("\n!!! HARDFAULT !!!\n\n");
    printf("Stacked registers:\n");
    printf("  pc:   0x%08x\n", stacked_pc);
    printf("  lr:   0x%08x\n", stacked_lr);
    printf("  psr:  0x%08x\n", stacked_psr);
    printf("  sp:   0x%08x\n", stack_pointer);
    printf("  r0:   0x%08x\n", stacked_r0);
    printf("  r1:   0x%08x\n", stacked_r1);
    printf("  r2:   0x%08x\n", stacked_r2);
    printf("  r3:   0x%08x\n", stacked_r3);
    printf("  r12:  0x%08x\n", stacked_r12);

    printf("\nFault status registers:\n");
    printf("  BFAR: 0x%08x\n", bfar);
    printf("  CFSR: 0x%08x\n", cfsr);
    printf("  HFSR: 0x%08x\n", hfsr);
    printf("  DFSR: 0x%08x\n", dfsr);
    printf("  AFSR: 0x%08x\n", afsr);
    printf("\n");
    if (cfsr & (1<<(7+0))) {
        printf("MMARVALID: MemMan 0x%08x\n", SCB->MMFAR);
    }
    if (cfsr & (1<<(4+0))) {
        printf("MSTKERR: MemMan error during stacking\n");
    }
    if (cfsr & (1<<(3+0))) {
        printf("MUNSTKERR: MemMan error during unstacking\n");
    }
    if (cfsr & (1<<(1+0))) {
        printf("DACCVIOL: MemMan memory access violation, data\n");
    }
    if (cfsr & (1<<(0+0))) {
        printf("IACCVIOL: MemMan memory access violation, instr\n");
    }

    if (cfsr & (1<<(7+8))) {
        printf("BFARVALID: BusFlt 0x%08x\n", SCB->BFAR);
    }
    if (cfsr & (1<<(4+8))) {
        printf("STKERR: BusFlt error during stacking\n");
    }
    if (cfsr & (1<<(3+8))) {
        printf("UNSTKERR: BusFlt error during unstacking\n");
    }
    if (cfsr & (1<<(2+8))) {
        printf("IMPRECISERR: BusFlt error during data access\n");
    }
    if (cfsr & (1<<(1+8))) {
        printf("PRECISERR: BusFlt error during data access\n");
    }
    if (cfsr & (1<<(0+8))) {
        printf("IBUSERR: BusFlt bus error\n");
    }

    if (cfsr & (1<<(9+16))) {
        printf("DIVBYZERO: UsaFlt division by zero\n");
    }
    if (cfsr & (1<<(8+16))) {
        printf("UNALIGNED: UsaFlt unaligned access\n");
    }
    if (cfsr & (1<<(3+16))) {
        printf("NOCP: UsaFlt execute coprocessor instr\n");
    }
    if (cfsr & (1<<(2+16))) {
        printf("INVPC: UsaFlt general\n");
    }
    if (cfsr & (1<<(1+16))) {
        printf("INVSTATE: UsaFlt execute ARM instr\n");
    }
    if (cfsr & (1<<(0+16))) {
        printf("UNDEFINSTR: UsaFlt execute bad instr\n");
    }

    if (hfsr & (1<<31)) {
        printf("DEBUGEVF: HardFlt debug event\n");
    }
    if (hfsr & (1<<30)) {
        printf("FORCED: HardFlt SVC/BKPT within SVC\n");
    }
    if (hfsr & (1<<1)) {
        printf("VECTBL: HardFlt vector fetch failed\n");
    }

    disp_set_clip_d(0,0,disp_width(),disp_height());
    disp_draw_string("   SYSTEM   ", disp_width()/2, disp_height()/2-16, 6,
        DISP_STR_ALIGN_CENTER, DISP_STR_ALIGN_CENTER, 0xffffffff, 0xffff0000);
    disp_draw_string("   ERROR!   ", disp_width()/2, disp_height()/2+16, 6,
        DISP_STR_ALIGN_CENTER, DISP_STR_ALIGN_CENTER, 0xffffffff, 0xffff0000);

    app_shutdown_from_hardfault_done();

    if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) {
        // if debug is enabled, breakpoint
        __asm__ volatile ("bkpt #0\n");
    }
    volatile int a = 0x3000000;
    while(--a);
    cpu_reset();
}
