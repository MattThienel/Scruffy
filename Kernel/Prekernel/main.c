#define QEMU

#include "uart.h"
#include "mmu.h"
#include "types.h"
#include <stddef.h>

extern uint64_t __stack_top;

extern void kmain( uint64_t x0, uint64_t x1, uint64_t x2, uint64_t x3 );

volatile UART_PL011 *uart_upper = (UART_PL011*)0xFFFFFFE009000000;

void init( uint64_t dts, uint64_t x1, uint64_t x2, uint64_t x3 ) {
    pk_uart_init();

    pk_print( "Hello!\n" );
   
    pk_mmu_init();

    pk_uart_map_upper();
    
    pk_print( "High Uart\n" );
    pk_printf( "Uart0 address: 0x%lx\n", (uint64_t)uart );
    
    uint64_t kernelStack = (uint64_t)&__stack_top;
    asm volatile( "mov sp, %0" : : "r" (kernelStack) );

//    kmain( 0,0,0,0 );
    kmain( dts, x1, x2, x3 );
    for( ;; );
}
