#include "kutils.h"

uint32_t big_to_little_endian( uint32_t value ) {
	return  ((value>>24)&0xff) | // move byte 3 to byte 0
			((value<<8)&0xff0000) | // move byte 1 to byte 2
			((value>>8)&0xff00) | // move byte 2 to byte 1
			((value<<24)&0xff000000); // byte 0 to byte 3
}

uint64_t big_to_little_endian64( uint64_t value ) {
	uint32_t lsw = big_to_little_endian( value );
	uint32_t msw = big_to_little_endian( value>>31 );
	return (msw << 31) | lsw;
}

uint32_t num_leading_zeros( uint32_t x ) {
    int n;
    if( x == 0 ) return 32;
    n = 1;
    if( (x >> 16) == 0 ) {
        n = n + 16;
        x = x << 16;
    }
    if( (x >> 24) == 0 ) {
        n = n + 8;
        x = x << 8;
    }
    if( (x >> 28) == 0 ) {
        n = n + 4;
        x = x << 4;
    }
    if( (x >> 30) == 0 ) {
        n = n + 2;
        x = x << 2;
    }
    n = n - (x >> 31);
    
    return n;
}

uint32_t num_trailing_zeros( uint32_t num ) {
    uint32_t nlz = num_leading_zeros( ~num & (num-1) );
    return 32-nlz;
}
