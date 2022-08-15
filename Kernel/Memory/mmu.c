#include "Memory/mmu.h"
#include "kernio.h"
#include "kutils.h"
#include "string.h"

#define TABLE_ADDRESS_MASK      (0x1FFFFFFLL << 12)
#define L1_TABLE_MASK           (0x7FLL)
#define L2_TABLE_MASK           (0x1FFLL)
#define L3_TABLE_MASK           (0x1FFLL)

static uint64_t *nextFreeTable;
static pmm_bitmap_t physicalMemBitMap;
static size_t *startAvailableMem, *endAvailableMem;

extern uint64_t __boot_text_start;

void mmu_init( size_t memSize ) {
    uint64_t *paging = (uint64_t*)((uint64_t)&__pg_tbl_start);
    uint64_t reg;
    memset( (void*)((uint64_t)&__pg_tbl_start-VA_START), 0, (uint64_t)&__pg_tbl_end-(uint64_t)&__pg_tbl_start );

    paging[0] = (uint64_t)(PT_BLOCK | PT_AF | PT_NX | PT_KERNEL | PT_OSH | PT_DEV | PT_RW );

    nextFreeTable = paging + (512*1);    
    
    map_pg_tbl( (uint64_t)&__text_start_phys, (uint64_t)&__text_start, (uint64_t)&__text_end - (uint64_t)&__text_start, (uint64_t)(PT_AF | PT_KERNEL | PT_ISH | PT_MEM | PT_RO) );
    map_pg_tbl( (uint64_t)&__rodata_start_phys, (uint64_t)&__rodata_start, (uint64_t)&__rodata_end - (uint64_t)&__rodata_start, (uint64_t)(PT_AF | PT_NX | PT_KERNEL | PT_ISH | PT_MEM | PT_RO ) );
    map_pg_tbl( (uint64_t)&__data_start_phys, (uint64_t)&__data_start, (uint64_t)&__data_end - (uint64_t)&__data_start, (uint64_t)(PT_AF | PT_NX | PT_KERNEL | PT_ISH | PT_MEM | PT_RW) );
    map_pg_tbl( (uint64_t)&__bss_start_phys, (uint64_t)&__bss_start, (uint64_t)&__bss_end - (uint64_t)&__bss_start, (uint64_t)(PT_AF | PT_NX | PT_KERNEL | PT_ISH | PT_MEM | PT_RW) );

    // set-up translation table pointer for MMU
    asm volatile( "msr ttbr0_el1, %0" : : "r" (0) ); // Clear user-space page table
    asm volatile( "msr ttbr1_el1, %0" : : "r" ((uint64_t)(((uint64_t)&__pg_tbl_start) - VA_START) + TTBR_CNP) );
    asm volatile( "tlbi vmalle1is; dsb ish; isb" ); // Clear cache

    /*
    NOTE(matt): Memory Bit Map maps 4K pages as individual bits, (ie. 1 byte = 8 (4k) pages)
    */
    physicalMemBitMap.bitmap = (uint8_t*)((uint64_t)&__end);
    physicalMemBitMap.size = memSize/(4096); // Number of bytes to map entire physical memory space 
    physicalMemBitMap.indexOfFirstFreePage = 0;
    map_pg_tbl( (uint64_t)physicalMemBitMap.bitmap-VA_START, (uint64_t)physicalMemBitMap.bitmap, physicalMemBitMap.size, (uint64_t)(PT_AF | PT_NX | PT_KERNEL | PT_ISH | PT_MEM | PT_RW) );
    memset( physicalMemBitMap.bitmap, 0, physicalMemBitMap.size ); 

    // Mark kernel and io addresses as used for the memory bit map
    size_t returnAddr;
    if( alloc_physical( ((uint64_t)&__end - VA_START + physicalMemBitMap.size)/4096, &returnAddr ) != PMM_OK ) {
        kprintf( "PMM ERROR: Could not allocate memory for io and kernel in physical memory bitmap\n" );
    }
    if( returnAddr != 0 ) {
        kprintf( "PMM ERROR: IO and Kernel not allocated at beginning of memory\n" );
    }
    kprintf( "Kernel Memory: 0x%lx\n", returnAddr );

    uint64_t addr;
    if( alloc_physical( 4, &addr ) != PMM_OK ) {
        kprintf( "PMM ERROR: Could not allocate memory\n" );
    }
    kprintf( "Memory after kernel: 0x%lx\n", addr );

    addr = 0;
    if( alloc_physical( 1, &addr ) != PMM_OK ) {
        kprintf( "PMM ERROR: Could not allocate memory\n" );
    }
    kprintf( "Memory after kernel: 0x%lx\n", addr );

}

/*
    return: error code & the physical address of the start of memory
*/
// TODO(matt): Implement buddy allocator
size_t alloc_physical( size_t numOfPages, size_t *physicalAddr ) {
    //physicalMemBitMap;
    size_t numOfAvailablePages = 0;
    size_t index = 0, startBit = 0;

    uint32_t *bitmap = (uint32_t *)physicalMemBitMap.bitmap;
    size_t size = physicalMemBitMap.size/4;    
    size_t i;
    ssize_t bitIndex = -1;
    uint32_t pages = 0, enoughMemoryAvailable = 0;

    // Find available memory
    for( i = physicalMemBitMap.indexOfFirstFreePage/4; i < size; ++i ) {
        pages = num_trailing_zeros( bitmap[i] );
        if( pages >= numOfPages ) {
            enoughMemoryAvailable = 1;
            index = i;
            bitIndex = 0;
            break;
        }
        
        pages = num_leading_zeros( bitmap[i] );
        if( pages > 0 ) {
            index = i;
            bitIndex = 32-pages;
            numOfAvailablePages += pages;
            for( ; i < size; ++i ) {
                pages = num_leading_zeros( bitmap[i] );
                numOfAvailablePages += pages;
                if( numOfAvailablePages >= numOfPages ) {
                    enoughMemoryAvailable = 1;
                    break;
                }
            }
        }

        if( numOfAvailablePages >= numOfPages ) {
            enoughMemoryAvailable = 1;
            break;
        }
    }

    // Handle if no memory is available
    if( !enoughMemoryAvailable ) {
        return PMM_MEM_NOT_AVAILABLE;
    }   

    // Convert index to memory address
    *physicalAddr = (bitIndex*4096) + (index*4096*32);
 
    // Mark memory as allocated
    uint32_t stopIndex = (bitIndex + numOfPages);
    if( stopIndex > 32 ) stopIndex = 32;
    if( bitIndex != 0 ) {
        uint64_t bits = (1 << (stopIndex));
        bits -= 1;
        bits &= ~(1 << bitIndex);
        bits += 1; 
        bitmap[index] |= bits;
        index++;
    }
    for( ; index < (numOfPages)/32; ++index  ) {
        bitmap[index] = 0xFFFFFFFF;
    }
    stopIndex = (numOfPages-(stopIndex-bitIndex))%32;
    uint64_t bits = (1 << stopIndex);
    bits -= 1;
    bitmap[index] |= bits;
    
    return PMM_OK;
}

void map_pg_tbl( uint64_t startPA, uint64_t startVA, int64_t size, uint64_t permissions ) {

    uint64_t va = startVA, pa = startPA;
    int64_t remainingSize = size;
    uint64_t *l2Table, *l3Table;
    volatile uint64_t *paging = ((uint64_t*)(uint64_t)&__pg_tbl_start);

    while( remainingSize > 0 ) {
        // Check if section starts and fits 1GB block
        uint64_t l1Index = (va>>30) & L1_TABLE_MASK;
        if( (va & (GIGABYTES(1)-1)) == 0 && remainingSize >= GIGABYTES(1) ) {
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

        l2Table = (uint64_t*)(((paging[l1Index] & TABLE_ADDRESS_MASK)) + VA_START);
        uint64_t l2Index = (va>>21) & L2_TABLE_MASK;
        // Check if section starts and fits 2MB block
        if( (va & (MEGABYTES(2)-1)) == 0 && remainingSize >= MEGABYTES(2) ) {
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

        l3Table = (uint64_t*)(((l2Table[(va>>21) & L2_TABLE_MASK] & TABLE_ADDRESS_MASK)) + VA_START);
        uint64_t l3Index = (va>>12) & L3_TABLE_MASK;
            
        l3Table[l3Index] = (uint64_t)((pa & (TABLE_ADDRESS_MASK)) | PT_PAGE | permissions);
        remainingSize -= KILOBYTES(4);
        va += KILOBYTES(4);
        pa += KILOBYTES(4);
    }

}
