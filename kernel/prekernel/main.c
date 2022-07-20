#define QEMU

#include "uart.h"
#include "mmu.h"
#include "types.h"
#include <stddef.h>

extern void kmain( uint64_t x0, uint64_t x1, uint64_t x2, uint64_t x3 );

volatile UART_PL011 *uart_upper = (UART_PL011*)0xFFFFFFE009000000;

void init() {
    pk_uart_init();

    pk_print( "Hello!\n" );
   
    mmu_init();

    pk_uart_map_upper();
    
    pk_print( "High Uart\n" );
    pk_printf( "Uart0 address: 0x%lx\n", (uint64_t)uart );
    
    pk_printf( "Offset of SCH = 0x%lx\n", offsetof(UART_16550, SCH) );
    pk_printf( "Offset of RFL = 0x%lx\n", offsetof(UART_16550, RFL) );
    pk_printf( "Offset of halt = 0x%lx\n", offsetof(UART_16550, HALT) );
    pk_printf( "Sizeof 16550 = 0x%lx\n", sizeof(UART_16550) );

    kmain( 0, 0, 0, 0 );
}
