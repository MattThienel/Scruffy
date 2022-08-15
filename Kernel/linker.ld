ENTRY(_start)

PA_START = 0x0000000040080000;
VA_START = 0xffffffe000000000;

SECTIONS {
	/* Starts at LOADER_ADDR */
	. = 0x0000000040080000;
/*  . = 0xffffffe040000000; */
	__boot_start = .;

	__boot_start = .;
    __boot_text_start = .;
    .text.boot : 
    {
        KEEP(build/obj/prekernel/* (.text.boot))
        /*KEEP( prekernel/build/\*(.text.boot) )*/
        KEEP(build/obj/prekernel/* (.text*))
    }
    . = ALIGN(4K);
    __boot_text_end = .;
    __boot_rodata_start = .;
    .rodata.boot :
    {
        build/obj/prekernel/* (.rodata*)
    }
    . = ALIGN(4K);
    __boot_rodata_end = .;
    __boot_data_start = .;
    .data.boot :
    {
         build/obj/prekernel/* (.data*)
        
        . = ALIGN(4K);
        __boot_stack_start = .;
        . += 4K;
        . = ALIGN(4K);
        __boot_stack_top = . - 8;
        __boot_stack_size = __boot_stack_top - __boot_stack_start;    

        __boot_pg_tbl_start = .;
        . += 4K;
        __boot_pg_tbl_end = .;
    }
    . = ALIGN(4K);
    __boot_data_end = .;

    __boot_bss_start = .;
    .bss.boot : 
    {
        build/obj/prekernel/* (.bss*)
    }
    . = ALIGN(4K);
    __boot_bss_end = .;
    __boot_bss_size = __boot_bss_end - __boot_bss_start;
    __boot_end = .;

    OFFSET = VA_START;
    . += OFFSET; 
/*    . = VA_START;*/

    __start = .;
    __text_start = .;
	.text ALIGN(4K) : AT (ADDR (.text) - OFFSET)
	{
        __text_start_phys = . - OFFSET;
        build/obj/kernel/kernel.o (.text*)
		build/obj/kernel/* (.text*)
        . = ALIGN(4K);
        __text_end_phys = . - OFFSET;
	}
	. = ALIGN(4096);
	__text_end = .;
	
	__rodata_start = .;
	.rodata ALIGN(4K) : AT (ADDR (.rodata) - OFFSET)
	{
        __rodata_start_phys = . - OFFSET;
		build/obj/kernel/* (.rodata*)
        . = ALIGN(4K);
        __rodata_end_phys = . - OFFSET;
	}
	. = ALIGN(4096);
	__rodata_end = .;

	__data_start = .;
	.data ALIGN(4k) : AT (ADDR (.data) - OFFSET)
	{
        __data_start_phys = . - OFFSET;
		build/obj/kernel/* (.data*)

        . = ALIGN(4K);
        __stack_start = .;
        . += 2M;
        . = ALIGN(2M);
        __stack_top = . - 8;

        __pg_tbl_start = .;
        . += 8M;
        __pg_tbl_end = .;

        __data_end_phys = . - OFFSET;
	}
	. = ALIGN(4096);
	__data_end = .;

	__bss_start = .;
	.bss ALIGN(4K) : AT (ADDR (.bss) - OFFSET)
	{
        __bss_start_phys = . - OFFSET;
		bss = .;
		build/obj/kernel/* (.bss*)
        . = ALIGN(4K);
        __bss_end_phys = . - OFFSET;
	}
	. = ALIGN(4096);
	__bss_end = .;
	__bss_size = __bss_end - __bss_start;
    
	__end = .;
}

__stack_size = __stack_top - __stack_start;    
