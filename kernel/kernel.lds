/* See LICENSE for license details. */

OUTPUT_ARCH( "riscv" )

ENTRY( _start )

SECTIONS
{

  /*--------------------------------------------------------------------*/
  /* Code and read-only segment                                         */
  /*--------------------------------------------------------------------*/

  /* Begining of code and text segment, starts from DRAM_BASE to be effective before enabling paging */
  . = 0x80200000;
    /* 将.text.init节放在最前面，这通常包含启动代码 */
	_ftext = .;
	/* 首先放置入口点段 */
  .text.entry : {
    *(.text.entry)
  }

  
  /* text: Program code section */
  .text : 
  {
		. = ALIGN(4); /* 4 字节对齐 */
    *(.text)
    *(.text.*)
    *(.gnu.linkonce.t.*)
    . = ALIGN(0x1000);

    _trap_sec_start = .;
    *(trapsec)
    . = ALIGN(0x1000);
  /*   ASSERT(. - _trap_sec_start == 0x1000, "error: trap section larger than one page");   */
  }

  /* rodata: Read-only data */
  .rodata : 
  {
    *(.rdata)
    *(.rodata)
    *(.rodata.*)
    *(.gnu.linkonce.r.*)
  }

  /* End of code and read-only segment */
  . = ALIGN(0x1000);
  _etext = .;

  /*--------------------------------------------------------------------*/
  /* HTIF, isolated onto separate page                                  */
  /*--------------------------------------------------------------------*/
  .htif :
  {
    PROVIDE( __htif_base = . );
    *(.htif)
  }
  . = ALIGN(0x1000);

  /*--------------------------------------------------------------------*/
  /* Initialized data segment                                           */
  /*--------------------------------------------------------------------*/

  /* Start of initialized data segment */
  . = ALIGN(16);
   _fdata = .;

  /* data: Writable data */
  .data : 
  {
    *(.data)
    *(.data.*)
    *(.srodata*)
    *(.gnu.linkonce.d.*)
    *(.comment)
  }

  /* End of initialized data segment */
  . = ALIGN(16);
  _edata = .;

  /*--------------------------------------------------------------------*/
  /* Uninitialized data segment                                         */
  /*--------------------------------------------------------------------*/

  /* Start of uninitialized data segment */
  . = .;
  _fbss = .;

  /* sbss: Uninitialized writeable small data section */
  . = .;

  /* bss: Uninitialized writeable data section */
  . = .;
  _bss_start = .;
  .bss : 
  {
    *(.bss)
    *(.bss.*)
    *(.sbss*)
    *(.gnu.linkonce.b.*)
    *(COMMON)
  }

  . = ALIGN(0x1000);
  _end = .;
}