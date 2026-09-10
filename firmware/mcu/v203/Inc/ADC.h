/*
 * ADC.h
 *
 *  Created on: May 20, 2020
 *      Author: Alka
 */

#include "main.h"
#include "targets.h"

#ifndef ADC_H_
#define ADC_H_

void ADC_DMA_Callback();
void ADCInit(void);
void startADCConversion();
int16_t getConvertedDegrees(uint16_t adcrawtemp);
int16_t getNTCDegrees(uint16_t ntcrawtemp);

#ifdef USE_PA12_ANALOG_MUX
extern volatile uint8_t admux_temp_slot; // 1 while the mux slot measured NTC
#endif

#endif /* ADC_H_ */
