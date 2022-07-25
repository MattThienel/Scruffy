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
