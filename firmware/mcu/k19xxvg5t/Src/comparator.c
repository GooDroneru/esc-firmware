/*
 * comparator.c — BEMF comparator inputs on GPIOB (PB4=A, PB5=B, PB6=C)
 *
 *  Created on: Sep. 26, 2020
 *      Author: Alka
 *      Modified by TempersLee June 21, 2024
 *      Ported to K1921VG5T: INTENCLR/INTPOLCLR are write-1-clear registers
 *      with a single VAL field (no per-pin bitfields).
 */

#include "comparator.h"

#include "targets.h"

#define COMP_A_MASK GPIO_Pin_4
#define COMP_B_MASK GPIO_Pin_5
#define COMP_C_MASK GPIO_Pin_6

uint8_t getCompOutputLevel(){
    if (step == 1 || step == 4)
    {   // c floating
        return SET == GPIO_ReadBit(COMPARATOR_REGISTER, PHASE_C_COMP_PIN);
    }
    if (step == 2 || step == 5)
    {   // a floating
        return SET == GPIO_ReadBit(COMPARATOR_REGISTER, PHASE_A_COMP_PIN);
    }
    if (step == 3 || step == 6)
    {   // b floating
        return SET == GPIO_ReadBit(COMPARATOR_REGISTER, PHASE_B_COMP_PIN);
    }
    return 0;
}


void maskPhaseInterrupts()
{
    COMPARATOR_REGISTER->INTENCLR = (COMP_A_MASK | COMP_B_MASK | COMP_C_MASK);
    COMPARATOR_REGISTER->INTSTATUS = (COMP_A_MASK | COMP_B_MASK | COMP_C_MASK);
}

void enableCompInterrupts()      //enable
{
    COMPARATOR_REGISTER->INTENSET_bit.PHASE_A_COMP = 1;
    COMPARATOR_REGISTER->INTENSET_bit.PHASE_B_COMP = 1;
    COMPARATOR_REGISTER->INTENSET_bit.PHASE_C_COMP = 1;
}
void changeCompInput()
{
    if (step == 1 || step == 4)
    {   // c floating
        COMPARATOR_REGISTER->INTENCLR = (COMP_A_MASK | COMP_B_MASK);
        COMPARATOR_REGISTER->INTENSET_bit.PHASE_C_COMP = 1;
    }
    if (step == 2 || step == 5)
    {   // a floating
        COMPARATOR_REGISTER->INTENCLR = (COMP_B_MASK | COMP_C_MASK);
        COMPARATOR_REGISTER->INTENSET_bit.PHASE_A_COMP = 1;
    }
    if (step == 3 || step == 6)
    {   // b floating
        COMPARATOR_REGISTER->INTENCLR = (COMP_A_MASK | COMP_C_MASK);
        COMPARATOR_REGISTER->INTENSET_bit.PHASE_B_COMP = 1;
    }

    if (rising)
    {
        COMPARATOR_REGISTER->INTPOLCLR = (COMP_A_MASK | COMP_B_MASK | COMP_C_MASK);
    }
    else
    {
        COMPARATOR_REGISTER->INTPOLSET_bit.PHASE_A_COMP = 1;
        COMPARATOR_REGISTER->INTPOLSET_bit.PHASE_B_COMP = 1;
        COMPARATOR_REGISTER->INTPOLSET_bit.PHASE_C_COMP = 1;
    }
}
