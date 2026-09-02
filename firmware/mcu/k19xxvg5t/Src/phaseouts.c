/*
 * phaseouts.c — K1921VG5T, phases on PA8..PA13 (PWM0/1/2 A+B)
 *
 *  Created on: Apr 22, 2020
 *      Author: Alka
 *      Modified by TempersLee June,21 2024
 *      Ported to K1921VG5T: the GPIO CLR registers (ALTFUNCCLR/DATAOUTCLR/
 *      OUTENCLR) are write-1-clear with a single VAL field, and alternate
 *      function outputs additionally require OUTENSET (UM 13.1, table 13.1).
 */
#include "phaseouts.h"

#include "functions.h"
#include "targets.h"
#include "common.h"

extern char prop_brake_active;

/* PA8/PA9 = TIM1_CH1/CH1N (phase A), PA10/PA11 = TIM1_CH2/CH2N (phase B),
 * PA12/PA13 = TIM1_CH3/CH3N (phase C) */
#define PHASE_A_HIGH (1u << 8)
#define PHASE_A_LOW  (1u << 9)
#define PHASE_B_HIGH (1u << 10)
#define PHASE_B_LOW  (1u << 11)
#define PHASE_C_HIGH (1u << 12)
#define PHASE_C_LOW  (1u << 13)

#define ALL_LOW  (PHASE_A_LOW | PHASE_B_LOW | PHASE_C_LOW)
#define ALL_HIGH (PHASE_A_HIGH | PHASE_B_HIGH | PHASE_C_HIGH)

/* plain GPIO output driven low */
#define GPIO_LOW(mask)  do { GPIOA->ALTFUNCCLR = (mask); GPIOA->DATAOUTCLR = (mask); GPIOA->OUTENSET = (mask); } while (0)
/* plain GPIO output driven high */
#define GPIO_HIGH(mask) do { GPIOA->ALTFUNCCLR = (mask); GPIOA->DATAOUTSET = (mask); GPIOA->OUTENSET = (mask); } while (0)
/* high impedance */
#define GPIO_FLOAT(mask) do { GPIOA->ALTFUNCCLR = (mask); GPIOA->DATAOUTCLR = (mask); GPIOA->OUTENCLR = (mask); } while (0)
/* back to PWM alternate function */
#define GPIO_PWM(mask)  do { GPIOA->ALTFUNCSET = (mask); GPIOA->OUTENSET = (mask); } while (0)

void proportionalBrake()
{
	GPIO_LOW(ALL_HIGH);
	GPIO_PWM(ALL_LOW);
}

void phaseBPWM()
{
	if(!eepromBuffer.comp_pwm){  // for future
		GPIO_LOW(PHASE_B_LOW);
	}else{
		GPIO_PWM(PHASE_B_LOW);
	}
	GPIO_PWM(PHASE_B_HIGH);
}

void phaseBFLOAT()
{
	GPIO_FLOAT(PHASE_B_LOW);
	GPIO_FLOAT(PHASE_B_HIGH);
}

void phaseBLOW()
{
	GPIO_LOW(PHASE_B_LOW);
	GPIO_LOW(PHASE_B_HIGH);
}

//////////////////////////////PHASE 2//////////////////////////////////////////////////
void phaseAPWM()
{
	if (!eepromBuffer.comp_pwm)
	{
		GPIO_LOW(PHASE_A_LOW);
	}
	else
	{
		GPIO_PWM(PHASE_A_LOW);
	}
	GPIO_PWM(PHASE_A_HIGH);
}

void phaseAFLOAT()
{
	GPIO_FLOAT(PHASE_A_LOW);
	GPIO_FLOAT(PHASE_A_HIGH);
}

void phaseALOW()
{
	GPIO_LOW(PHASE_A_LOW);
	GPIO_LOW(PHASE_A_HIGH);
}

///////////////////////////////////////////////PHASE 3 /////////////////////////////////////////////////

void phaseCPWM()
{
	if (!eepromBuffer.comp_pwm){
		GPIO_LOW(PHASE_C_LOW);
	}else{
		GPIO_PWM(PHASE_C_LOW);
	}
	GPIO_PWM(PHASE_C_HIGH);
}

void phaseCFLOAT()
{
	GPIO_FLOAT(PHASE_C_LOW);
	GPIO_FLOAT(PHASE_C_HIGH);
}

void phaseCLOW()
{
	GPIO_LOW(PHASE_C_LOW);
	GPIO_LOW(PHASE_C_HIGH);
}

void allOff()
{
	phaseAFLOAT();
	phaseBFLOAT();
	phaseCFLOAT();
}

void comStep(int newStep)
{
    switch (newStep) {
    case 1: // A-B
        phaseAPWM();
        phaseBLOW();
        phaseCFLOAT();
        break;

    case 2: // C-B
        phaseAFLOAT();
        phaseBLOW();
        phaseCPWM();
        break;

    case 3: // C-A
        phaseALOW();
        phaseBFLOAT();
        phaseCPWM();
        break;

    case 4: // B-A
        phaseALOW();
        phaseBPWM();
        phaseCFLOAT();
        break;

    case 5: // B-C
        phaseAFLOAT();
        phaseBPWM();
        phaseCLOW();
        break;

    case 6: // A-C
        phaseAPWM();
        phaseBFLOAT();
        phaseCLOW();
        break;
    }
}

void fullBrake()
{ // full braking shorting all low sides
    phaseALOW();
    phaseBLOW();
    phaseCLOW();
}

void allpwm()
{ // for stepper_sine
    phaseAPWM();
    phaseBPWM();
    phaseCPWM();
}

void twoChannelForward()
{
    phaseAPWM();
    phaseBLOW();
    phaseCPWM();
}

void twoChannelReverse()
{
    phaseALOW();
    phaseBPWM();
    phaseCLOW();
}
