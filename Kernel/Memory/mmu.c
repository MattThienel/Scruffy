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
    physicalMemBitMap.indexOfFirstFreePage = 0;

    physicalMemBitMap.buddySizes[0] = memSize/(4096*(8<<0));
    physicalMemBitMap.buddySizes[1] = memSize/(4096*(8<<1));
    physicalMemBitMap.buddySizes[2] = memSize/(4096*(8<<2));
    physicalMemBitMap.buddySizes[3] = memSize/(4096*(8<<3));
    physicalMemBitMap.buddySizes[4] = memSize/(4096*(8<<4));
    physicalMemBitMap.buddySizes[5] = memSize/(4096*(8<<5));
    physicalMemBitMap.buddySizes[6] = memSize/(4096*(8<<6));
    physicalMemBitMap.buddySizes[7] = memSize/(4096*(8<<7));
    physicalMemBitMap.buddySizes[8] = memSize/(4096*(8<<8));
    physicalMemBitMap.buddySizes[9] = memSize/(4096*(8<<9));
    physicalMemBitMap.size = physicalMemBitMap.buddySizes[0] + physicalMemBitMap.buddySizes[1] + physicalMemBitMap.buddySizes[2] + physicalMemBitMap.buddySizes[3] + physicalMemBitMap.buddySizes[4] + physicalMemBitMap.buddySizes[5] + physicalMemBitMap.buddySizes[6] + physicalMemBitMap.buddySizes[7] + physicalMemBitMap.buddySizes[8] + (physicalMemBitMap.buddySizes[9]*2);

    physicalMemBitMap.buddies[0] = (uint8_t*)((uint64_t)&__end);
    physicalMemBitMap.buddies[1] = (uint8_t*)((uint64_t)physicalMemBitMap.buddies[0] + physicalMemBitMap.buddySizes[0]);

    physicalMemBitMap.buddies[2] = (uint8_t*)((uint64_t)physicalMemBitMap.buddies[1] + physicalMemBitMap.buddySizes[1]);
    physicalMemBitMap.buddies[3] = (uint8_t*)((uint64_t)physicalMemBitMap.buddies[2] + physicalMemBitMap.buddySizes[2]);
    physicalMemBitMap.buddies[4] = (uint8_t*)((uint64_t)physicalMemBitMap.buddies[3] + physicalMemBitMap.buddySizes[3]);
    physicalMemBitMap.buddies[5] = (uint8_t*)((uint64_t)physicalMemBitMap.buddies[4] + physicalMemBitMap.buddySizes[4]);
    physicalMemBitMap.buddies[6] = (uint8_t*)((uint64_t)physicalMemBitMap.buddies[5] + physicalMemBitMap.buddySizes[5]);
    physicalMemBitMap.buddies[7] = (uint8_t*)((uint64_t)physicalMemBitMap.buddies[6] + physicalMemBitMap.buddySizes[6]);
    physicalMemBitMap.buddies[8] = (uint8_t*)((uint64_t)physicalMemBitMap.buddies[7] + physicalMemBitMap.buddySizes[7]);
    physicalMemBitMap.buddies[9] = (uint8_t*)((uint64_t)physicalMemBitMap.buddies[8] + physicalMemBitMap.buddySizes[8]);
    physicalMemBitMap.allocated2M= (uint8_t*)((uint64_t)physicalMemBitMap.buddies[9] + physicalMemBitMap.buddySizes[9]);


    map_pg_tbl( (uint64_t)physicalMemBitMap.bitmap-VA_START, (uint64_t)physicalMemBitMap.bitmap, physicalMemBitMap.size, (uint64_t)(PT_AF | PT_NX | PT_KERNEL | PT_ISH | PT_MEM | PT_RW) );
    memset( physicalMemBitMap.bitmap, 0, physicalMemBitMap.size ); 

    // Mark kernel and io addresses as used for the memory bit map
    size_t returnAddr;
    if( alloc_physical( ((uint64_t)&__end - VA_START + physicalMemBitMap.size), &returnAddr ) != PMM_OK ) {
        kprintf( "PMM ERROR: Could not allocate memory for io and kernel in physical memory bitmap\n" );
    }
    if( returnAddr != 0 ) {
        kprintf( "PMM ERROR: IO and Kernel not allocated at beginning of memory\n" );
    }
    kprintf( "Kernel Memory: 0x%lx\n", returnAddr );

    uint64_t addr;
    if( alloc_physical( MEGABYTES(4), &addr ) != PMM_OK ) {
        kprintf( "PMM ERROR: Could not allocate memory\n" );
    }
    kprintf( "Memory after kernel: 0x%lx\n", addr );

    addr = 0;
    if( alloc_physical( MEGABYTES(3), &addr ) != PMM_OK ) {
        kprintf( "PMM ERROR: Could not allocate memory\n" );
    }
    kprintf( "Memory after kernel: 0x%lx\n", addr );

}

/*
    return: error code & the physical address of the start of memory
*/
// TODO(matt): Implement buddy allocator
/*size_t alloc_physical( size_t numOfPages, size_t *physicalAddr ) {
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
}*/

size_t alloc_physical( size_t size, size_t *physicalAddr ) {
    /*uint32_t buddyIndex = 0;
    uint32_t bitIndexScaleFactor = 0;
    uint32_t numOf4kPagesToAllocate = size/4096;
    // Round memory size up 
    // TODO(matt): Look into optimizing out if statements
    if( size > MEGABYTES(2) ) {
         return PMM_REQUESTED_MEM_LARGER_THAN_LARGEST_BUDDY;
    } else if( size > MEGABYTES(1) ) {          // Handle 2M buddy
        buddyIndex = 9;
        bitIndexScaleFactor = 512;
    } else if( size > KILOBYTES(512) ) { // Handle 1M buddy
        buddyIndex = 8;
        bitIndexScaleFactor = 256;
    } else if( size > KILOBYTES(256) ) { // Handle 512K buddy
        buddyIndex = 7;
        bitIndexScaleFactor = 128;
    } else if( size > KILOBYTES(128) ) { // Handle 256K buddy 
        buddyIndex = 6;
        bitIndexScaleFactor = 64;
    } else if( size > KILOBYTES(64) ) {  // Handle 128K buddy
        buddyIndex = 5;
        bitIndexScaleFactor = 32;
    } else if( size > KILOBYTES(32) ) {  // Handle 64K buddy
        buddyIndex = 4;
        bitIndexScaleFactor = 16;
    } else if( size > KILOBYTES(16) ) {  // Handle 32K buddy
        buddyIndex = 3;
        bitIndexScaleFactor = 8;
    } else if( size > KILOBYTES(8) ) {   // Handle 16K buddy
        buddyIndex = 2;
        bitIndexScaleFactor = 4;
    } else if( size > KILOBYTES(4) ) {   // Handle 8K buddy
        buddyIndex = 1;
        bitIndexScaleFactor = 2;
    } else {                            // Handle 4K buddy
        buddyIndex = 0;
        bitIndexScaleFactor = 1;
    }
    uint32_t *bitmap = (uint32_t*)physicalMemBitMap.buddies[buddyIndex];
    uint32_t bitmapSize = physicalMemBitMap.buddySizes[buddyIndex]/4;

    // Find available page
    // TODO(matt): Use binary search of split buddies instead of linear search
    uint32_t index;
    uint32_t bitIndex = 0;
    for( index = 0; index < bitmapSize; ++index ) {
        if( bitmap[index] != 0xFFFFFFFF ) {
            bitIndex = num_trailing_zeros( ~bitmap[index] );
            break; 
        }
    }
    if( index >= bitmapSize ) {
        return PMM_MEM_NOT_AVAILABLE;
    }
    *physicalAddr = (index*32 + bitIndex) * (bitIndexScaleFactor * 4096);
    
    // Set container buddies as split
    for( uint32_t i = buddyIndex+1; i < 10; ++i ) {
        uint32_t *buddy = (uint32_t*)physicalMemBitMap.buddies[i];
        index /= 2;
        bitIndex /= 2;
        buddy[index] |= (1 << bitIndex);
    } 
    */

    // Find concurrent memory fitting requested memory size
    if( size > MEGABYTES(2) ) {
        // Handle more complex case of large memory allocation
        uint32_t *bitmap2M = (uint32_t *)physicalMemBitMap.buddies[9];
        uint32_t *allocated2M = (uint32_t *)physicalMemBitMap.allocated2M;
        size_t size2M = physicalMemBitMap.buddySizes[9]/4;
        size_t i, index;
        ssize_t bitIndex = -1;
        uint32_t pages = 0, enoughMemoryAvailable = 0, numOfPages = ((size+MEGABYTES(2)-1)&(~(MEGABYTES(2)-1)))/MEGABYTES(2);
        size_t numOfAvailablePages = 0;

        // Find available memory
        for( i = 0; i < size2M; ++i ) {
            pages = num_trailing_zeros( allocated2M[i] );
            if( pages >= numOfPages ) {
                enoughMemoryAvailable = 1;
                index = i;
                bitIndex = 0;
                break;
            }
            
            pages = num_leading_zeros( allocated2M[i] );
            if( pages > 0 ) {
                index = i;
                bitIndex = 32-pages;
                numOfAvailablePages += pages;
                for( ; i < size2M; ++i ) {
                    pages = num_leading_zeros( allocated2M[i] );
                    numOfAvailablePages += pages;
                    if( numOfAvailablePages >= numOfPages ) {
                        enoughMemoryAvailable = 1;
                        break;
                    }
                    if( pages != 32 ) {
                        numOfAvailablePages = 0;
                        break;
                    }
                }
            }

            if( numOfAvailablePages >= numOfPages ) {
                enoughMemoryAvailable = 1;
                break;
            }
        }
        if( !enoughMemoryAvailable ) {
            kprintf( "PMM ERROR: not enough memory available for allocation\n" );
            return PMM_MEM_NOT_AVAILABLE;
        }
        *physicalAddr = (index*32 + bitIndex) * MEGABYTES(2);

        // Mark 2MB bitmap as allocated
        uint32_t *bitmap = (uint32_t *)physicalMemBitMap.allocated2M;
        uint32_t stopIndex = bitIndex + numOfPages;
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

        // Mark 4KB bitmap as allocated
        bitmap = (uint32_t *)physicalMemBitMap.buddies[0];
        index = *physicalAddr/(4096*8);
        bitIndex = *physicalAddr & 31;
        stopIndex = (bitIndex + (size/4096));
        if( stopIndex > 32 ) stopIndex = 32;
        if( bitIndex != 0 ) {
            uint64_t bits = (1 << (stopIndex));
            bits -= 1;
            bits &= ~(1 << bitIndex);
            bits += 1; 
            bitmap[index] |= bits;
            index++;
        }
        for( ; index < (size/4096)/32; ++index  ) {
            bitmap[index] = 0xFFFFFFFF;
        }
        stopIndex = ((size/4096)-(stopIndex-bitIndex))%32;
        bits = (1 << stopIndex);
        bits -= 1;
        bitmap[index] |= bits;
    
    } else { 
        uint32_t buddyIndex = 0;
        uint32_t bitIndexScaleFactor = 0;
        if( size > MEGABYTES(1) ) {   // Handle 2M buddy
            buddyIndex = 9;
            bitIndexScaleFactor = 512;
        } else if( size > KILOBYTES(512) ) { // Handle 1M buddy
            buddyIndex = 8;
            bitIndexScaleFactor = 256;
        } else if( size > KILOBYTES(256) ) { // Handle 512K buddy
            buddyIndex = 7;
            bitIndexScaleFactor = 128;
        } else if( size > KILOBYTES(128) ) { // Handle 256K buddy 
            buddyIndex = 6;
            bitIndexScaleFactor = 64;
        } else if( size > KILOBYTES(64) ) {  // Handle 128K buddy
            buddyIndex = 5;
            bitIndexScaleFactor = 32;
        } else if( size > KILOBYTES(32) ) {  // Handle 64K buddy
            buddyIndex = 4;
            bitIndexScaleFactor = 16;
        } else if( size > KILOBYTES(16) ) {  // Handle 32K buddy
            buddyIndex = 3;
            bitIndexScaleFactor = 8;
        } else if( size > KILOBYTES(8) ) {   // Handle 16K buddy
            buddyIndex = 2;
            bitIndexScaleFactor = 4;
        } else if( size > KILOBYTES(4) ) {   // Handle 8K buddy
            buddyIndex = 1;
            bitIndexScaleFactor = 2;
        } else {                            // Handle 4K buddy
            buddyIndex = 0;
            bitIndexScaleFactor = 1;
        }

        
        // Mark pages as allocated
        /*uint32_t *bitmap = (uint32_t *)physicalMemBitMap.buddies[0];
        index = *physicalAddr/(4096*32);
        bitIndex = (*physicalAddr/4096) & 31;
        uint32_t stopIndex = (bitIndex + numOf4kPagesToAllocate);
        if( stopIndex > 32 ) stopIndex = 32;
        if( bitIndex != 0 ) {
            uint64_t bits = (1 << (stopIndex));
            bits -= 1;
            bits &= ~(1 << bitIndex);
            bits += 1; 
            bitmap[index] |= bits;
            index++;
        }
        for( ; index < (numOf4kPagesToAllocate)/32; ++index  ) {
            bitmap[index] = 0xFFFFFFFF;
        }
        stopIndex = (numOf4kPagesToAllocate-(stopIndex-bitIndex))%32;
        uint64_t bits = (1 << stopIndex);
        bits -= 1;
        bitmap[index] |= bits;*/
    
    }

    return PMM_OK;
}

size_t free_physical( size_t physicalAddr ) {
    // Look at address alignment for hint at how big the allocated memory might be. Then check if memory is split

   // Mark pages as unallocated and merge any split buddies 
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
