ENTRY(_start)
SECTIONS {
	. = 0x0000000040080000;
	__start = .;

	__text_start = .;
    .text.boot : 
    {
        KEEP( *(.text.boot) )
    }
	.text : 
	{
		*(.text)
	}
	/*. = ALIGN(4096);*/
    . = ALIGN(2M);
	__text_end = .;
	
	__rodata_start = .;
	.rodata :
	{
		*(.rodata)
	}
	/*. = ALIGN(4096);*/
    . = ALIGN(2M);
	__rodata_end = .;

	__data_start = .;
	.data :
	{
		*(.data)
	}
	/*. = ALIGN(4096);*/
    . = ALIGN(2M);
	__data_end = .;

	__bss_start = .;
	.bss :
	{
		bss = .;
		*(.bss)
	}
	/*. = ALIGN(4096);*/
    . = ALIGN(2M);
	__bss_end = .;
	__bss_size = __bss_end - __bss_start;
    __stack_start = .;
    . += 0x4000;
    /*. = ALIGN(4096);*/
    . = ALIGN(2M);
    __stack_top = . - 8;
    __stack_size = (__stack_top-8) - __stack_start;    

	__pg_tbl_start = .;
    . += 8M;
    __pg_tbl_end = .;
    
	__end = .;
    __prekernel_size = __end - __start;
}
