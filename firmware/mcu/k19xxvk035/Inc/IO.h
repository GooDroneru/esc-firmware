/*
 * IO.h
 *
 *  Created on: Sep. 26, 2020
 *      Author: Alka
 */

#ifndef IO_H_
#define IO_H_

#include "main.h"
#include "targets.h"

void changeToOutput();
void changeToInput();
void receiveDshotDma();
void sendDshotDma();

uint8_t getInputPinState();
void setInputPolarityRising();
void setInputPullDown();
void setInputPullUp();
void enableHalfTransferInt();
void setInputPullNone();

/* IC timer period access differs between NIIET MCUs (ECAP PRD vs TMR PERIOD) */
static inline void ic_timer_set_period(uint16_t period)
{
    IC_TIMER_REGISTER->PRD = period;
}

extern char inputSet;
extern char dshot;
extern char servoPwm;
extern char send_telemetry;
extern uint8_t degrees_celsius;
extern uint16_t ADC_raw_volts;

#endif /* IO_H_ */


