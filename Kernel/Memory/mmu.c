#include "Memory/mmu.h"
#include "kutils.h"

//static uint64_t *nextFreeTable;
#define TABLE_ADDRESS_MASK      (0x1FFFFFFLL << 12)
#define L1_TABLE_MASK           (0x7FLL)
#define L2_TABLE_MASK           (0x1FFLL)
#define L3_TABLE_MASK           (0x1FFLL)

void mmu_init( void ) {
    uint64_t *paging = (uint64_t*)&__pg_tbl_start;
    uint64_t pageTableSize = &__pg_tbl_end - &__pg_tbl_start;
    // clear page table memory
    /*for( uint64_t i = 0; i < pageTableSize; ++i ) {
        *(((uint8_t*)paging)+i) = 0;
    }*/

 //   nextFreeTable = paging+512;
    paging[0] = (uint64_t)(PT_BLOCK | PT_AF | PT_NX | PT_KERNEL | PT_OSH | PT_DEV | PT_RW ); // MMIO
//    paging[1] = (uint64_t)( (((uint64_t)paging+512) & TABLE_ADDRESS_MASK) | PT_PAGE); 
//    nextFreeTable += 512*2;
    paging[1] = (uint64_t)((1LL<<30) | PT_BLOCK | PT_AF | PT_KERNEL | PT_ISH | PT_MEM | PT_RW );
/*
    map_section( (uint64_t)&__text_start_phys, (uint64_t)&__text_start, (uint64_t)&__text_end - (uint64_t)&__text_start, (uint64_t)(PT_AF | PT_KERNEL | PT_ISH | PT_MEM | PT_RO) );
    map_section( (uint64_t)&__rodata_start_phys, (uint64_t)&__rodata_start, (uint64_t)&__rodata_end - (uint64_t)&__rodata_start, (uint64_t)(PT_AF | PT_NX | PT_KERNEL | PT_ISH | PT_MEM | PT_RO ) );
    map_section( (uint64_t)&__data_start_phys, (uint64_t)&__data_start, (uint64_t)&__data_end - (uint64_t)&__data_start, (uint64_t)(PT_AF | PT_NX | PT_KERNEL | PT_ISH | PT_MEM | PT_RW) );
    map_section( (uint64_t)&__bss_start_phys, (uint64_t)&__bss_start, (uint64_t)&__bss_end - (uint64_t)&__bss_start, (uint64_t)(PT_AF | PT_NX | PT_KERNEL | PT_ISH | PT_MEM | PT_RW) );
*/
    asm volatile( "msr ttbr1_el1, %0" : : "r" ((uint64_t)&__pg_tbl_start + TTBR_CNP));
}

void map_section( uint64_t startPA, uint64_t startVA, int64_t size, uint64_t permissions ) {
/*
    uint64_t va = startVA, pa = startPA;
    int64_t remainingSize = size;
    uint64_t *l2Table, *l3Table;
    volatile uint64_t *paging = (uint64_t*)&__pg_tbl_start;

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

        l2Table = (uint64_t*)(VA_START + (paging[l1Index] & TABLE_ADDRESS_MASK));
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

        l3Table = (uint64_t*)(VA_START + (l2Table[(va>>21) & L2_TABLE_MASK] & TABLE_ADDRESS_MASK));
        uint64_t l3Index = (va>>12) & L3_TABLE_MASK;
            
        l3Table[l3Index] = (uint64_t)((pa & (TABLE_ADDRESS_MASK)) | PT_BLOCK | permissions);
        remainingSize -= KILOBYTES(4);
        va += KILOBYTES(4);
        pa += KILOBYTES(4);
    }
*/
}
