OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)

ENTRY(_start)

SECTIONS
{
  .text : ALIGN(4)
  {
   
    *(.text .text.* )
      
  } > TEXT 

  . = ALIGN(4);
  _etext = .;
  __etext = .;

  .data : ALIGN(4)
  {
    *(.data_begin .data_begin.*)
    *(.data .data.* .gnu.linkonce.d.*)
    *(.data_end .data_end.*)
    
    *(.sdata2 .sdata2.* .gnu.linkonce.s2.*)
    
    SORT(CONSTRUCTORS)
    
    *(.srodata.cst16) *(.srodata.cst8) *(.srodata.cst4) *(.srodata.cst2) *(.srodata .srodata.*)
    *(.sdata .sdata.* .gnu.linkonce.s.*)
    *(.tdata .tdata.* .gnu.linkonce.td.*)

  } > DATA

  .bss (NOLOAD) : ALIGN(4)
  {
    *(.bss_begin .bss_begin.*)
    *(.bss .bss.*)
    *(.gnu.linkonce.b.*)
    *(COMMON)
    *(.bss_end .bss_end.*)
    
    *(.dynsbss)
    *(.sbss .sbss.* .gnu.linkonce.sb.*)
    *(.scommon)
    *(.dynbss)
  
  } > DATA
  
  __image_end     = LOADADDR(.data) + SIZEOF(.data);
  __app_image_end = LOADADDR(.data) + SIZEOF(.data);
  

  /DISCARD/ : {
      *(.ARM.exidx*)
      *(.ARM.extab*)
  }

}
