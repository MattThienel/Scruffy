#ifndef _INTERRUPTS_H
#define _INTERRUPTS_H

#include "types.h"

void pk_irq_vector_init( void );
void pk_show_invalid_entry_message( size_t type, size_t esr, size_t address );

void pk_handle_irq( void );
void pk_handle_sync( void );

#endif /* _INTERRUPTS_H */
