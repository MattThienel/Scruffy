/*
 *
 */

#ifndef _PREKERNEL_MM_H
#define _PREKERNEL_MM_H

#define VA_PERIPHERALS  0xFFFFFFE000000000
#define VA_KERNEL       0xFFFFFFF000000000

#if defined PINEPHONE
    #define UART_OFFSET 0x01C28000
#else
    #define UART_OFFSET 0x09000000
#endif

#define UART_LOW        UART_OFFSET
#define UART_HIGH       VA_PERIPHERALS + UART_OFFSET

#endif /* _PREKERNEL_MM_H */
