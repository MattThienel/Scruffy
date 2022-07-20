ENTRY(_start)

PA_START = 0x0000000004000000;
VA_START = 0xfffffff000000000;

SECTIONS {
	/* Starts at LOADER_ADDR */
	. = 0x0000000040080000;
/*  . = 0xffffffe040000000; */
	__start = .;

	__boot_start = .;
    __boot_text_start = .;
    .text.boot : 
    {
        *(.text.boot)
        /*KEEP( prekernel/build/\*(.text.boot) )*/
        build/obj/prekernel* (.text)
    }
    . = ALIGN(4K);
    __boot_text_end = .;
    __boot_rodata_start = .;
    .rodata.boot :
    {
        build/obj/prekernel* (.rodata)
    }
    . = ALIGN(4K);
    __boot_rodata_end = .;
    __boot_data_start = .;
    .data.boot :
    {
         build/obj/prekernel* (.data)
        
    }
    . = ALIGN(4K);
    __boot_data_end = .;
    __boot_bss_start = .;
    .bss.boot : 
    {
        build/obj/prekernel* (.bss)
    }
    . = ALIGN(4K);
    __boot_bss_end = .;
    __boot_end = .;

    __stack_start = .;
    . += 2M;
    . = ALIGN(2M);
    __stack_top = . - 8;
    __stack_size = __stack_top - __stack_start;    

	__pg_tbl_start = .;
    . += 2M;
    . = ALIGN(2M);
    __pg_tbl_end = .;

    . += VA_START; 

    __text_start = .;
	.text ALIGN(4K) : AT (ADDR (.text) - VA_START)
	{
		*(.text)
	}
	. = ALIGN(4096);
	__text_end = .;
	
	__rodata_start = .;
	.rodata ALIGN(4K) : AT (ADDR (.rodata) - VA_START)
	{
		*(.rodata)
	}
	. = ALIGN(4096);
	__rodata_end = .;

	__data_start = .;
	.data ALIGN(4k) : AT (ADDR (.data) - VA_START)
	{
		*(.data)
	}
	. = ALIGN(4096);
	__data_end = .;

	__bss_start = .;
	.bss ALIGN(4K) : AT (ADDR (.bss) - VA_START)
	{
		bss = .;
		*(.bss)
	}
	. = ALIGN(4096);
	__bss_end = .;
	__bss_size = __bss_end - __bss_start;
    
	__end = .;
}
