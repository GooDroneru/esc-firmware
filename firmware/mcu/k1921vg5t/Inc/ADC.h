/*
 * ADC.h
 *
 *  Created on: May 20, 2020
 *      Author: Alka
 */

#include "main.h"
#include "targets.h"
#include "plib035_adc.h"

#ifndef ADC_H_
#define ADC_H_

void ADC_DMA_Callback();
void ADCInit(void);
void startADCConversion();
int16_t getConvertedDegrees(uint16_t adcrawtemp);
#ifdef USE_NTC
int16_t getNTCDegrees(uint16_t ntcrawtemp);
#endif

#endif /* ADC_H_ */
