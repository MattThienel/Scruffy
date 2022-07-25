#include "types.h"
#include "kernio.h"
#include "Interrupts/interrupts.h"
#include "Memory/mmu.h"

void kmain( uint64_t dts, uint64_t x1, uint64_t x2, uint64_t x3 ) {
    uint64_t pg_tbl_addr = 0;
    asm volatile ("msr ttbr0_el1, %0" : : "r" (pg_tbl_addr));
    irq_vector_init(); 

    kprintf( "Testing\n" );
    
    asm volatile ("svc 5");

    for(;;);
}
