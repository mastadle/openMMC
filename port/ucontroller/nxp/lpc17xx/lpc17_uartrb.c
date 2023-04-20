/*
 *   openMMC  --
 *
 *   Copyright (C) 2015  Henrique Silva  <henrique.silva@lnls.br>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*!
 * @file lpc17_uart.c
 * @author Henrique Silva <henrique.silva@lnls.br>, LNLS
 * @date June 2016
 *
 * @brief
 */

#include "port.h"

#ifndef UART_RINGBUFFER
#error "lpc17_uartrb.c can only be compiled with the UART_RINGBUFFER flag"
#endif

const lpc_uart_cfg_t usart_cfg[4] = {
    {LPC_UART0, UART0_IRQn, SYSCTL_PCLK_UART0},
    {LPC_UART1, UART1_IRQn, SYSCTL_PCLK_UART1},
    {LPC_UART2, UART2_IRQn, SYSCTL_PCLK_UART2},
    {LPC_UART3, UART3_IRQn, SYSCTL_PCLK_UART3}
};

/* Transmit and receive ring buffer sizes - MUST be power of 2 */
#define UART_SRB_SIZE 256       /* Send */
#define UART_RRB_SIZE 256        /* Receive */

/* Transmit and receive buffers */
static uint8_t rxbuff[UART_RRB_SIZE], txbuff[UART_SRB_SIZE];
RINGBUFF_T rxring, txring;

void UART0_IRQHandler(void)
{
    Chip_UART_IRQRBHandler(LPC_UART0, &rxring, &txring);
}

void UART1_IRQHandler(void)
{
    Chip_UART_IRQRBHandler(LPC_UART1, &rxring, &txring);
}

void UART2_IRQHandler(void)
{
    Chip_UART_IRQRBHandler(LPC_UART2, &rxring, &txring);
}

void UART3_IRQHandler(void)
{
    Chip_UART_IRQRBHandler(LPC_UART3, &rxring, &txring);
}

void uart_init( uint8_t id )
{
    Chip_Clock_SetPCLKDiv( usart_cfg[id].sysclk, SYSCTL_CLKDIV_2 );
    Chip_UART_Init( usart_cfg[id].ptr );

    /* Maximum 921600 baud rate of CP2102 */
    uart_set_baud( UART_DEBUG, 921600 );

    /* read back register Interrupt enable, transmission hold reg INT gets enabled in SendRB */
    uart_int_enable( id, UART_IER_RBRINT);

    RingBuffer_Init(&rxring, rxbuff, 1, UART_RRB_SIZE);
    RingBuffer_Init(&txring, txbuff, 1, UART_SRB_SIZE);

    /* Enable UARTn interruptions */
    NVIC_DisableIRQ( usart_cfg[id].irq );
    NVIC_SetPriority( usart_cfg[id].irq, configMAX_SYSCALL_INTERRUPT_PRIORITY );
    NVIC_EnableIRQ( usart_cfg[id].irq );

    uart_tx_enable(id);
}

int uart_send( uint8_t id, const void * msg, int len) {
    int n_chars_sent = 0;
    while (n_chars_sent < len) {
        n_chars_sent += Chip_UART_SendRB(usart_cfg[id].ptr, &txring, (uint8_t *) msg + n_chars_sent, len - n_chars_sent);
    }
    return n_chars_sent;
}

int uart_read_blocking( uint8_t id, void * buf, int len)
{
    int n_chars_read = 0;
    LPC_USART_T *pUART = usart_cfg[id].ptr;
    while (len) {
        taskENTER_CRITICAL();
        Chip_UART_RXIntHandlerRB(pUART, &rxring);
        taskEXIT_CRITICAL();
        int number_of_pops = RingBuffer_GetCount(&rxring);
        if (len < number_of_pops) number_of_pops = len;
        if (number_of_pops)	{
            number_of_pops = RingBuffer_PopMult(&rxring, (uint8_t *) buf + n_chars_read, number_of_pops);
            len -= number_of_pops;
            n_chars_read += number_of_pops;
        }
    }
    return n_chars_read;
}
