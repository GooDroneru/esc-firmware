/*
 * serial_telemetry.c — K1921VG5T, UART0 TX on PB9 (AF1)
 *
 *  Created on: May 13, 2020
 *      Author: Alka
 *      modified by TempersLee June 21, 2024
 */

#include "plib5t_uart.h"
#include "serial_telemetry.h"
#include "peripherals.h"
#include "common.h"
#include "kiss_telemetry.h"
#include "functions.h"

UART_Init_TypeDef UART_InitStruct;

void send_telem_DMA(uint8_t bytes)
{
    GPIO_AltFuncCmd(GPIOB, GPIO_Pin_9, ENABLE);
    for(uint8_t i = 0; i < bytes; i++) {
        UART0->DR = aTxBuffer[i];
    }
}

void telem_UART_Init(void)
{
    RCU_AHBClkCmd(RCU_AHBClk_GPIOB, ENABLE);
    RCU_AHBRstCmd(RCU_AHBRst_GPIOB, ENABLE);

    // PB9 -> AF1 = UART0_TX, alternate function push-pull output
    GPIOB->ALTFUNCNUM_bit.PIN9 = 1;
    GPIOB->ALTFUNCSET_bit.PIN9 = 1;
    GPIOB->OUTENSET_bit.PIN9 = 1;

    RCU->CGCFGAPB_bit.UART0EN = 1;
    RCU->RSTDISAPB_bit.UART0EN = 1;
    RCU_UARTClkConfig(UART0_Num, RCU_PeriphClk_PllClk, 0, DISABLE);
    RCU_UARTClkCmd(UART0_Num, ENABLE);
    RCU_UARTRstCmd(UART0_Num, ENABLE);

    UART_StructInit(&UART_InitStruct);
    UART_InitStruct.BaudRate = 115200;
    UART_InitStruct.Tx = ENABLE;
    UART_Init(UART0, &UART_InitStruct);
    UART0->LCRH_bit.FEN = 1;
    UART0->IMSC_bit.TDIM = 1;
    UART_Cmd(UART0, ENABLE);

    PLIC_SetIrqHandler(Plic_Mach_Target, IsrVect_IRQ_UART0_TD, UART0_TD_IRQHandler);
    PLIC_SetMode(IsrVect_IRQ_UART0_TD, PLIC_IRQMODE_HILEVEL);
    PLIC_SetPriority(IsrVect_IRQ_UART0_TD, 1);
    PLIC_IntEnable(Plic_Mach_Target, IsrVect_IRQ_UART0_TD);
}
