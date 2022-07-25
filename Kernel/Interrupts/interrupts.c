#include "Interrupts/interrupts.h"
#include "kernio.h"
#include "types.h"
#include "kutils.h"


void irq_vector_init( void ) {
	size_t vectorPosition = 0;
	asm volatile( "adr %0, _vectors" \
				: "=r" (vectorPosition) );
	WRITE_INTERNAL_REG( vectorPosition, "vbar_el1" );
}

const char *invalidMessage[] = {
	"SYNC_INVALID_EL1t",
	"IRQ_INVALID_EL1t",		
	"FIQ_INVALID_EL1t",		
	"ERROR_INVALID_EL1T",		

	"SYNC_INVALID_EL1h",		
	"IRQ_INVALID_EL1h",		
	"FIQ_INVALID_EL1h",		
	"ERROR_INVALID_EL1h",		

	"SYNC_INVALID_EL0_64",		
	"IRQ_INVALID_EL0_64",		
	"FIQ_INVALID_EL0_64",		
	"ERROR_INVALID_EL0_64",	

	"SYNC_INVALID_EL0_32",		
	"IRQ_INVALID_EL0_32",		
	"FIQ_INVALID_EL0_32",		
	"ERROR_INVALID_EL0_32"		
};

void show_invalid_entry_message( size_t type, size_t esr, size_t address ) {
	kprintf( "%s, ESR: 0x%x, address: 0x%x\n", invalidMessage[type], esr, address );
}

void handle_irq( void ) {
	print( "IRQ\n" );
	/*unsigned int irq = get32(IRQ_PENDING_1);
	switch (irq) {
		case (SYSTEM_TIMER_IRQ_1):
			handle_timer_irq();
			break;
		default:
			printf("Unknown pending irq: %x\r\n", irq);
	}*/
}

void handle_sync( void ) {
	size_t exceptionReg, address;
	asm volatile ( "mrs %0, esr_el1" \
					: "=r" (exceptionReg) );
	asm volatile ( "mrs %0, elr_el1" \
					: "=r" (address) );

	kprintf( "Kernel SYNC INT: 0x%lx @ 0x%lx\n", exceptionReg, address-4 );
	size_t EC = (exceptionReg & 0xFC000000) >> 26;
	if( EC == 0b100101 ) {
		kprintf( "HALTING PROCESS\n" );
		for(;;);
	}
    for( ;; );
}
