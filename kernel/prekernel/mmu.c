#include "mmu.h"
#include "uart.h"

/*
 *  Virtual Memory Map (QEMU virt)
 *  MMIO        0x0000 0000 0000 0000 to 0xffff ffe0 0000 0000
 *  PREKERNEL   0x0000 0000 4008 0000 to 0xffff fff0 0000 0000
 */

void mmu_init( void ) {
    uint64_t *paging = (uint64_t*)&__pg_tbl_start;
    uint64_t reg;

/*
    paging[0] = (uint64_t)((uint8_t *)&__pg_tbl_start+PAGESIZE) |
            PT_PAGE |
            PT_AF   |
            PT_USER |
            PT_ISH  |
            PT_MEM;
  
    paging[1] = (uint64_t)((uint8_t *)&__pg_tbl_start+2*PAGESIZE) |
            PT_PAGE |
            PT_AF   |
            PT_USER |
            PT_ISH  |
            PT_MEM;
    
    for( uint64_t i = 0; i < 512; ++i ) {
        paging[1*512+i] = (uint64_t)((i<<21)) |
            PT_BLOCK |
            PT_AF    |
            PT_NX    |
            PT_USER  |
            PT_OSH   |
            PT_DEV;
    }

    for( uint64_t i = 0; i < 512; ++i ) {
        paging[2*512+i] = (uint64_t)((0x40000000 + (uint64_t)(i<<21))) |
            PT_BLOCK |
            PT_AF    |
            PT_USER  |
            PT_ISH   |
            PT_RW;
    }

      
    paging[3*512] = (uint64_t)((uint8_t *)&__pg_tbl_start+4*PAGESIZE) |
            PT_PAGE |
            PT_AF   |
            PT_USER |
            PT_ISH  |
            PT_MEM;
    
    for( uint64_t i = 0; i < 512; ++i ) {
        paging[4*512+i] = (uint64_t)((i<<21)) |
            PT_BLOCK |
            PT_AF    |
            PT_NX    |
            PT_KERNEL |
            PT_OSH   |
            PT_DEV;
    }
*/

   /* paging[1*512+0] = (uint64_t)(PT_BLOCK | PT_AF | PT_NX | PT_USER | PT_OSH | PT_DEV );
    paging[1*512+1] = (uint64_t)((1LL<<30) | PT_BLOCK | PT_AF | PT_USER | PT_ISH | PT_MEM );

   
    paging[2*512+0] = (uint64_t)((uint8_t *)&__pg_tbl_start+3*PAGESIZE) | 
            PT_PAGE |
            PT_AF   |
            PT_KERNEL |
            PT_ISH  |
            PT_MEM;

    paging[3*512+0] = (uint64_t)(PT_BLOCK | PT_AF | PT_NX | PT_KERNEL | PT_OSH | PT_DEV );
*/

    paging[0] = (uint64_t)(PT_BLOCK | PT_AF | PT_NX | PT_KERNEL | PT_OSH | PT_DEV | PT_RW );
    paging[1] = (uint64_t)( (1LL<<30) | PT_BLOCK | PT_AF | PT_KERNEL | PT_ISH | PT_MEM | PT_RW );

    paging[1*512+0] = (uint64_t)(PT_BLOCK | PT_AF | PT_NX | PT_KERNEL | PT_OSH | PT_DEV | PT_RW );
    paging[1*512+65] = (uint64_t)((1LL<<30) | PT_BLOCK | PT_AF | PT_KERNEL | PT_ISH | PT_MEM | PT_RW );


    // check for 4k granule and at least 36 bits physical address bus
    asm volatile( "mrs %0, id_aa64mmfr0_el1" : "=r" (reg) );
    uint64_t paRange = reg & 0xF;
    if( reg & (0xF << 28) || paRange < 1 ) {
        pk_print( "ERROR: 4k granule or 36 bit address space not supported\n" );
        return;
    }

    // set memory attributes array
    reg = (0xFF << 0 |
           0x04 << 8 |
           0x44 << 16);
    asm volatile( "msr mair_el1, %0" : : "r" (reg) );

    // specify mapping characteristics in translate control register
    reg = (TCR_TBI | (paRange << 32) /*IPS=autodetected*/ | TCR_TG1 | TCR_SH1 | TCR_ORGN1 |
           TCR_IRGN1 | TCR_EPD1 | TCR_T1SZ | TCR_TG0 | TCR_SH0 | TCR_ORGN0 | 
           TCR_IRGN0 | TCR_EPD0 | TCR_T0SZ);
    asm volatile( "msr tcr_el1, %0; isb" : : "r" (reg) );

    // set-up translation table pointer for MMU
    asm volatile( "msr ttbr0_el1, %0" : : "r" ((uint64_t)&__pg_tbl_start + TTBR_CNP) );
    asm volatile( "msr ttbr1_el1, %0" : : "r" ((uint64_t)&__pg_tbl_start + TTBR_CNP + 1*PAGESIZE) );

    // enable mmu
    asm volatile( "dsb ish; isb; mrs %0, sctlr_el1" : "=r" (reg) );
    //reg |= 0xC00800;    // reserved bits
    reg &= ~((1LL<<25) |  // clear EE, little endian translation tables
             (1LL<<24) |  // clear E0E
             (1LL<<19) |  // clear WXN
             (1LL<<12) |  // clear I, no instruction cache
             (1LL<<4)  |  // clear SA0
             (1LL<<3)  |  // clear SA
             (1LL<<2)  |  // clear C, no cache at all
             (1LL<<1));   // clear A, no alignment check
    reg |= (1LL<<0);      // set M, enable MMU
    asm volatile( "msr sctlr_el1, %0; isb" : : "r" (reg) );
}
