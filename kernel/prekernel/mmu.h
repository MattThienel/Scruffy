/*
 *
 */

#ifndef _PREKERNEL_MMU_H
#define _PREKERNEL_MMU_H

#include "types.h"

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

//extern volatile uint64_t __pg_tbl_start;
extern volatile uint64_t __pg_tbl_start;

void mmu_init( void );

#endif /* _PREKERNEL_MMU_H */
