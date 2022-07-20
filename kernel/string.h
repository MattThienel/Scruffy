/*

*/

#ifndef _STRING_H
#define _STRING_H

#include "types.h"

int strcmp( const char *str1, const char *str2 );
int strncmp( const char *str1, const char *str2, size_t n );

void* memset( void *mem, char value, size_t n ); 
void* memcpy( void *dst, const void *src, size_t n );

#endif /* _STRING_H */
