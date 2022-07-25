.section ".text.boot"

.global _start
_start:
    // Save kernel arguments
    mov     x19, x0
    mov     x20, x1
    mov     x21, x2
    mov     x22, x3

    // read cpu id, stop slave cores
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, 2f
   
    // if cpu id > 0, stop
1:  wfe
    b       1b

2:  // cpu id == 0

    ldr     x5, =__boot_stack_top
    
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
    ldr     x5, =__boot_bss_start
    ldr     x6, =__boot_bss_size
    cbz     x6, 2f
1:  str     xzr, [x5], #8
    subs    x6, x6, #8
    bgt     1b 
2:
    bl      _pk_irq_vector_init

    // Reload kernel arguments
    mov     x0, x19
    mov     x1, x20
    mov     x2, x21
    mov     x3, x22
    bl      init
    b       .
