#include "interrupts.h"
#include "uart.h"
#include "types.h"
#include "kutils.h"


void pk_irq_vector_init( void ) {
	size_t vectorPosition = 0;
	asm volatile( "adr %0, _pk_vectors" \
				: "=r" (vectorPosition) );
	WRITE_INTERNAL_REG( vectorPosition, "vbar_el1" );
}

const char *pk_invalidMessage[] = {
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

void pk_show_invalid_entry_message( size_t type, size_t esr, size_t address ) {
	pk_printf( "%s, ESR: 0x%x, address: 0x%x\n", pk_invalidMessage[type], esr, address );
}

void pk_handle_irq( void ) {
	pk_print( "PREKERNEL IRQ\n" );
	/*unsigned int irq = get32(IRQ_PENDING_1);
	switch (irq) {
		case (SYSTEM_TIMER_IRQ_1):
			handle_timer_irq();
			break;
		default:
			printf("Unknown pending irq: %x\r\n", irq);
	}*/
}

void pk_handle_sync( void ) {
	size_t exceptionReg, address;
	asm volatile ( "mrs %0, esr_el1" \
					: "=r" (exceptionReg) );
	asm volatile ( "mrs %0, elr_el1" \
					: "=r" (address) );

	pk_printf( "PREKERNEL SYNC INT: 0x%lx @ 0x%lx\n", exceptionReg, address );
	size_t EC = (exceptionReg & 0xFC000000) >> 26;
	if( EC == 0b100101 ) {
		pk_printf( "HALTING PROCESS\n" );
		for(;;);
	}
    for( ;; );
}
