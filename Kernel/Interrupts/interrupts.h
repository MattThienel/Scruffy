#ifndef _INTERRUPTS_H
#define _INTERRUPTS_H

#include "types.h"

void irq_vector_init( void );
void show_invalid_entry_message( size_t type, size_t esr, size_t address );

void handle_irq( void );
void handle_sync( void );

#endif /* _INTERRUPTS_H */
