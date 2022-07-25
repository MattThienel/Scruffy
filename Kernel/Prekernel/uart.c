#include "uart.h"
#include "mm.h"

volatile void* uart;

void pk_uart_init( void ) {
    uart = (void *)UART_LOW;
    #if defined PINEPHONE
    pk_putchar = pk_uart_16550_putchar;
    pk_getchar = pk_uart_16550_getchar; 
    #else
    pk_putchar = pk_uart_pl011_putchar;
    pk_getchar = pk_uart_pl011_getchar;
    #endif
}

void pk_uart_map_upper( void ) {
    uart = (void *)UART_HIGH;
}

void pk_uart_pl011_putchar( char c ) {
	do {
		asm volatile( "nop" );
	} while( ((UART_PL011*)uart)->FR & (1<<5) );
	((UART_PL011*)uart)->DR = c;
}

char pk_uart_pl011_getchar( void ) {
	do {
		asm volatile( "nop" );
	} while( ((UART_PL011*)uart)->FR & (1<<4) );
	return ((UART_PL011*)uart)->DR;
}

void pk_uart_16550_putchar( char c ) {
    do {
        asm volatile( "nop" );
    } while( ((UART_16550*)uart)->LSR & (1<<5) );
    ((UART_16550*)uart)->THR = c; 
}

char pk_uart_16550_getchar( void ) {
    do {
        asm volatile( "nop" );
    } while( !(((UART_16550*)uart)->LCR & (1<<0)) );
    return ((UART_16550*)uart)->RBR;
}

void pk_print( const char *s ) {
	while( *s != '\0' ) {
		pk_putchar( *s );
		s++;
	}
}

void pk_printf( const char *fmt, ... ) {
	int paramCount = 0;
	for( size_t i = 0; fmt[i] != 0; ++i ) {
		if( fmt[i] == '%' ) paramCount++;
	}

	if( paramCount == 0 ) {
		pk_print( fmt );
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
					pk_putchar( '%' );
				} break;
				case 'c':
				{
					pk_putchar( (char)va_arg( valist, int ) );
				} break;
				case 's':
				{
					pk_print( (char*)va_arg( valist, size_t ) );
				} break;
				case 'd':
				{
					int val = (int)va_arg( valist, int );
					if( val < 0 ) {
						pk_putchar( '-' );
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
						pk_putchar( '0' + reverseVal%10);
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
							pk_putchar( '-' );
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
							pk_putchar( '0' + reverseVal%10);
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
							pk_putchar( '0' + reverseVal%10);
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
							if( hexVal < 0xA ) pk_putchar( '0' + hexVal );
							else pk_putchar( 'A' + (hexVal-0xA) );
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
							pk_putchar( '0' + hexVal );
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
							pk_putchar( '0' + binVal );
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
						pk_putchar( '0' + reverseVal%10);
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
						if( hexVal < 0xA ) pk_putchar( '0' + hexVal );
						else pk_putchar( 'A' + (hexVal-0xA) );
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
						pk_putchar( '0' + octVal );
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
						pk_putchar( '0' + binVal );
						reverseVal = reverseVal >> 1;
					} while( --numOfBinDigits > 0 );
				} break;
				case 'f':
				{
					pk_print( "(FLOAT ERR: unsupported)" );					
				} break;
				default:
					pk_print( "(ERROR: Unsupported format)" );
			}
		} else {
			pk_putchar( fmt[i] );
		}
		specifierFound = 0;
	}

	va_end( valist );
}
