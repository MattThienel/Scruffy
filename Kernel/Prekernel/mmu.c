#include "mmu.h"
#include "uart.h"
#include "pkutils.h"

/*
 *  Virtual Memory Map (QEMU virt)
 *  MMIO        0x0000 0000 0000 0000 to 0xffff ffe0 0000 0000
 *  PREKERNEL   0x0000 0000 4008 0000 to 0xffff fff0 0000 0000
 */
#define VA_START    0xFFFFFFE000000000
#define PA_START    0x0000000040080000

#define TABLE_ADDRESS_MASK      (0x1FFFFFFLL << 12)
#define L1_TABLE_MASK           (0x7FLL)
#define L2_TABLE_MASK           (0x1FFLL)
#define L3_TABLE_MASK           (0x1FFLL)

static uint64_t *nextFreeTable;

extern uint64_t __text_start, __text_end;
extern uint64_t __text_start_phys, __text_end_phys;
extern uint64_t __rodata_start, __rodata_end;
extern uint64_t __rodata_start_phys, __rodata_end_phys;
extern uint64_t __data_start, __data_end;
extern uint64_t __data_start_phys, __data_end_phys;
extern uint64_t __bss_start, __bss_end;
extern uint64_t __bss_start_phys, __bss_end_phys;
extern uint64_t __start, __end;
extern uint64_t __pg_tbl_start, __pg_tbl_end;

void pk_mmu_init( void ) {
    uint64_t *paging = (uint64_t*)&__boot_pg_tbl_start;
    uint64_t reg;

    paging[0] = (uint64_t)(PT_BLOCK | PT_AF | PT_NX | PT_KERNEL | PT_OSH | PT_DEV | PT_RW );
    paging[1] = (uint64_t)( (1LL<<30) | PT_BLOCK | PT_AF | PT_KERNEL | PT_ISH | PT_MEM | PT_RW );

    paging[1*512+0] = (uint64_t)(PT_BLOCK | PT_AF | PT_NX | PT_KERNEL | PT_OSH | PT_DEV | PT_RW );
    //paging[1*512+1] = (uint64_t)((1LL<<30) | PT_BLOCK | PT_AF | PT_KERNEL | PT_ISH | PT_MEM | PT_RW );
    nextFreeTable = paging + (512*2);    
    
    pk_map_section( (uint64_t)&__text_start_phys, (uint64_t)&__text_start, (uint64_t)&__text_end - (uint64_t)&__text_start, (uint64_t)(PT_AF | PT_KERNEL | PT_ISH | PT_MEM | PT_RO) );
    pk_map_section( (uint64_t)&__rodata_start_phys, (uint64_t)&__rodata_start, (uint64_t)&__rodata_end - (uint64_t)&__rodata_start, (uint64_t)(PT_AF | PT_NX | PT_KERNEL | PT_ISH | PT_MEM | PT_RO ) );
    pk_map_section( (uint64_t)&__data_start_phys, (uint64_t)&__data_start, (uint64_t)&__data_end - (uint64_t)&__data_start, (uint64_t)(PT_AF | PT_NX | PT_KERNEL | PT_ISH | PT_MEM | PT_RW) );
    pk_map_section( (uint64_t)&__bss_start_phys, (uint64_t)&__bss_start, (uint64_t)&__bss_end - (uint64_t)&__bss_start, (uint64_t)(PT_AF | PT_NX | PT_KERNEL | PT_ISH | PT_MEM | PT_RW) );

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
    asm volatile( "msr ttbr0_el1, %0" : : "r" ((uint64_t)&__boot_pg_tbl_start + TTBR_CNP) );
    asm volatile( "msr ttbr1_el1, %0" : : "r" ((uint64_t)&__boot_pg_tbl_start + TTBR_CNP + 1*PAGESIZE) );

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

void pk_map_section( uint64_t startPA, uint64_t startVA, int64_t size, uint64_t permissions ) {
    uint64_t va = startVA, pa = startPA;
    int64_t remainingSize = size;
    uint64_t *l2Table, *l3Table;
    volatile uint64_t *paging = ((uint64_t*)&__boot_pg_tbl_start)+512;

    while( remainingSize > 0 ) {
        // Check if section starts and fits 1GB block
        uint64_t l1Index = (va>>30) & L1_TABLE_MASK;
        if( (va & (GIGABYTES(1)-1)) == 0 && remainingSize <= GIGABYTES(1) ) {
            paging[l1Index] = (uint64_t)((pa & (TABLE_ADDRESS_MASK)) | PT_BLOCK | permissions);
            remainingSize -= GIGABYTES(1);
            va += GIGABYTES(1);
            pa += GIGABYTES(1);
            continue;
        }
    
        // Set-up l2 table if non is present
        if( (paging[l1Index] & 0x1) == 0 ) {
            paging[l1Index] = (uint64_t)(((uint64_t)nextFreeTable & TABLE_ADDRESS_MASK) | PT_PAGE);
            nextFreeTable += 512;
        } 

        l2Table = (uint64_t*)((paging[l1Index] & TABLE_ADDRESS_MASK));
        uint64_t l2Index = (va>>21) & L2_TABLE_MASK;
        // Check if section starts and fits 2MB block
        if( (va & (MEGABYTES(2)-1)) == 0 && remainingSize <= MEGABYTES(2) ) {
            l2Table[l2Index] = (uint64_t)((pa & (TABLE_ADDRESS_MASK)) | PT_BLOCK | permissions);
            remainingSize -= MEGABYTES(2);
            va += MEGABYTES(2);
            pa += MEGABYTES(2);
            continue;
        }

        // Set-up l3 table if non is present
        if( (l2Table[l2Index] & 0x1) == 0 ) {
            l2Table[l2Index] = (uint64_t)(((uint64_t)nextFreeTable & TABLE_ADDRESS_MASK) | PT_PAGE);
            nextFreeTable += 512;
        } 

        l3Table = (uint64_t*)((l2Table[(va>>21) & L2_TABLE_MASK] & TABLE_ADDRESS_MASK));
        uint64_t l3Index = (va>>12) & L3_TABLE_MASK;
            
        l3Table[l3Index] = (uint64_t)((pa & (TABLE_ADDRESS_MASK)) | PT_PAGE | permissions);
        remainingSize -= KILOBYTES(4);
        va += KILOBYTES(4);
        pa += KILOBYTES(4);
    }
}
