#include "types.h"
#include "kutils.h"
#include "kernio.h"
#include "Interrupts/interrupts.h"
#include "Memory/mmu.h"

#include "Prekernel/uart.h"

#define MEMORY_AVAILABLE GIGABYTES(4)

void kmain( uint64_t dts, uint64_t x1, uint64_t x2, uint64_t x3 ) {
    irq_vector_init(); 
    mmu_init( MEMORY_AVAILABLE );

    kprintf( "Testing\n" );
    asm volatile ("svc 4");
    
    for(;;);
}
