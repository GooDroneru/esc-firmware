/*
 * peripherals.c — K1921VG5T (NIIET RISC-V, SCR4), 96 MHz PLL from 16 MHz HSE
 *
 * Timer allocation:
 *   TMR0 — DShot input capture / GCR telemetry output on PA14 (TMR0_IO)
 *   TMR1 — commutation timer (COM_TIMER)
 *   TMR2 — zero-cross interval timer (INTERVAL_TIMER, free running)
 *   TMR3 — blocking us-delay timer (DELAY_TIMER)
 *   mtimer (RISC-V machine timer) — 20 kHz control loop tick
 *
 *  Created on: Sep. 26, 2020
 *      Author: Alka
 *      Ported to K1921VG5T, 2026
 */

// PERIPHERAL SETUP
#include "peripherals.h"

#include "ADC.h"
#include "common.h"
#include "functions.h"
#include "plib5t_dma.h"
#include "serial_telemetry.h"
#include "targets.h"

#ifndef MTIME_FREQ_HZ
#define MTIME_FREQ_HZ ((uint64_t)CPU_FREQUENCY_MHZ * 1000000ULL)
#endif

/* TMR is clocked by SYSCLK; interval/com time is kept in 0.5 us units
 * like on K19XXVK035 (there /50 @ 100 MHz, here /(CPU_FREQUENCY_MHZ/2)). */
#define TIMER_TICKS_PER_HALFUS (CPU_FREQUENCY_MHZ / 2)

void initCorePeripherals(void)
{
  PlicIrqSetup();
  UN_TIM0_Init( );    // dshot input capture timer + DMA
  ALL_GPIO_Init( );   //gpio clock
  ALL_DMA_Init( );    //IC DMA
  PWM_TIM1_Init( );   //6 channels PWM
  ZC_TIM2_Init( );    //zero cross timer
  ALL_COMP_Init( );   //ALL comparer
  COM_TIM1_Init( );   //
  TENKHz_SysTick_Init( );
  MX_TIM16_Init( );

  #ifdef USE_SERIAL_TELEMETRY
  telem_UART_Init();
  #endif
}

void initAfterJump(void)
{
    // clocks are configured by the bootloader, nothing to do
}

void SystemClock_Config(void)
{
    SystemCoreClockUpdate();
}

void setAutoReloadPWM(uint16_t relval)
{
  PWM0->TBPRD = relval;
  PWM1->TBPRD = relval;
  PWM2->TBPRD = relval;
}

void setPrescalerPWM(uint16_t presc){
  if(presc == 0) {
    PWM0->TBCTL_bit.CLKDIV = PWM_TBCTL_CLKDIV_Div2;        //12 KHz
    PWM1->TBCTL_bit.CLKDIV = PWM_TBCTL_CLKDIV_Div2;        //12 KHz
    PWM2->TBCTL_bit.CLKDIV = PWM_TBCTL_CLKDIV_Div2;        //12 KHz
    PWM0->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div1; //48 KHz
    PWM1->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div1; //48 KHz
    PWM2->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div1; //48 KHz
  }
  if(presc == 10) {
    PWM0->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div10;
    PWM1->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div10;
    PWM2->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div10;
  }
  else if(presc == 20) {
    PWM0->TBCTL_bit.CLKDIV = PWM_TBCTL_CLKDIV_Div2;
    PWM1->TBCTL_bit.CLKDIV = PWM_TBCTL_CLKDIV_Div2;
    PWM2->TBCTL_bit.CLKDIV = PWM_TBCTL_CLKDIV_Div2;
    PWM0->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div10;
    PWM1->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div10;
    PWM2->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div10;
  }
  else if(presc == 25) {
    PWM0->TBCTL_bit.CLKDIV = PWM_TBCTL_CLKDIV_Div2;
    PWM1->TBCTL_bit.CLKDIV = PWM_TBCTL_CLKDIV_Div2;
    PWM2->TBCTL_bit.CLKDIV = PWM_TBCTL_CLKDIV_Div2;
    PWM0->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div12;
    PWM1->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div12;
    PWM2->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div12;
  }
  else if(presc == 30) {
    PWM0->TBCTL_bit.CLKDIV = PWM_TBCTL_CLKDIV_Div2;
    PWM1->TBCTL_bit.CLKDIV = PWM_TBCTL_CLKDIV_Div2;
    PWM2->TBCTL_bit.CLKDIV = PWM_TBCTL_CLKDIV_Div2;
    PWM0->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div14;
    PWM1->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div14;
    PWM2->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div14;
  }
  else if(presc == 40) {
    PWM0->TBCTL_bit.CLKDIV = PWM_TBCTL_CLKDIV_Div4;
    PWM1->TBCTL_bit.CLKDIV = PWM_TBCTL_CLKDIV_Div4;
    PWM2->TBCTL_bit.CLKDIV = PWM_TBCTL_CLKDIV_Div4;
    PWM0->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div10;
    PWM1->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div10;
    PWM2->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div10;
  }
  else if(presc == 50) {
    PWM0->TBCTL_bit.CLKDIV = PWM_TBCTL_CLKDIV_Div4;
    PWM1->TBCTL_bit.CLKDIV = PWM_TBCTL_CLKDIV_Div4;
    PWM2->TBCTL_bit.CLKDIV = PWM_TBCTL_CLKDIV_Div4;
    PWM0->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div12;
    PWM1->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div12;
    PWM2->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div12;
  }
  else if(presc == 55) {
    PWM0->TBCTL_bit.CLKDIV = PWM_TBCTL_CLKDIV_Div4;
    PWM1->TBCTL_bit.CLKDIV = PWM_TBCTL_CLKDIV_Div4;
    PWM2->TBCTL_bit.CLKDIV = PWM_TBCTL_CLKDIV_Div4;
    PWM0->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div14;
    PWM1->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div14;
    PWM2->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div14;
  }
  else if(presc == 60) {
    PWM0->TBCTL_bit.CLKDIV = PWM_TBCTL_CLKDIV_Div8;
    PWM1->TBCTL_bit.CLKDIV = PWM_TBCTL_CLKDIV_Div8;
    PWM2->TBCTL_bit.CLKDIV = PWM_TBCTL_CLKDIV_Div8;
    PWM0->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div8;
    PWM1->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div8;
    PWM2->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div8;
  }
  else if(presc == 70) {
    PWM0->TBCTL_bit.CLKDIV = PWM_TBCTL_CLKDIV_Div8;
    PWM1->TBCTL_bit.CLKDIV = PWM_TBCTL_CLKDIV_Div8;
    PWM2->TBCTL_bit.CLKDIV = PWM_TBCTL_CLKDIV_Div8;
    PWM0->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div10;
    PWM1->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div10;
    PWM2->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div10;
  }
  else if(presc == 80) {
    PWM0->TBCTL_bit.CLKDIV = PWM_TBCTL_CLKDIV_Div8;
    PWM1->TBCTL_bit.CLKDIV = PWM_TBCTL_CLKDIV_Div8;
    PWM2->TBCTL_bit.CLKDIV = PWM_TBCTL_CLKDIV_Div8;
    PWM0->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div12;
    PWM1->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div12;
    PWM2->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div12;
  }
  else if(presc == 90) {
    PWM0->TBCTL_bit.CLKDIV = PWM_TBCTL_CLKDIV_Div8;
    PWM1->TBCTL_bit.CLKDIV = PWM_TBCTL_CLKDIV_Div8;
    PWM2->TBCTL_bit.CLKDIV = PWM_TBCTL_CLKDIV_Div8;
    PWM0->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div14;
    PWM1->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div14;
    PWM2->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div14;
  }
}

void setDutyCycleAll(uint16_t newdc){
  PWM0->CMPA_bit.CMPA = newdc;
  PWM0->CMPB_bit.CMPB = TIM1_AUTORELOAD - newdc;
  PWM1->CMPA_bit.CMPA = newdc;
  PWM1->CMPB_bit.CMPB = TIM1_AUTORELOAD - newdc;
  PWM2->CMPA_bit.CMPA = newdc;
  PWM2->CMPB_bit.CMPB = TIM1_AUTORELOAD - newdc;
}

void resetInputCaptureTimer()
{
  IC_TIMER_REGISTER->COUNT = 0;
}

void setPWMCompare1(uint16_t compareone)
{
  PWM0->CMPA_bit.CMPA = compareone;
  PWM0->CMPB_bit.CMPB = compareone;
}
void setPWMCompare2(uint16_t comparetwo)
{
  PWM1->CMPA_bit.CMPA = comparetwo;
  PWM1->CMPB_bit.CMPB = comparetwo;
}
void setPWMCompare3(uint16_t comparethree)
{
  PWM2->CMPA_bit.CMPA = comparethree;
  PWM2->CMPB_bit.CMPB = comparethree;
}

void enableCorePeripherals(){
    TMR_ITCmd(COM_TIMER, TMR_IT_TimerUpdate, DISABLE);
    TMR_SetMode(COM_TIMER, TMR_Mode_Capcom_Up);

#ifdef USE_ADC
    ADCInit();
#endif
}

void generatePwmTimerEvent(){
}

void ALL_COMP_Init(void)
{
    // BEMF comparator inputs PB4 (A), PB5 (B), PB6 (C) — GPIO edge interrupts
    GPIOB->PULLMODE_bit.PIN4 = 0x01;
    GPIOB->PULLMODE_bit.PIN5 = 0x01;
    GPIOB->PULLMODE_bit.PIN6 = 0x01;
    GPIOB->INTTYPESET_bit.PIN4 = 1;
    GPIOB->INTTYPESET_bit.PIN5 = 1;
    GPIOB->INTTYPESET_bit.PIN6 = 1;
    GPIOB->INTEDGESET_bit.PIN4 = 1;
    GPIOB->INTEDGESET_bit.PIN5 = 1;
    GPIOB->INTEDGESET_bit.PIN6 = 1;
    GPIOB->INTSTATUS = 0xFFFFFFFF; // clear stale flags
}

void MX_IWDG_Init(void)
{
    /* WDT clock: WDTCFG.CLKSEL 00b = HSICLK 4 MHz (default) */
    RCU->WDTCFG_bit.CLKEN = 1;
    RCU->WDTCFG_bit.RSTDIS = 1;
    WDT->LOAD = 200000; // 50 ms @ 4 MHz
    WDT->CTRL_bit.INTEN = 1;
    WDT->CTRL_bit.RESEN = 1;
}

void PWM_TIM1_Init(void)  //PWM
{
    // select AF1 (PWM0_A/B, PWM1_A/B, PWM2_A/B) and enable altfunc push-pull outputs
    GPIOA->ALTFUNCNUM_bit.PIN8 = 1;
    GPIOA->ALTFUNCNUM_bit.PIN9 = 1;
    GPIOA->ALTFUNCNUM_bit.PIN10 = 1;
    GPIOA->ALTFUNCNUM_bit.PIN11 = 1;
    GPIOA->ALTFUNCNUM_bit.PIN12 = 1;
    GPIOA->ALTFUNCNUM_bit.PIN13 = 1;
    GPIOA->ALTFUNCSET |= GPIO_ALTFUNCSET_PIN8_Msk | GPIO_ALTFUNCSET_PIN9_Msk | GPIO_ALTFUNCSET_PIN10_Msk | GPIO_ALTFUNCSET_PIN11_Msk | GPIO_ALTFUNCSET_PIN12_Msk | GPIO_ALTFUNCSET_PIN13_Msk;
    GPIOA->OUTENSET |= GPIO_OUTENSET_PIN8_Msk | GPIO_OUTENSET_PIN9_Msk | GPIO_OUTENSET_PIN10_Msk | GPIO_OUTENSET_PIN11_Msk | GPIO_OUTENSET_PIN12_Msk | GPIO_OUTENSET_PIN13_Msk;

    PWM0->TBCTL_bit.CLKDIV = PWM_TBCTL_CLKDIV_Div2;        //48 MHz -> 24 KHz
    PWM0->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div1;
    PWM1->TBCTL_bit.CLKDIV = PWM_TBCTL_CLKDIV_Div2;        //48 MHz -> 24 KHz
    PWM1->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div1;
    PWM2->TBCTL_bit.CLKDIV = PWM_TBCTL_CLKDIV_Div2;        //48 MHz -> 24 KHz
    PWM2->TBCTL_bit.HSPCLKDIV = PWM_TBCTL_HSPCLKDIV_Div1;
    PWM0->ETSEL_bit.SOCAEN = 1;
    PWM0->ETSEL_bit.SOCASEL = PWM_ETSEL_INTSEL_CTREqZero;
    PWM0->TBPRD = TIM1_AUTORELOAD; //24 KHz
    PWM0->AQCTLA_bit.PRD = PWM_AQCTLA_PRD_Set;
    PWM0->AQCTLA_bit.CAU = PWM_AQCTLA_CAU_Clear;
    PWM0->AQCTLB_bit.PRD = PWM_AQCTLB_PRD_Clear;
    PWM0->AQCTLB_bit.CBU = PWM_AQCTLB_CBU_Set;
    PWM1->TBPRD = TIM1_AUTORELOAD; //24 KHz
    PWM1->AQCTLA_bit.PRD = PWM_AQCTLA_PRD_Set;
    PWM1->AQCTLA_bit.CAU = PWM_AQCTLA_CAU_Clear;
    PWM1->AQCTLB_bit.PRD = PWM_AQCTLB_PRD_Clear;
    PWM1->AQCTLB_bit.CBU = PWM_AQCTLB_CBU_Set;
    PWM2->TBPRD = TIM1_AUTORELOAD; //24 KHz
    PWM2->AQCTLA_bit.PRD = PWM_AQCTLA_PRD_Set;
    PWM2->AQCTLA_bit.CAU = PWM_AQCTLA_CAU_Clear;
    PWM2->AQCTLB_bit.PRD = PWM_AQCTLB_PRD_Clear;
    PWM2->AQCTLB_bit.CBU = PWM_AQCTLB_CBU_Set;
    PWM0->DBRED = DEAD_TIME;
    PWM0->DBFED = DEAD_TIME;
    PWM0->DBCTL_bit.OUTMODE = 0x03;
    PWM0->DBCTL_bit.POLSEL = 0x02;
    PWM1->DBRED = DEAD_TIME;
    PWM1->DBFED = DEAD_TIME;
    PWM1->DBCTL_bit.OUTMODE = 0x03;
    PWM1->DBCTL_bit.POLSEL = 0x02;
    PWM2->DBRED = DEAD_TIME;
    PWM2->DBFED = DEAD_TIME;
    PWM2->DBCTL_bit.OUTMODE = 0x03;
    PWM2->DBCTL_bit.POLSEL = 0x02;
    PWM0->DBCTL_bit.INMODE =  0;
    PWM1->DBCTL_bit.INMODE =  0;
    PWM2->DBCTL_bit.INMODE =  0;

    // single write to SIU->CNTEN starts all three PWM counters in sync
    SIU->CNTEN = SIU_CNTEN_PWM0EN_Msk | SIU_CNTEN_PWM1EN_Msk | SIU_CNTEN_PWM2EN_Msk;
}

void ZC_TIM2_Init(void)
{
    // free-running zero-cross interval timer
    TMR_SetDivider(INTERVAL_TIMER, 0);
    INTERVAL_TIMER->COUNT = 0;
    TMR_SetMode(INTERVAL_TIMER, TMR_Mode_Multiple);
    SIU->CNTEN |= SIU_CNTEN_TMR2EN_Msk;
}

void disableComTimerInt()
{
    TMR_ITCmd(COM_TIMER, TMR_IT_TimerUpdate, DISABLE);
}

void enableComTimerInt()
{
    TMR_ITCmd(COM_TIMER, TMR_IT_TimerUpdate, ENABLE);
}

/* preset added into getintervaTimerCount() so that it returns the last
 * SET_INTERVAL_TIMER_COUNT() value immediately, like the vk035 down-counter */
static uint32_t interval_preset_ticks = 0;

uint16_t getintervaTimerCount()
{
  return (uint16_t)((INTERVAL_TIMER->COUNT + interval_preset_ticks) / TIMER_TICKS_PER_HALFUS);
}

void setintervaTimerCount(uint16_t intertime)
{
  interval_preset_ticks = (uint32_t)intertime * TIMER_TICKS_PER_HALFUS;
  INTERVAL_TIMER->COUNT = 0;
}

void setAndEnableComInt(uint16_t time)
{
    COM_TIMER->COUNT = 0;
    COM_TIMER->PERIOD = (uint32_t)time * TIMER_TICKS_PER_HALFUS;
    TMR_ITClear(COM_TIMER, TMR_IT_TimerUpdate);
    TMR_ITCmd(COM_TIMER, TMR_IT_TimerUpdate, DISABLE); // re-enabled below
    TMR_SetMode(COM_TIMER, TMR_Mode_Capcom_Up);
    TMR_ITCmd(COM_TIMER, TMR_IT_TimerUpdate, ENABLE);
}

void TENKHz_SysTick_Init(void)
{
    /* 20 kHz control loop tick from the RISC-V machine timer.
     * After reset mtimer runs from the core clock (CLKSRC=0, DIVIDER=0,
     * UM 10.1) = 96 MHz. mtimer_set_raw_time_cmp() raises MTI (mcause 7). */
    riscv_irq_init();
    riscv_irq_set_handler(RISCV_IRQ_MTI, MTIMER_IRQHandler);
    // route external (PLIC) interrupts through the same trap entry
    riscv_irq_set_handler(RISCV_IRQ_MEI, PLIC_MachHandler);
    riscv_irq_enable(RISCV_IRQ_MTI);
    riscv_irq_enable(RISCV_IRQ_MEI);
    riscv_irq_global_enable();
    mtimer_set_raw_time_cmp(MTIME_FREQ_HZ / LOOP_FREQUENCY_HZ);
}

void reloadWatchDogCounter()
{
    WDT->INTCLR = 1;
}

void COM_TIM1_Init(void)
{
    TMR_SetDivider(COM_TIMER, 0);
    COM_TIMER->COUNT = 0;
    COM_TIMER->PERIOD = 0xFFFFFFFF;
    TMR_SetMode(COM_TIMER, TMR_Mode_Stop);
    SIU->CNTEN |= SIU_CNTEN_TMR1EN_Msk;
}

void MX_TIM16_Init(void)
{
    // blocking us-delay timer
    TMR_SetDivider(DELAY_TIMER, 0);
    DELAY_TIMER->COUNT = 0;
    DELAY_TIMER->PERIOD = 0xFFFFFFFF;
    TMR_SetMode(DELAY_TIMER, TMR_Mode_Stop);
    SIU->CNTEN |= SIU_CNTEN_TMR3EN_Msk;
}

void MX_TIM17_Init(void)
{
}

void ALL_DMA_Init(void)
{
    // DMA controller clock + reset release (CGCFGAPB.DMAEN / RSTDISAPB.DMAEN)
    RCU->CGCFGAPB_bit.DMAEN = 1;
    RCU->RSTDISAPB_bit.DMAEN = 1;
    DMA_DeInit();
}

void ALL_GPIO_Init(void)
{
    RCU->CGCFGAHB_bit.GPIOBEN = 1;
    RCU->RSTDISAHB_bit.GPIOBEN = 1;
    RCU->CGCFGAHB_bit.GPIOAEN = 1;
    RCU->RSTDISAHB_bit.GPIOAEN = 1;
}

extern uint32_t dma_buffer[64];

/* arm DMA channel 0: CAPCOM_VAL0 -> dma_buffer[], paced by TMR0 capture events */
void updateDma() {
  DMA_CH_ActiveCmd(DMA_Channel_0, DISABLE);
  DMA_ChannelEnableCmd(DMA_Channel_MSK(DMA_Channel_0), DISABLE);
  DMA_CH_SrcPtrConfig(DMA_Channel_0, (uint32_t)&TMR0->CAPCOM[0].VAL0);
  DMA_CH_DstPtrConfig(DMA_Channel_0, (uint32_t)&dma_buffer[0]);
  DMA_CH_DataSizeConfig(DMA_Channel_0, buffersize);   // NDTL: transfers
  DMA_CH_SrcDataSizeConfig(DMA_Channel_0, DMA_DataSize_32);
  DMA_CH_DstDataSizeConfig(DMA_Channel_0, DMA_DataSize_32);
  DMA_CH_SrcDataIncConfig(DMA_Channel_0, DISABLE);
  DMA_CH_DstDataIncConfig(DMA_Channel_0, ENABLE);
  DMA_CH_ReadPeripheralNumConfig(DMA_Channel_0, TMR0_DMA_REQUESTOR_IDX);
  DMA_CH_WritePeripheralNumConfig(DMA_Channel_0, MEMORY_DMA_REQUESTOR_IDX);
  DMA->CH[DMA_Channel_0].CONFIG_bit.CMD_SET_INT = 1;
  DMA->CH[DMA_Channel_0].CONFIG_bit.CMD_LAST = 1;
  DMA->CH[DMA_Channel_0].INT_ENABLE = DMA_CH_INT_ENABLE_CH_END_Msk;
  DMA->CH[DMA_Channel_0].INT_CLEAR = DMA_CH_INT_RAWSTAT_CH_END_Msk;
  DMA_ChannelEnableCmd(DMA_Channel_MSK(DMA_Channel_0), ENABLE);
  DMA_CH_StartCmd(DMA_Channel_0);
}

/* arm DMA channel 1: gcr[] -> TMR0 CAPCOM_VAL1, paced by TMR0 period events */
void updateDmaTransmit() {
  DMA_CH_ActiveCmd(DMA_Channel_1, DISABLE);
  DMA_ChannelEnableCmd(DMA_Channel_MSK(DMA_Channel_1), DISABLE);
  DMA_CH_SrcPtrConfig(DMA_Channel_1, (uint32_t)&gcr[buffer_padding]);
  DMA_CH_DstPtrConfig(DMA_Channel_1, (uint32_t)&TMR0->CAPCOM[0].VAL1);
  DMA_CH_DataSizeConfig(DMA_Channel_1, 22);           // NDTL: 22 words
  DMA_CH_SrcDataSizeConfig(DMA_Channel_1, DMA_DataSize_32);
  DMA_CH_DstDataSizeConfig(DMA_Channel_1, DMA_DataSize_32);
  DMA_CH_SrcDataIncConfig(DMA_Channel_1, ENABLE);
  DMA_CH_DstDataIncConfig(DMA_Channel_1, DISABLE);
  DMA_CH_ReadPeripheralNumConfig(DMA_Channel_1, MEMORY_DMA_REQUESTOR_IDX);
  DMA_CH_WritePeripheralNumConfig(DMA_Channel_1, TMR0_DMA_REQUESTOR_IDX);
  DMA->CH[DMA_Channel_1].CONFIG_bit.CMD_SET_INT = 1;
  DMA->CH[DMA_Channel_1].CONFIG_bit.CMD_LAST = 1;
  DMA->CH[DMA_Channel_1].INT_ENABLE = DMA_CH_INT_ENABLE_CH_END_Msk;
  DMA->CH[DMA_Channel_1].INT_CLEAR = DMA_CH_INT_RAWSTAT_CH_END_Msk;
  DMA_ChannelEnableCmd(DMA_Channel_MSK(DMA_Channel_1), ENABLE);
  DMA_CH_StartCmd(DMA_Channel_1);
}

void setDmaCnt(uint8_t size) {
  DMA_CH_DataSizeConfig(DMA_Channel_0, size);
}

void reverseBuffer() {
  // not needed on VG5T: TMR0 counts up, captures are direct elapsed timestamps
}

void UN_TIM0_Init(void)
{
    RCU->CGCFGAPB_bit.TMR0EN = 1;
    RCU->RSTDISAPB_bit.TMR0EN = 1;

    // PA14 -> AF1 = TMR0_IO, alternate function input
    GPIOA->ALTFUNCNUM_bit.PIN14 = 1;
    GPIOA->ALTFUNCSET_bit.PIN14 = 1;
    GPIOA->OUTENCLR = GPIO_OUTENSET_PIN14_Msk;

    // capture source for TMR0 CCIA = external pin TMR0_IO (SIU->TMRMUX.TMR0CCIA = 0)
    SIU->TMRMUX_bit.TMR0CCIA = 0x0;

    // TMR0 free-running, CAPCOM in capture mode on both edges
    TMR_SetDivider(IC_TIMER_REGISTER, 0);
    IC_TIMER_REGISTER->COUNT = 0;
    IC_TIMER_REGISTER->CAPCOM[0].CTRL_bit.CAP = 1;      // capture mode
    IC_TIMER_REGISTER->CAPCOM[0].CTRL_bit.CCISEL = 0;   // CCIA
    IC_TIMER_REGISTER->CAPCOM[0].CTRL_bit.CAPMODE = 3;  // both edges
    IC_TIMER_REGISTER->DMA_RXIM_bit.CAPCOM0_0 = 1;      // DMA RX request on capture
    IC_TIMER_REGISTER->DMA_TXIM_bit.TMR = 0;
    TMR_SetMode(IC_TIMER_REGISTER, TMR_Mode_Multiple);

    SIU->CNTEN |= SIU_CNTEN_TMR0EN_Msk;
}
