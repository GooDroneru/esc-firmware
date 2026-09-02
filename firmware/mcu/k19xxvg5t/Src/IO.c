/*
 * IO.c — signal input/output on PA14 (TMR0_IO) for K1921VG5T
 *
 * RX: TMR0 CAPCOM captures both edges into CAPCOM_VAL0, DMA channel 0 moves
 *     timestamps into dma_buffer[], completion IRQ -> transfercomplete().
 * TX: TMR0 CAPCOM in compare mode generates the GCR waveform on PA14
 *     (OUTMODE=7: reset at VAL0=0, set at VAL1), DMA channel 1 feeds the next
 *     pulse width into CAPCOM_VAL1 on every period event.
 *
 *  Created on: Sep. 26, 2020
 *      Author: Alka
 */

#include "IO.h"

#include "common.h"
#include "dshot.h"
#include "functions.h"
#include "serial_telemetry.h"
#include "targets.h"
#include "peripherals.h"

uint8_t buffer_padding = 7;
char ic_timer_prescaler = CPU_FREQUENCY_MHZ / 5;
uint32_t dma_buffer[64] __attribute__((aligned(4)));

char out_put = 0;
extern uint16_t counter;
extern uint16_t halfpulsetime;
extern char fallingEdgeTrigger;
extern uint16_t periodTime;
extern uint16_t bitShift;

__RAMFUNC void changeToInput()
{
    // stop output path
    TMR0->DMA_TXIM_bit.TMR = 0;
    TMR_SetMode(IC_TIMER_REGISTER, TMR_Mode_Stop);

    // capture mode: both edges of TMR0_IO (PA14) -> CAPCOM[0].VAL0 -> DMA ch0
    IC_TIMER_REGISTER->CAPCOM[0].CTRL_bit.CAP = 1;
    IC_TIMER_REGISTER->CAPCOM[0].CTRL_bit.CCISEL = 0;   // CCIA = TMR0_IO (SIU->TMRMUX)
    IC_TIMER_REGISTER->CAPCOM[0].CTRL_bit.CAPMODE = 3;  // both edges
    IC_TIMER_REGISTER->CAPCOM[0].CTRL_bit.OUTMODE = 0;
    IC_TIMER_REGISTER->COUNT = 0;

    updateDma();

    IC_TIMER_REGISTER->DMA_RXIM_bit.CAPCOM0_0 = 1;   // DMA request per capture

    // PA14 = alternate function input
    GPIOA->ALTFUNCSET_bit.PIN14 = 1;
    GPIOA->OUTENCLR = GPIO_OUTENSET_PIN14_Msk;

    TMR_SetMode(IC_TIMER_REGISTER, TMR_Mode_Multiple);
}

__RAMFUNC void receiveDshotDma()
{
    out_put = 0;
    changeToInput();
}

__RAMFUNC void changeToOutput()
{
    // compare mode: TMR0 period = periodTime, output set at VAL1 (gcr pulse)
    TMR0->DMA_RXIM_bit.CAPCOM0_0 = 0;
    TMR_SetMode(IC_TIMER_REGISTER, TMR_Mode_Stop);

    IC_TIMER_REGISTER->CAPCOM[0].CTRL_bit.CAP = 0;
    IC_TIMER_REGISTER->CAPCOM[0].CTRL_bit.OUTMODE = 7;  // reset @ VAL0, set @ VAL1
    IC_TIMER_REGISTER->CAPCOM[0].VAL0 = 0;
    IC_TIMER_REGISTER->CAPCOM[0].VAL1 = 0;
    IC_TIMER_REGISTER->COUNT = 0;
    IC_TIMER_REGISTER->PERIOD = periodTime;

    updateDmaTransmit();

    IC_TIMER_REGISTER->DMA_TXIM_bit.TMR = 1;         // DMA request per period

    // PA14 = alternate function output
    GPIOA->ALTFUNCSET_bit.PIN14 = 1;
    GPIOA->OUTENSET_bit.PIN14 = 1;

    TMR_SetMode(IC_TIMER_REGISTER, TMR_Mode_Capcom_Up);
}

__RAMFUNC void sendDshotDma()
{
    out_put = 1;
    changeToOutput();
}

uint8_t getInputPinState()
{
    return SET == GPIO_ReadBit(INPUT_PIN_PORT, INPUT_PIN);
}

void setInputPolarityRising()
{
}

void setInputPullDown()
{
}

void setInputPullUp()
{
}

void enableHalfTransferInt()
{
}
