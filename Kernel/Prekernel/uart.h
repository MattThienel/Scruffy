/*
 *
 */

#ifndef _PREKERNEL_UART_H
#define _PREKERNEL_UART_H

#include "mm.h"
#include "types.h"
#include <stdarg.h>

typedef struct {
	uint32_t DR;
	union {
		uint32_t RSR;
		uint32_t ECR;
	};
	uint32_t reserved1[4];
	uint32_t FR;
	uint32_t reserved2;
	uint32_t ILPR;
	uint32_t IBRD;
	uint32_t FBRD;
	uint32_t LCR_H;
	uint32_t CR;
	uint32_t IFLS;
	uint32_t IMSC;
	uint32_t RIS;
	uint32_t MIS;
	uint32_t ICR;
	uint32_t DMACR;
	uint32_t reserved3[997];
	uint32_t PeriphID0;
	uint32_t PeriphID1;
	uint32_t PeriphID2;
	uint32_t PeriphID3;
	uint32_t PCellID0;
	uint32_t PCellID1;
	uint32_t PCellID2;
	uint32_t PCellID3;
} UART_PL011;

typedef struct {
    union {
        uint32_t RBR;
        uint32_t THR;
        uint32_t DLL;
    };
    union {
        uint32_t DLH;
        uint32_t IER;
    };
    union {
        uint32_t IIR;
        uint32_t FCR;
    };
    uint32_t LCR;
    uint32_t MCR;
    uint32_t LSR;
    uint32_t MSR;
    uint32_t SCH;
    uint32_t reserved0[23];
    uint32_t USR;
    uint32_t TFL;
    uint32_t RFL;
    uint32_t reserved1[7];
    uint32_t HALT;
} UART_16550;


extern volatile void* uart;

static void (*pk_putchar)( char c );
static char (*pk_getchar)( void );
void pk_print( const char *s );
void pk_printf( const char *fmt, ... );

void pk_uart_init( void );
void pk_uart_map_upper( void );

void pk_uart_pl011_putchar( char c );
char pk_uart_pl011_getchar( void );

void pk_uart_16550_putchar( char c );
char pk_uart_16550_getchar( void );

#endif /* _PREKERNEL_UART_H */
