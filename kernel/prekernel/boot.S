.section ".text.boot"

.global _start
_start:
    // read cpu id, stop slave cores
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, 2f
   
    // if cpu id > 0, stop
1:  wfe
    b       1b

2:  // cpu id == 0

    ldr     x5, =__stack_top
    
    // get current exception level
    mrs     x0, CurrentEL
    lsr     x0, x0, #2
    and     x0, x0, #3

    // running at EL3?
    cmp     x0, #3
    bne     3f
    
    mov     x1, #0x5b1
    msr     scr_el3, x1
    mov     x1, #0x3c9
    msr     spsr_el3, x1
    adr     x1, 3f
    msr     elr_el3, x1
    eret
    
    // running at EL2?
3:  cmp     x0, #1
    beq     4f
    
    msr     sp_el1, x5

    // enable CNTP for EL1
    mrs     x0, cnthctl_el2
    orr     x0, x0, #3
    msr     cnthctl_el2, x0    
    msr     cntvoff_el2, xzr

    // enable AArch64 in EL1
    mov     x0, #(1 << 31)      // AArch64
    orr     x0, x0, #(1 << 1)   // SWIO
    msr     hcr_el2, x0
    mrs     x0, hcr_el2
    // Setup SCTLR access
    mov     x2, #0x0800
    movk    x2, #0x30d0, lsl #16
    msr     sctlr_el1, x2
    // change execution level to EL1
    mov     x2, #0x3c4
    msr     spsr_el2, x2
    adr     x2, 4f
    msr     elr_el2, x2 
    eret

    // EL1
4:  mov     sp, x5
    
    // Clear BSS
    ldr     x5, =__bss_start
    ldr     x6, =__bss_size
    cbz     x6, 2f
1:  str     xzr, [x5], #8
    subs    x6, x6, #8
    bgt     1b 
2:
    bl      _irq_vector_init

    bl      init
    b       .
