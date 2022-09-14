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
    size_t size = 0;

    // Fill and total sizes for each buffer
    /*for( uint32_t i = 0; i < 10; ++i ) {
        physicalMemBitMap.buddySizes[i] = memSize/(4096*(8<<i));
        size += physicalMemBitMap.buddySizes[i];
    }
    size += physicalMemBitMap.buddySizes[9]; // Add size for allocated2M array

    // Set memory addresses for arrays
    physicalMemBitMap.buddies[0] = (uint8_t*)((uint64_t)&__end);
    for( uint32_t i = 1; i < 10; ++i ) {
        physicalMemBitMap.buddies[i] = (uint8_t*)((uint64_t)physicalMemBitMap.buddies[i-1] + physicalMemBitMap.buddySizes[i-1]);
    }
    physicalMemBitMap.allocated2M = (uint8_t*)((uint64_t)physicalMemBitMap.buddies[9] + physicalMemBitMap.buddySizes[9]);

    // Initialize free buddy arrays
    physicalMemBitMap.numFreeSplitBuddies = 64;
    physicalMemBitMap.freeSplitBuddies[0] = (free_buddy_t*)((uint64_t)physicalMemBitMap.allocated2M + physicalMemBitMap.buddySizes[9]);
    size += physicalMemBitMap.numFreeSplitBuddies;
    for( uint32_t i = 1; i < 10; ++i ) {
        physicalMemBitMap.freeSplitBuddies[i] = (free_buddy_t*)((uint64_t)physicalMemBitMap.freeSplitBuddies[i-1] + physicalMemBitMap.numFreeSplitBuddies);
        size += physicalMemBitMap.numFreeSplitBuddies;
    }*/

    physicalMemBitMap.bitmapSize[0] = (memSize/KILOBYTES(32*4));
    physicalMemBitMap.bitmapSize[1] = (memSize/MEGABYTES(32*1));
    physicalMemBitMap.bitmap[0] = (uint32_t*)((uint64_t)&__end);
    physicalMemBitMap.bitmap[1] = (uint32_t*)((uint64_t)physicalMemBitMap.bitmap[0] + physicalMemBitMap.bitmapSize[0]*4);
    size += physicalMemBitMap.bitmapSize[0]*4;    
    size += physicalMemBitMap.bitmapSize[1]*4;    

    // Map physical memory manager to virtual memory
    map_pg_tbl( (uint64_t)physicalMemBitMap.bitmap[0]-VA_START, (uint64_t)physicalMemBitMap.bitmap[0], size, (uint64_t)(PT_AF | PT_NX | PT_KERNEL | PT_ISH | PT_MEM | PT_RW) );
    memset( physicalMemBitMap.bitmap[0], 0, size ); 
    kprintf( "PMM Size: 0x%lx\n", size );

    // Mark kernel and io addresses as used for the memory bit map
    page_block_t kernelIO = alloc_physical( ((uint64_t)&__end - VA_START + size) ); 
    if( kernelIO.size == 0 ) {
        kprintf( "PMM ERROR: Could not allocate memory for io and kernel in physical memory bitmap\n" );
    }
    if( kernelIO.address != 0 ) {
        kprintf( "PMM ERROR: IO and Kernel not allocated at beginning of memory\n" );
    }
    kprintf( "Kernel Memory: 0x%lx\n", kernelIO.address );

    page_block_t randomMem[10];

    randomMem[0] = alloc_physical( KILOBYTES(8) ); 
    randomMem[1] = alloc_physical( KILOBYTES(4) );
    randomMem[2] = alloc_physical( KILOBYTES(4) );
    randomMem[3] = alloc_physical( KILOBYTES(8) );
    randomMem[4] = alloc_physical( KILOBYTES(4) );
    randomMem[5] = alloc_physical( KILOBYTES(16) );
    randomMem[6] = alloc_physical( KILOBYTES(8) );
    randomMem[7] = alloc_physical( KILOBYTES(8) );

    for( uint32_t i = 0; i < 8; ++i ) {
        if( randomMem[i].size == 0 ) {
            kprintf( "PMM ERROR: Could not allocate memory\n" );
        }
        kprintf( "Memory after kernel: 0x%lx\n", randomMem[i].address );
    }

    free_physical( randomMem[0] ); 
    free_physical( randomMem[1] );
    free_physical( randomMem[5] );
    randomMem[3] = alloc_physical( KILOBYTES(16) );
    
    if( randomMem[3].size == 0 ) {
        kprintf( "PMM ERROR: Could not allocate memory\n" );
    }
    kprintf( "Memory after kernel: 0x%lx\n", randomMem[3].address );

}

/*bool32_t find_free_buddy( uint32_t buddyIndex ) {
    return 0;
}*/

void mark_pages_as_allocated( uint32_t **bitmap, uint32_t index, uint32_t bitIndex, uint32_t numOfPages ) {
    uint32_t stopIndex = bitIndex + numOfPages;
    uint32_t bits = 0;
    uint32_t pagesSet = 0;
    if( bitIndex != 0 ) {
        if( stopIndex >= 32 ) {
            bits = 0xFFFFFFFF;
        } else {
            bits = 1 << stopIndex;
            bits -= 1;
        }
        bits &= ~(1 << bitIndex);
        bits += 1;
        (*bitmap)[index] |= bits;
        if( stopIndex == 32 ) return;
        pagesSet = 32-bitIndex; 
        if( pagesSet >= numOfPages ) return;
        index++;
    }

    uint32_t i;
    for( i = 0; i < (numOfPages-pagesSet)/32; ++i ) {
        (*bitmap)[index+i] = 0xFFFFFFFF;
    }
    index = index+i;
    //pagesSet += 32*((numOfPages-pagesSet)/32);
    pagesSet += (32*i);
    
    stopIndex = (numOfPages-pagesSet)%32;
    bits = (1 << stopIndex);
    bits -= 1;
    (*bitmap)[index] |= bits;
 
}

// IMPORTANT(matt): Only allocate memory in 1MB and 4KB sizes 
// TODO(matt): Allow smaller memory allocation sizes
page_block_t alloc_physical( size_t size ) {
    // Find concurrent memory fitting requested memory size
    page_block_t memory = {0};
    uint32_t bitmapSelect = 0;
    uint32_t numOfPages;
    if( size >= MEGABYTES(1) ) { 
        bitmapSelect = 1;
        numOfPages = ((size + MEGABYTES(1)-1)&(~(MEGABYTES(1)-1)))/MEGABYTES(1);
        memory.size = numOfPages * MEGABYTES(1);
    } else {
        numOfPages = ((size + KILOBYTES(4)-1)&(~(KILOBYTES(4)-1)))/KILOBYTES(4);
        memory.size = numOfPages * KILOBYTES(4);
    } 

    size_t i, index;
    ssize_t bitIndex = -1;
    uint32_t pages = 0, enoughMemoryAvailable = 0;
    size_t numOfAvailablePages = 0;
    uint32_t *bitmap = physicalMemBitMap.bitmap[bitmapSelect];

    // Find available memory
    // TODO(matt): Detect zero bits sandwiched between two set bits
    for( i = 0; i < physicalMemBitMap.bitmapSize[bitmapSelect]; ++i ) {
        pages = num_trailing_zeros( bitmap[i] );
        if( pages >= numOfPages ) {
            enoughMemoryAvailable = 1;
            index = i;
            bitIndex = 0;
            goto memory_found;
        }
        
        if( pages < 32 ) { 
            while(1) {
                uint32_t numShiftedBits = pages+1;
                uint32_t bits = bitmap[i];
                bits = (bits >> 1) | (1 << 31);
                bits = (uint32_t)((int32_t)bits >> pages);
                uint32_t tempBits = bits + 1;
                uint32_t numUsedPages = num_trailing_zeros( tempBits );
                bits = (uint32_t)((int32_t)bits >> numUsedPages);      
                numShiftedBits += numUsedPages;
                pages = num_trailing_zeros( bits );
                if( pages >= numOfPages ) {
                    enoughMemoryAvailable = 1;
                    index = i;
                    bitIndex = numShiftedBits;
                    goto memory_found;
                }
                if( numShiftedBits >= 32 ) {
                    break;
                }
            }
        }


        pages = num_leading_zeros( bitmap[i] );
        if( pages > 0 ) {
            index = i;
            bitIndex = 32-pages;
            numOfAvailablePages += pages;
            for( ; i < physicalMemBitMap.bitmapSize[bitmapSelect]; ++i ) {
                pages = num_leading_zeros( bitmap[i] );
                numOfAvailablePages += pages;
                if( numOfAvailablePages >= numOfPages ) {
                    enoughMemoryAvailable = 1;
                    goto memory_found;
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
memory_found:
    if( !enoughMemoryAvailable ) {
        kprintf( "PMM ERROR: not enough memory available for allocation\n" );
        memory.size = 0;
        memory.address = (void*)0;
        return memory;
    }

    mark_pages_as_allocated( &bitmap, index, bitIndex, numOfPages ); 
    size_t physicalAddr = (index*32 + bitIndex);
    if( size >= MEGABYTES(1) ) {
        physicalAddr *= MEGABYTES(1);
        bitmap = physicalMemBitMap.bitmap[0];
        index = physicalAddr/KILOBYTES(32*4); 
        bitIndex = 0; 
        numOfPages *= 256;
    } else {
        physicalAddr *= KILOBYTES(4);
        bitmap = physicalMemBitMap.bitmap[1];
        index /= 256;
        bitIndex = (physicalAddr >> 20) & 0x1F;
        numOfPages = 1;
    }
    memory.address = (void*)(physicalAddr);
    mark_pages_as_allocated( &bitmap, index, bitIndex, numOfPages ); 

    return memory;
}

void mark_pages_as_free( uint32_t **bitmap, uint32_t index, uint32_t bitIndex, uint32_t numOfPages ) {
/*    uint32_t stopIndex = bitIndex + numOfPages;
    if( stopIndex > 32 ) stopIndex = 32;
    if( bitIndex != 0 ) {
        uint64_t bits = (1 << (stopIndex));
        bits -= 1;
        bits &= ~(1 << bitIndex);
        bits += 1; 
        (*bitmap)[index] &= ~bits;
        index++;
    }
    for( ; index < (numOfPages)/32; ++index  ) {
        (*bitmap)[index] = 0x0;
    }
    stopIndex = (numOfPages-(bitIndex))%32;
    uint64_t bits = (1 << stopIndex);
    bits -= 1;
    (*bitmap)[index] &= ~bits;*/

    uint32_t stopIndex = bitIndex + numOfPages;
    uint32_t bits = 0;
    uint32_t pagesSet = 0;
    if( bitIndex != 0 ) {
        if( stopIndex >= 32 ) {
            bits = 0xFFFFFFFF;
        } else {
            bits = 1 << stopIndex;
            bits -= 1;
        }
        bits &= ~(1 << bitIndex);
        bits += 1;
        (*bitmap)[index] &= ~bits;
        if( stopIndex == 32 ) return;
        pagesSet = 32-bitIndex; 
        if( pagesSet >= numOfPages ) return;
        index++;
    }

    uint32_t i;
    for( i = 0; i < (numOfPages-pagesSet)/32; ++i ) {
        (*bitmap)[index+i] = 0x0;
    }
    index = index+i;
    //pagesSet += 32*((numOfPages-pagesSet)/32);
    pagesSet += (32*i);
    
    stopIndex = (numOfPages-pagesSet)%32;
    bits = (1 << stopIndex);
    bits -= 1;
    (*bitmap)[index] &= ~bits;
}

void free_physical( page_block_t page ) {
    if( page.size >= MEGABYTES(1) ) {
        // Clear 1MB bitmap
        uint32_t index = (size_t)page.address/MEGABYTES(32*1);
        uint32_t bitIndex = ((size_t)page.address/MEGABYTES(1)) & 0x1F;
        mark_pages_as_free( &(physicalMemBitMap.bitmap[1]), index, bitIndex, page.size/MEGABYTES(1) ); 

        // Clear 4KB bitmap
        index = (size_t)page.address/KILOBYTES(32*4);
        bitIndex = ((size_t)page.address/KILOBYTES(4)) & 0x1F;
        mark_pages_as_free( &(physicalMemBitMap.bitmap[0]), index, bitIndex, page.size/KILOBYTES(4) ); 
    } else {
        // Clear 4KB bitmap
        uint32_t index = (size_t)page.address/KILOBYTES(32*4);
        uint32_t bitIndex = ((size_t)page.address/KILOBYTES(4)) & 0x1F;
        mark_pages_as_free( &(physicalMemBitMap.bitmap[0]), index, bitIndex, page.size/KILOBYTES(4) ); 
        
        // Clear 1MB if all other 4KB pages are free 
        index = (size_t)page.address/MEGABYTES(32*1);
        bitIndex = ((size_t)page.address/MEGABYTES(1)) & 0x1F;
        uint32_t megabyteFree = 1;
    
        for( uint32_t i = 0; i < 256/32; ++i ) {
            if( physicalMemBitMap.bitmap[0][(((index*32)+bitIndex)*(256/32))+i] != 0x0) {
                megabyteFree = 0;
                break;
            }
        }

        if( megabyteFree ) 
            mark_pages_as_free( &(physicalMemBitMap.bitmap[1]), index, bitIndex, 1 ); 
    }
    
}

size_t kalloc( size_t size ) {
    return 0;
}

void kfree( size_t size ) {

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

// Buddy Allocator Attempt
/*
    #if 0
    if( size > MEGABYTES(1) ) {
    #else
    if( 1 ) {
    #endif
        // Handle more complex case of large memory allocation
        uint32_t *bitmap2M = (uint32_t *)physicalMemBitMap.buddies[9];
        uint32_t *allocated2M = (uint32_t *)physicalMemBitMap.allocated2M;
        size_t size2M = physicalMemBitMap.buddySizes[9]/4;
        size_t i, index;
        ssize_t bitIndex = -1;
        uint32_t pages = 0, enoughMemoryAvailable = 0, numOfPages = ((size+MEGABYTES(2)-1)&(~(MEGABYTES(2)-1)))/MEGABYTES(2);
        size_t numOfAvailablePages = 0;
        page_block_t memory = {0};

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
            return memory;
            //return PMM_MEM_NOT_AVAILABLE;
        }

        size_t physicalAddr = (index*32 + bitIndex) * MEGABYTES(2);
        memory.address = (void*)(physicalAddr);

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
        index = physicalAddr/(4096*8);
        bitIndex = physicalAddr & 31;
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
        
        memory.address = (void*) physicalAddr;
        memory.size = numOfPages * MEGABYTES(2); 
        return memory;
    
    } else { 
        uint32_t bitIndex = 0, index = 0;
        uint32_t buddyIndex = 0;
        if( size > KILOBYTES(512) ) {        // Handle 1M buddy
            buddyIndex = 8;
        } else if( size > KILOBYTES(256) ) { // Handle 512K buddy
            buddyIndex = 7;
        } else if( size > KILOBYTES(128) ) { // Handle 256K buddy 
            buddyIndex = 6;
        } else if( size > KILOBYTES(64) ) {  // Handle 128K buddy
            buddyIndex = 5;
        } else if( size > KILOBYTES(32) ) {  // Handle 64K buddy
            buddyIndex = 4;
        } else if( size > KILOBYTES(16) ) {  // Handle 32K buddy
            buddyIndex = 3;
        } else if( size > KILOBYTES(8) ) {   // Handle 16K buddy
            buddyIndex = 2;
        } else if( size > KILOBYTES(4) ) {   // Handle 8K buddy
            buddyIndex = 1;
        } else {                             // Handle 4K buddy
            buddyIndex = 0;
        }

        // Check if free split buddy exists
        if( physicalMemBitMap.freeBuddiesLength[buddyIndex] > 0 ) {
            bitIndex = ((free_buddy_t*)physicalMemBitMap.freeSplitBuddies[buddyIndex]+physicalMemBitMap.freeBuddiesStartIndex[buddyIndex])->bitIndex;
            index = ((free_buddy_t*)physicalMemBitMap.freeSplitBuddies[buddyIndex]+physicalMemBitMap.freeBuddiesStartIndex[buddyIndex])->index;
            physicalMemBitMap.freeBuddiesLength[buddyIndex] -= 1;
            physicalMemBitMap.freeBuddiesStartIndex[buddyIndex] += 1;
            if( physicalMemBitMap.freeBuddiesStartIndex[buddyIndex] >= physicalMemBitMap.numFreeSplitBuddies ) {
                physicalMemBitMap.freeBuddiesStartIndex[buddyIndex] = 0;
            }
            goto memory_found;
        }

        // TODO(matt): Look into best fit memory search 
        // Find free memory, binary tree style search
        uint32_t *allocated2M = (uint32_t*)physicalMemBitMap.allocated2M;
        uint32_t *buddies[10];
        uint32_t buddySizes[10]; 

        // Convert buddies into 32-bit array for searching bits
        for( uint32_t i = 0; i < 10; ++i ) {
            buddies[i] = (uint32_t*)(physicalMemBitMap.buddies[i]);
            buddySizes[i] = physicalMemBitMap.buddySizes[i]/4;
        }

        // Find available memory to split for buddy
        for( uint32_t i = 0; i < buddySizes[9]; ++i ) {
            if( allocated2M[i] == 0xFFFFFFFF && buddies[9] == 0x0 ) {
                continue;
            }
            uint32_t val = allocated2M[i];
            for( uint8_t j = num_trailing_zeros(~allocated2M[i]); j < 31; ++j ) {
                
            } 

        } 
memory_not_found:
        page_block_t memNotFound = {0};
        return memNotFound;

memory_found:
        page_block_t memory = {0};
  */      
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
