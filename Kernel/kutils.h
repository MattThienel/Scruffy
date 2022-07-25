#ifndef _KUTILS_H
#define _KUTILS_H

#include "types.h"

#define KILOBYTES(x) (1024L*x)
#define MEGABYTES(x) (1024L*KILOBYTES(x))
#define GIGABYTES(x) (1024L*MEGABYTES(x))

uint32_t big_to_little_endian( uint32_t value );
uint64_t big_to_little_endian64( uint64_t value );

#define READ_INTERNAL_REG( var, reg ) \
	asm volatile( "mrs %0," reg \
					: "=r" (var) );

#define WRITE_INTERNAL_REG( var, reg ) \
	asm volatile( "msr " reg ", %0" \
					: "=r" (var) );

#endif /* _KUTILS_H */
