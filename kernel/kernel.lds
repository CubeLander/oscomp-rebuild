/* See LICENSE for license details. */

OUTPUT_ARCH( "riscv" )

ENTRY( _entry )

/* 定义高地址基础值和偏移量 */
_HIGH_ADDR_BASE = 0xff000000;
_LOAD_ADDR_BASE = 0x80000000;

SECTIONS
{
  /*--------------------------------------------------------------------*/
  /* Boot code and data (低地址区域，直接映射)                          */
  /*--------------------------------------------------------------------*/

  /* 从SBI加载内核的位置开始 */
  . = 0x80200000;
	  /* 记录物理内存起始位置 */
  _kernel_phys_start = .;
  
  /* 计算当前加载地址相对于基准加载地址的偏移量 */
  _load_offset = _kernel_phys_start - _LOAD_ADDR_BASE;
	
  	/* 首先放置入口点段 */
  .text.entry : {
    *(.text.entry)
  }


  . = ALIGN(0x1000);  /* 确保页对齐 */
  /* 为早期页表分配预留额外的物理页 */
  _early_boot_page_pool = .;
  . += 0x10000;  /* 预留16页 = 64KB，可以根据需求调整 */
  _early_boot_page_pool_end = .;
  
  /* 启动代码区域 */
  .boot_text : {
    _boot_text_start = .;
    *(.boot_text)   /* 添加的boot_text段 */
    *(.boot_text.*)
    . = ALIGN(16);
    _boot_text_end = .;
  }
  
  /* 启动数据区域 */
  .boot_data : {
    _boot_data_start = .;
    *(.boot_data)   /* 添加的boot_data段 */
    *(.boot_data.*)
    *(.boot_rodata) /* 启动期间只读数据 */
    *(.boot_rodata.*)
    . = ALIGN(16);
    _boot_data_end = .;
  }
  . = ALIGN(0x1000);  /* 确保页对齐 */
  /* 记录低地址区域结束位置 */
  _boot_end = .;
  /* 为页表预留物理空间 */
  _g_pagetable_pa = .;
  _kernel_code_offset = .;
  
  /*--------------------------------------------------------------------*/
  /* 内核主体代码和数据 (高地址区域)                                    */
  /*--------------------------------------------------------------------*/
  . = ALIGN(0x1000);  /* 确保页对齐 */
  /* 切换到高地址区域 - 基于实际加载位置计算虚拟地址起始位置 */
  . = _HIGH_ADDR_BASE + _kernel_code_offset - _LOAD_ADDR_BASE;
  _g_pagetable_va = .;
  /* 为早期引导页表在虚拟空间预留位置 */
  . += 0x1000;  /* 预留一页 */
  
  /* Code and read-only segment */
  . = ALIGN(0x1000);
  _ftext = .;
  _text_phys = _ftext - _HIGH_ADDR_BASE + _LOAD_ADDR_BASE;
  /* text: Program code section */
  .text : AT(_text_phys) {
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
	_frodata = .;
  _rodata_phys = _frodata - _HIGH_ADDR_BASE + _LOAD_ADDR_BASE;
  /* rodata: Read-only data */
  .rodata : AT(_rodata_phys){
    *(.rdata)
    *(.rodata)
    *(.rodata.*)
    *(.gnu.linkonce.r.*)
  }

  /* End of code and read-only segment */
  . = ALIGN(0x1000);
  _etext = .;


  /*--------------------------------------------------------------------*/
  /* Initialized data segment                                           */
  /*--------------------------------------------------------------------*/

  /* Start of initialized data segment */
  . = ALIGN(16);
  _fdata = .;

  _data_phys = _fdata - _HIGH_ADDR_BASE + _LOAD_ADDR_BASE;
  /* data: Writable data */
  .data :AT(_data_phys) {
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

  /* bss: Uninitialized writeable data section */
  . = .;
  _bss_start = .;
  _bss_phys = _fbss - _HIGH_ADDR_BASE + _LOAD_ADDR_BASE;

  .bss :AT(_bss_phys) {
    *(.bss)
    *(.bss.*)
    *(.sbss*)
    *(.gnu.linkonce.b.*)
    *(COMMON)
  }

  . = ALIGN(0x1000);
  _end = .;
  
  /* 记录虚拟地址结束位置 */
  _kernel_virt_end = .;
  
}