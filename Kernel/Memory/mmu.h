/*

*/

#ifndef _MMU_H
#define _MMU_H

#include <assert.h>
#include "types.h"


#define VA_START    0xFFFFFFE000000000
#define PA_START    0x0000000040080000

#define PAGESIZE    4096

// granularity
#define PT_PAGE     0b11
#define PT_BLOCK    0b01

// accessibility
#define PT_KERNEL   (0 << 6)    // privileged, supervisor EL1 access only
#define PT_USER     (1 << 6)    // unprivileged, EL0 access allowed
#define PT_RW       (0 << 7)    // read-write
#define PT_RO       (1 << 7)    // read-only
#define PT_AF       (1 << 10)   // accessed flag
#define PT_NX       (1ULL << 54) // no execute

// shareability
#define PT_OSH      (2 << 8)    // outter shareable
#define PT_ISH      (3 << 8)    // inner shareable

// defined in MAIR register
#define PT_MEM      (0 << 2)    // normal memory
#define PT_DEV      (1 << 2)    // device MMIO
#define PT_NC       (2 << 2)    // non-cachable

#define TTBR_CNP    1

#define TCR_TBI     0b00LL << 37
#define TCR_TG1     0b10LL << 30
#define TCR_SH1     0b11LL << 28
#define TCR_ORGN1   0b01LL << 26
#define TCR_IRGN1   0b01LL << 24
#define TCR_EPD1    0b0LL << 23
#define TCR_T1SZ    (64LL-37LL) << 16
#define TCR_TG0     0b00LL << 14
#define TCR_SH0     0b11LL << 12
#define TCR_ORGN0   0b01LL << 10
#define TCR_IRGN0   0b01LL << 8
#define TCR_EPD0    0b0LL << 7
#define TCR_T0SZ    (64LL-37LL) << 0

typedef union {
    struct {
        uint64_t validBit : 1;
        uint64_t tableBit : 1;
        uint64_t attrIndx : 3;
        uint64_t NS : 1;            // Non-secure bit
        uint64_t AP : 2;            // Data Access Permissions bits
        uint64_t SH : 2;            // Shareability field
        uint64_t AF : 1;            // Access Flag
        uint64_t nG : 1;            // Not global bit
        uint64_t OA : 4;        
        uint64_t nT : 1;            // Block translation entry
        uint64_t address : 33;  
        uint64_t GP : 1;            // Guarded Page
        uint64_t DBM : 1;           // Dirty Bit Modifier
        uint64_t contiguous : 1;    // Contigous hint bit for TLB
        uint64_t PXN : 1;           // Privelege execute-never field
        uint64_t UXN_XN : 1;        // Execute-never or Unprivileged execute-never field
        uint64_t res0 : 4;          
        uint64_t PBHA : 4;          // Page-based Hardware Attributes bits
        uint64_t ignored : 1;
    };
    uint64_t value;
} page_table_descriptor_t;

typedef union {
    uint64_t reg;
    struct {
        uint64_t index : 52;
        uint64_t bitIndex : 5;
        uint64_t extra : 7;
    };
} free_buddy_t;
static_assert( sizeof(free_buddy_t) == sizeof(uint64_t) );

typedef struct free_buddy_node {
    struct free_buddy_node *next, *prev;
    free_buddy_t freeBuddy;
} free_buddy_node_t;

typedef struct free_buddy_pool {
    struct free_buddy_pool *nextPool; 
    size_t numFreeBuddies;
    free_buddy_t *freeBuddies;
} free_buddy_pool_t;

/*
Buddy System: 0 = not split, 1 = split for all bitmaps except smallest 4k bitmap, where 0 = free, 1 = used
*/
typedef struct {
    //uint8_t* buddies[10]; // First buddy is 4k pages, incrementing to 2M, denotes split or not
    //uint32_t buddySizes[10];
    //uint8_t* allocated2M;  // Denotes if a 2M is allocated or not
    //free_buddy_t* freeSplitBuddies[9]; // Free buddy is a circular list
    //uint32_t freeBuddiesStartIndex[9]; // Index for start of circular list
    //uint32_t freeBuddiesLength[9];     // Length of circular list
    //uint32_t numFreeSplitBuddies;      // Number of items in circular buffer
    uint32_t *bitmap[2];    // 0: 4K pages, 1: 1M pages
    uint32_t bitmapSize[2];
} pmm_bitmap_t;

typedef struct {
    uint32_t *allocated1G;
    uint32_t *allocated2M;
    uint32_t *allocated4K;
} vmm_bitmap_t;

enum physical_memory_manager_status_n {
    PMM_OK,
    PMM_MEM_NOT_AVAILABLE,
    PMM_REQUESTED_MEM_LARGER_THAN_LARGEST_BUDDY,
};

typedef struct {
    void *address;
    size_t size;
} page_block_t;

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


void mmu_init( size_t memSize );
//size_t alloc_physical( size_t numOfPages, size_t *physicalAddr );
void map_pg_tbl( uint64_t startPA, uint64_t startVA, int64_t size, uint64_t permissions );

page_block_t alloc_physical( size_t size );
void free_physical( page_block_t page );
size_t kalloc( size_t size );
void kfree( size_t size );

#endif /* _MMU_H */
