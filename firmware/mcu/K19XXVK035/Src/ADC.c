/*
 * ADC.c
 *
 *  Created on: May 20, 2020
 *      Author: Alka
 *      Modified by TempersLee June 21, 2024
 */
#include "ADC.h"
#include "ntc_tables.h"
#include "functions.h"
#ifdef USE_ADC
#ifdef USE_ADC_INPUT
uint16_t ADCDataDMA[4];
#else
uint16_t ADCDataDMA[3];
#endif

extern uint16_t ADC_raw_temp;
extern uint16_t ADC_raw_volts;
extern uint16_t ADC_raw_current;
extern uint16_t ADC_raw_input;
extern uint16_t ADC_raw_ntc;

void ADC_DMA_Callback()
{  // read dma buffer and set extern variables

#ifdef USE_ADC_INPUT
    ADC_raw_temp =    ADCDataDMA[3];
    ADC_raw_volts = ADCDataDMA[1] / 2;
    ADC_raw_current = ADCDataDMA[2];
    ADC_raw_input = ADCDataDMA[0];

#else
    ADC_raw_temp =    ADCDataDMA[2];
#ifdef PA6_VOLTAGE
    ADC_raw_volts = ADCDataDMA[1]; 
    ADC_raw_ntc = ADCDataDMA[0];
#else
    ADC_raw_volts = ADCDataDMA[0];
    ADC_raw_current = ADCDataDMA[1];
    ADC_raw_ntc = ADCDataDMA[2];
#endif
#endif
}

#ifdef USE_NTC
static int16_t getNTCDegrees(uint16_t ntcrawtemp) {
    int p1 = NTC_table[ ntcrawtemp >> 6     ];
    int p2 = NTC_table[(ntcrawtemp >> 6) + 1];
    // linear interpolation within 64-step bucket, result in 0.1 °C
    return (int16_t)(p1 - ((p1 - p2) * (ntcrawtemp & 0x3F)) / 64);
}
#endif

int16_t getConvertedDegrees(uint16_t adcrawtemp) {
    (void)adcrawtemp;
#ifdef USE_NTC
    return getNTCDegrees(ADC_raw_ntc);
#else
    return 0;
#endif
}

//trigger ADC
void startADCConversion()
{
    ADC_SEQ_ITConfig(ADC_SEQ_Num_0, 1, DISABLE);
    ADC_SEQ_ITCmd(ADC_SEQ_Num_0, ENABLE);
    NVIC_EnableIRQ(ADC_SEQ0_IRQn);
    NVIC_SetPriority(ADC_SEQ0_IRQn, 6);
}

void ADCInit(void)
{
    RCU->ADCCFG_bit.CLKSEL = RCU_ADCCFG_CLKSEL_PLLCLK;
    RCU->ADCCFG_bit.DIVN = 0x1; // div4
    RCU->ADCCFG_bit.DIVEN = 0x1;
    RCU->ADCCFG_bit.CLKEN = 0x1;
    RCU->ADCCFG_bit.RSTDIS = 0x1;

    ADC->ACTL_bit.ADCEN = 0x1;
    ADC->EMUX_bit.EM0 = ADC_EMUX_EM0_PWM012A | ADC_EMUX_EM0_SwReq;
    //ADC->SEQSYNC = ADC_SEQSYNC_SYNC0_Msk;
    ADC->SEQ[0].SCCTL_bit.ICNT = 2;
    ADC->SEQ[0].SRQCTL_bit.RQMAX = 0x2;
    ADC->SEQ[0].SRQCTL_bit.QAVGVAL = ADC_SEQ_SRQCTL_QAVGVAL_Average64;
    ADC->SEQ[0].SRQCTL_bit.QAVGEN = 0;
    ADC->SEQ[0].SRQSEL_bit.RQ0 = 0x0;
    ADC->SEQ[0].SRQSEL_bit.RQ1 = 0x1;
    ADC->SEQ[0].SRQSEL_bit.RQ2 = 0x2;
    ADC_SEQ_SetRestartTimer(ADC_SEQ_Num_0, 50);
    //ADC->SEQ[0].SRQSEL_bit.RQ3 = 0x3;
    ADC->SEQEN_bit.SEQEN0 = 1;
    while (!ADC->ACTL_bit.ADCRDY) {
    };
    ADC->IM_bit.SEQIM0 = 1;
    NVIC_EnableIRQ(ADC_SEQ0_IRQn);
    NVIC_SetPriority(ADC_SEQ0_IRQn, 6);
}

#endif // USE_ADC
