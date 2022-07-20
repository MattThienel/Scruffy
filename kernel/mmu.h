/*

*/

#ifndef _MMU_H
#define _MMU_H

#include <assert.h>
#include "types.h"


#define VA_START    0xFFFFFFE000000000
#define PA_START    0x40000000

#define MT_DEVICE_nGnR E        0x0   
#define MT_NORMAL_NC            0x1
#define MT_NORMAL               0x2
#define MT_DEVICE_nGnRE_FLAG    0x04 // Device, nGnRE 
#define MT_NORMAL_NC_FLAG       0x44 // Non-cacheable 
#define MT_NORMAL_FLAG          0xFF // Normal, IWBWA, OWBWA, NTR
#define MAIR_VALUE \
    ( MT_NORMAL_FLAG << (8 * MT_NORMAL) | \
      MT_NORMAL_NC_FLAG << (8 * MT_NORMAL_NC) | \
      MT_DEVICE_nGnRE_FLAG << (8 * MT_DEVICE_nGnRE))


#define TCR_T0SZ        (64 - 37)
#define TCR_T1SZ        ((64 - 37) << 16)
#define TCR_TG0_4K      (0 << 14)
#define TCR_TG1_4K      (2 << 30)
#define TCR_SH0         (3 << 12)
#define TCR_SH1         (3 << 28)
#define TCR_ORGN0       (1 << 10)
#define TCR_ORGN1       (1 << 26)
#define TCR_IRGN0       (1 << 8)
#define TCR_IRGN1       (1 << 24)
#define TCR_TBI         (0 << 37)
#define TCR_VALUE \
    (TCR_T0SZ | TCR_T1SZ | TCR_TG0_4K | TCR_TG1_4K \
     TCR_SH0 | TCR_SH1 | TCR_ORGN0 | TCR_ORGN1 \
     TCR_IRGN0 | TCR_IRGN1 | TCR_TBI)

#define MMU_PAGE_ENTRY          0b11 << 0
#define MMU_TABLE_ENTRY         0b11 << 0
#define MMU_BLOCK_ENTRY         0b01 << 0
#define MMU_ACCESS              0b1  << 10
#define MMU_ACCESS_PERMISSION   0b1  << 6

#define MMU_FLAGS               (MMU_ACCESS | (NORMAL_NC << 2) | MMU_BLOCK_ENTRY)
#define MMU_DEVICE_FLAGS        (MMU_ACCESS | (DEVICE_nGnRnE << 2) | MMU_BLOCK_ENTRY)
#define MMU_PTE_FLAGS           (MMU_ACCESS | MM_ACCESS_PERMISSION | (NORMAL_NC << 2) | MMU_PAGE_ENTRY)

#define PAGE_SHIFT              9
#define TABLE_SHIFT             12

#define PAGE_SIZE               (1 << PAGE_SHIFT)
#define TABLE_SIZE              (1 << TABLE_SHIFT)

#define SCTLR_RESERVED                  (3 << 28) | (3 << 22) | (1 << 20) | (1 << 11)
#define SCTLR_EE_LITTLE_ENDIAN          (0 << 25)
#define SCTLR_EOE_LITTLE_ENDIAN         (0 << 24)
#define SCTLR_I_CACHE_DISABLED          (0 << 12)
#define SCTLR_D_CACHE_DISABLED          (0 << 2)
#define SCTLR_MMU_DISABLED              (0 << 0)
#define SCTLR_MMU_ENABLED               (1 << 0)

#define SCTLR_VALUE_MMU_DISABLED	(SCTLR_RESERVED | SCTLR_EE_LITTLE_ENDIAN | SCTLR_I_CACHE_DISABLED | SCTLR_D_CACHE_DISABLED | SCTLR_MMU_DISABLED)

#endif /* _MMU_H */
