/*
 * K19xxVG5T_it.c — interrupt handlers for K1921VG5T (RISC-V + PLIC)
 *
 * Handlers are registered with PLIC_SetIrqHandler() (see PlicIrqSetup()),
 * machine timer (MTI) is dispatched by riscv-irq.c trap entry.
 */
#include "main.h"
#include "K19xxVK035_it.h"
#include "targets.h"
#include "ADC.h"
#include "string.h"
#include "comparator.h"
#include "functions.h"
#include "peripherals.h"
#include "IO.h"
#include "common.h"

extern void transfercomplete();
extern void PeriodElapsedCallback();
extern void interruptRoutine();
extern void tenKhzRoutine();
extern void sendDshotDma();
extern void receiveDshotDma();
extern void processDshot();

extern char send_telemetry;
extern char telemetry_done;
extern char servoPwm;
extern char dshot;
extern char inputSet;
extern uint16_t ADCDataDMA[3];
extern uint32_t dma_buffer[64];
extern uint8_t buffersize;
extern uint8_t buffer_padding;

#ifndef MTIME_FREQ_HZ
#define MTIME_FREQ_HZ ((uint64_t)CPU_FREQUENCY_MHZ * 1000000ULL)
#endif

/* register all peripheral interrupt handlers with the PLIC.
 * NOTE: PLIC priority 0 means "disabled", valid range is 1..7 (7 = highest). */
void PlicIrqSetup(void)
{
    PLIC_SetThreshold(Plic_Mach_Target, 0);

    // commutation timer TMR1
    PLIC_SetIrqHandler(Plic_Mach_Target, IsrVect_IRQ_TMR1, COM_TIMER_IRQHandler);
    PLIC_SetMode(IsrVect_IRQ_TMR1, PLIC_IRQMODE_HILEVEL);
    PLIC_SetPriority(IsrVect_IRQ_TMR1, 7);

    // BEMF comparator inputs on GPIOB
    PLIC_SetIrqHandler(Plic_Mach_Target, IsrVect_IRQ_GPIOB, GPIOB_IRQHandler);
    PLIC_SetMode(IsrVect_IRQ_GPIOB, PLIC_IRQMODE_HILEVEL);
    PLIC_SetPriority(IsrVect_IRQ_GPIOB, 6);

    // ADC sequencer 0
    PLIC_SetIrqHandler(Plic_Mach_Target, IsrVect_IRQ_ADC_SEQ0, ADC_SEQ0_IRQHandler);
    PLIC_SetMode(IsrVect_IRQ_ADC_SEQ0, PLIC_IRQMODE_HILEVEL);
    PLIC_SetPriority(IsrVect_IRQ_ADC_SEQ0, 4);

    // DMA channel 0: dshot input capture, channel 1: gcr telemetry transmit
    PLIC_SetIrqHandler(Plic_Mach_Target, IsrVect_IRQ_DMA_CH0, DMA_CH0_IRQHandler);
    PLIC_SetMode(IsrVect_IRQ_DMA_CH0, PLIC_IRQMODE_HILEVEL);
    PLIC_SetPriority(IsrVect_IRQ_DMA_CH0, 3);
    PLIC_SetIrqHandler(Plic_Mach_Target, IsrVect_IRQ_DMA_CH1, DMA_CH1_IRQHandler);
    PLIC_SetMode(IsrVect_IRQ_DMA_CH1, PLIC_IRQMODE_HILEVEL);
    PLIC_SetPriority(IsrVect_IRQ_DMA_CH1, 3);
}

__RAMFUNC void MTIMER_IRQHandler(void)
{
    mtimer_set_raw_time_cmp(MTIME_FREQ_HZ / LOOP_FREQUENCY_HZ);
    tenKhzRoutine();
}

__RAMFUNC void COM_TIMER_IRQHandler(void)
{
    TMR_ITCmd(COM_TIMER, TMR_IT_TimerUpdate, DISABLE); // one-shot: re-armed by setAndEnableComInt()
    TMR_ITClear(COM_TIMER, TMR_IT_TimerUpdate);
    PeriodElapsedCallback();
}

__RAMFUNC void UART0_TD_IRQHandler(void)
{
    if(UART0->RIS_bit.TDRIS) {
        UART0->ICR_bit.TDIC = 1;
        GPIO_AltFuncCmd(GPIOB, GPIO_Pin_9, DISABLE);
    }
}

__RAMFUNC void ADC_SEQ0_IRQHandler()
{
    ADC_SEQ_ITStatusClear(ADC_SEQ_Num_0);
    /* sequencer IRQ fires after ICNT+1 = 3 FIFO writes (one full scan of
     * RQ0..RQ2), so all three results are guaranteed to be in the FIFO —
     * no polling delay needed (UM 16.2) */
    ADCDataDMA[0] = ADC_SEQ_GetFIFOData(ADC_SEQ_Num_0);
    ADCDataDMA[1] = ADC_SEQ_GetFIFOData(ADC_SEQ_Num_0);
    ADCDataDMA[2] = ADC_SEQ_GetFIFOData(ADC_SEQ_Num_0);
}

__RAMFUNC void GPIOB_IRQHandler()
{
    if (step == 1 || step == 4)
    {   // c floating
        if(GPIOB->INTSTATUS_bit.PIN6) {
            GPIOB->INTSTATUS = 0xFFFFFFFF;
            if(COMPARATOR_REGISTER->INTENSET_bit.PIN6) {
                interruptRoutine();
            }
        }
        return;
    }
    if (step == 2 || step == 5)
    {   // a floating
        if(GPIOB->INTSTATUS_bit.PIN4) {
            GPIOB->INTSTATUS = 0xFFFFFFFF;
            if(COMPARATOR_REGISTER->INTENSET_bit.PIN4) {
                interruptRoutine();
            }
        }
        return;
    }
    if (step == 3 || step == 6)
    {   // b floating
        if(GPIOB->INTSTATUS_bit.PIN5) {
            GPIOB->INTSTATUS = 0xFFFFFFFF;
            if(COMPARATOR_REGISTER->INTENSET_bit.PIN5) {
                interruptRoutine();
            }
        }
    }
    return;
}

/* DMA channel 0: dshot input capture complete (buffersize edges) */
__RAMFUNC void DMA_CH0_IRQHandler()
{
    DMA->CH[DMA_Channel_0].INT_CLEAR = DMA_CH_INT_RAWSTAT_CH_END_Msk;
    transfercomplete();
    processDshot();
}

/* DMA channel 1: gcr telemetry transmission complete */
__RAMFUNC void DMA_CH1_IRQHandler()
{
    DMA->CH[DMA_Channel_1].INT_CLEAR = DMA_CH_INT_RAWSTAT_CH_END_Msk;
    TMR0->CAPCOM[0].VAL1 = 0;
    TMR_SetMode(IC_TIMER_REGISTER, TMR_Mode_Stop);
    transfercomplete();
    processDshot();
}
