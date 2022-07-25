#include "kernio.h"

#include <stdarg.h>

#include "types.h"
#include "kutils.h"

void putchar( char c ) {
	do {
		asm volatile( "nop" );
	} while( uart0->FR & (1<<5) );
	uart0->DR = c;
}

char getchar( void ) {
	do {
		asm volatile( "nop" );
	} while( uart0->FR & (1<<4) );
	return uart0->DR;
}

void print( const char *s ) {
	while( *s != '\0' ) {
		putchar( *s );
		s++;
	}
}

void kprintf( const char *fmt, ... ) {
	int paramCount = 0;
	for( size_t i = 0; fmt[i] != 0; ++i ) {
		if( fmt[i] == '%' ) paramCount++;
	}

	if( paramCount == 0 ) {
		print( fmt );
		return;
	}

	va_list valist;

	va_start( valist, fmt );
	
	uint32_t specifierFound = 0;
	for( size_t i = 0; fmt[i] != 0; ++i ) {
		if( fmt[i] == '%' && specifierFound == 0 ) {
			specifierFound = 1;
			continue;
		}	
		if( specifierFound ) {
			switch( fmt[i] ) {
				case '%':
				{
					putchar( '%' );
				} break;
				case 'c':
				{
					putchar( (char)va_arg( valist, int ) );
				} break;
				case 's':
				{
					print( (char*)va_arg( valist, size_t ) );
				} break;
				case 'd':
				{
					int val = (int)va_arg( valist, int );
					if( val < 0 ) {
						putchar( '-' );
						val *= -1;
					}
					
					size_t digitCount = 0;
					int reverseVal = 0;
					do {
						reverseVal *= 10;
						reverseVal += val%10;
						val /= 10;
						digitCount++;
					} while( val != 0);

					do {
						putchar( '0' + reverseVal%10);
						reverseVal /= 10;
					} while( --digitCount > 0);
				} break;
				case 'l':
				{
					i++;
					if( fmt[i] == 0 ) return;
					if( fmt[i] == 'd' ) {
						ssize_t val = (ssize_t)va_arg( valist, ssize_t );
						if( val < 0 ) {
							putchar( '-' );
							val *= -1;
						}
						
						size_t digitCount = 0;
						ssize_t reverseVal = 0;
						do {
							reverseVal *= 10;
							reverseVal += val%10;
							val /= 10;
							digitCount++;
						} while( val != 0);

						do {
							putchar( '0' + reverseVal%10);
							reverseVal /= 10;
						} while( --digitCount > 0);
					} else if( fmt[i] == 'u' ) {
						size_t val = (size_t)va_arg( valist, size_t );
						size_t digitCount = 0;
						size_t reverseVal = 0;
						do {
							reverseVal *= 10;
							reverseVal += val%10;
							val /= 10;
							digitCount++;
						} while( val != 0);

						do {
							putchar( '0' + reverseVal%10);
							reverseVal /= 10;
						} while( --digitCount > 0);
					} else if( fmt[i] == 'x' ) {
						size_t val = (size_t)va_arg( valist, size_t );
						size_t reverseVal = 0;
						int numOfHexDigits = 0;
						while( val != 0 ) {
							reverseVal = reverseVal << 4;
							reverseVal += val & 0xF;
							val = val >> 4;
							numOfHexDigits++;
						}

						size_t hexVal = 0;
						do {
							hexVal = reverseVal & 0xF;
							if( hexVal < 0xA ) putchar( '0' + hexVal );
							else putchar( 'A' + (hexVal-0xA) );
							reverseVal = reverseVal >> 4;
						} while( --numOfHexDigits > 0 );
					} else if( fmt[i] == 'o' ) {
						size_t val = (size_t)va_arg( valist, size_t );
						size_t reverseVal = 0;
						int numOfOctalDigits = 0;
						while( val != 0 ) {
							reverseVal = reverseVal << 3;
							reverseVal += val & 0x7;
							val = val >> 3;
							numOfOctalDigits++;
						}

						size_t hexVal = 0;
						do {
							hexVal = reverseVal & 0x7;
							putchar( '0' + hexVal );
							reverseVal = reverseVal >> 3;
						} while( --numOfOctalDigits > 0 );
					} else if( fmt[i] == 'b' ) {
						size_t val = (size_t)va_arg( valist, size_t );
						size_t reverseVal = 0;
						int numOfBinDigits = 0;
						while( val != 0 ) {
							reverseVal = reverseVal << 1;
							reverseVal += val & 0x1;
							val = val >> 1;
							numOfBinDigits++;
						}

						size_t binVal = 0;
						do {
							binVal = reverseVal & 0x1;
							putchar( '0' + binVal );
							reverseVal = reverseVal >> 1;
						} while( --numOfBinDigits > 0 );
					}
				} break;
				case 'u':
				{
					unsigned val = (unsigned)va_arg( valist, unsigned );
					unsigned digitCount = 0;
					unsigned reverseVal = 0;
					do {
						reverseVal *= 10;
						reverseVal += val%10;
						val /= 10;
						digitCount++;
					} while( val != 0);

					do {
						putchar( '0' + reverseVal%10);
						reverseVal /= 10;
					} while( --digitCount > 0);
				} break;
				case 'x':
				{
					unsigned val = (unsigned)va_arg( valist, unsigned );
					unsigned reverseVal = 0;
					int numOfHexDigits = 0;
					while( val != 0 ) {
						reverseVal = reverseVal << 4;
						reverseVal += val & 0xF;
						val = val >> 4;
						numOfHexDigits++;
					}

					unsigned hexVal = 0;
					do {
						hexVal = reverseVal & 0xF;
						if( hexVal < 0xA ) putchar( '0' + hexVal );
						else putchar( 'A' + (hexVal-0xA) );
						reverseVal = reverseVal >> 4;
					} while( --numOfHexDigits > 0 );
				} break;
				case 'o':
				{
					unsigned val = (unsigned)va_arg( valist, unsigned );
					unsigned reverseVal = 0;
					int numOfOctalDigits = 0;
					while( val != 0 ) {
						reverseVal = reverseVal << 3;
						reverseVal += val & 0x7;
						val = val >> 3;
						numOfOctalDigits++;
					}

					unsigned octVal = 0;
					do {
						octVal = reverseVal & 0x7;
						putchar( '0' + octVal );
						reverseVal = reverseVal >> 3;
					} while( --numOfOctalDigits > 0 );
				} break;
				case 'b':
				{
					unsigned val = (unsigned)va_arg( valist, unsigned );
					unsigned reverseVal = 0;
					int numOfBinDigits = 0;
					while( val != 0 ) {
						reverseVal = reverseVal << 1;
						reverseVal += val & 0x1;
						val = val >> 1;
						numOfBinDigits++;
					}

					unsigned binVal = 0;
					do {
						binVal = reverseVal & 0x1;
						putchar( '0' + binVal );
						reverseVal = reverseVal >> 1;
					} while( --numOfBinDigits > 0 );
				} break;
				case 'f':
				{
					print( "(FLOAT ERR: unsupported)" );					
				} break;
				default:
					print( "(ERROR: Unsupported format)" );
			}
		} else {
			putchar( fmt[i] );
		}
		specifierFound = 0;
	}

	va_end( valist );
}

void memdump( void *address, size_t size, bool_t isBigEndian ) {
	uint32_t *dump = (uint32_t*)address;
	for( size_t i = 0; i < size>>2; ++i ) {
		unsigned val = 0;
		if( isBigEndian ) {
			val = big_to_little_endian( dump[i] );
		} else {
			val = dump[i];
		}
		unsigned reverseVal = 0;
		int numOfHexDigits = 0;
		while( numOfHexDigits < 8 ) {
			reverseVal = reverseVal << 4;
			reverseVal += val & 0xF;
			val = val >> 4;
			numOfHexDigits++;
		}

		unsigned hexVal = 0;
		do {
			hexVal = reverseVal & 0xF;
			if( hexVal < 0xA ) putchar( '0' + hexVal );
			else putchar( 'A' + (hexVal-0xA) );
			reverseVal = reverseVal >> 4;
		} while( --numOfHexDigits > 0 );
		putchar( ' ' );
	}
}

