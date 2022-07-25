/*
	Handles Kernel Input/Output
*/

#ifndef _KERNIO_H
#define _KERNIO_H

#include <stdarg.h>
#include "types.h"

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
} UART;
/*
static volatile UART *uart0 = (UART *)0x0000000009000000;
*/
static volatile UART *uart0 = (UART *)0xFFFFFFE009000000;


void putchar( char c );
char getchar( void );
void print( const char *s );
void kprintf( const char *fmt, ... );
void memdump( void *address, size_t size, bool_t isBigEndian );

#endif /* _KERNIO_H */
