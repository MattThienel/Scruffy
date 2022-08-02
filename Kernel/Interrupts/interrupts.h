#ifndef _INTERRUPTS_H
#define _INTERRUPTS_H

#include "types.h"

typedef union {
    uint64_t reg;
    struct {
        uint64_t ISS : 25;
        uint64_t IL : 1;
        uint64_t EC : 6;
        uint64_t ISS2 : 5;
        uint64_t res0 : 27;
    };
} ESR;

void irq_vector_init( void );
void show_invalid_entry_message( size_t type, size_t esr, size_t address );

void handle_irq( void );
void handle_sync( void );

void int_instr_abort( uint64_t iss );
void int_data_abort( uint64_t iss );

#endif /* _INTERRUPTS_H */
