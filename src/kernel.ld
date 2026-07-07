ENTRY(_start)

SECTIONS {
	. = 0x100000;              /* physical load address */

	.text : {
		KEEP(*(.text.start))
		*(.text*)
	}

	.rodata : { *(.rodata*) }
	.data  :  { *(.data*) }
	.bss  :   { *(.bss*)  *(COMMON) }

	/* Place page tables after the kernel, still under 2 MiB */
	. = ALIGN(4096);
	.pagetables : { *(.pagetables) }

/* 	. = ALIGN(4096); */
	. = 0x200000;
  .stack      : { *(.stack) }

  /* Everything must fit under 2 MiB for now.
     If not, extend the mapping (add more PT/PD/PDPT) before the mov %cr3. */
}
