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

    ESR esr;
    esr.reg = exceptionReg;
    switch( esr.EC ) {
        case( 0b100001 ):
        {
            kprintf( "\t" );
            int_instr_abort( esr.ISS );
            kprintf( "\n" );
        } break;
        case( 0b100101 ):
        {
            kprintf( "\t" );
            int_data_abort( esr.ISS );
            kprintf( "\n" );
        } break;
        default:
            break;
    }    

	size_t EC = (exceptionReg & 0xFC000000) >> 26;
	if( EC == 0b100101 ) {
		kprintf( "HALTING PROCESS\n" );
		for(;;);
	}
    for( ;; );
}

static char* faultStatus[] = {
    "address size fault, level 0",
    "address size fault, level 1",
    "address size fault, level 2",
    "address size fault, level 3",
    "translation fault, level 0",
    "translation fault, level 1",
    "translation fault, level 2",
    "translation fault, level 3",
    "access flag fault, level 1",
    "access flag fault, level 2",
    "access flag fault, level 3",
    "access flag fault, level 0",
    "permission fault, level 0",
    "permission fault, level 1",
    "permission fault, level 2",
    "permission fault, level 3",
    "synchronous external abort, not on translation table walk",
    "synchronous external abort on translation table walk or hardware update of translation table, level -1",
    "synchronous external abort on translation table walk or hardware update of translation table, level 0",
    "synchronous external abort on translation table walk or hardware update of translation table, level 1",
    "synchronous external abort on translation table walk or hardware update of translation table, level 2",
    "synchronous external abort on translation table walk or hardware update of translation table, level 3",
    "synchronous parity or ECC error on memory access, not on translation table walk",
    "synchronous parity or ECC error on translation table walk or hardware update of translation table, level -1",
    "synchronous parity or ECC error on translation table walk or hardware update of translation table, level 0",
    "synchronous parity or ECC error on translation table walk or hardware update of translation table, level 1",
    "synchronous parity or ECC error on translation table walk or hardware update of translation table, level 2",
    "synchronous parity or ECC error on translation table walk or hardware update of translation table, level 3",
    "address size fault, level -1",
    "translation fault, level -1", 
    "TLB conflict",
    "unsupported atomic hardware update fault",
};

void int_instr_abort( uint64_t iss ) {
    kprintf( "Instruction Fault Error" );
    if( (iss&0x3F) <= 0b110001 ) {
        kprintf(": %s", faultStatus[iss] );    
    }
}

void int_data_abort( uint64_t iss ) {
    kprintf( "Data Fault Error" );
    if( (iss&0x3F) <= 0b110001 ) {
        kprintf(": %s", faultStatus[iss] );
    }
}
