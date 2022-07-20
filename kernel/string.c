#include "string.h"

int strcmp( const char *str1, const char *str2 ) {
	size_t i;
	for( i = 0; str1[i] != 0 && str2[i] != 0; ++i ) {
		if( str1[i] != str2[i] ) return (str1[i] - str2[i]);
	}
	return (str1[i] - str2[i]);
}

int strncmp( const char *str1, const char *str2, size_t n ) {
	size_t i;
	for( i = 0; i < n && str1[i] != 0 && str2[i] != 0; ++i ) {
		if( str1[i] != str2[i] ) return (str1[i] - str2[i]);
	}
	return (str1[i] - str2[i]);
}

void* memset( void *mem, char value, size_t n ) {
	char *bytePtr = (char*)mem;
	while( n-- > 0 ) {
		*bytePtr = value;
		bytePtr++;
	}

	return mem;
}

void* memcpy( void *dst, const void *src, size_t n ) {
	char *newDst = (char*)dst;
	char *newSrc = (char*)src;
	while( n-- > 0 ) {
		*newDst = *newSrc;
		newDst++;
		newSrc++;
	}

	return dst;
}
