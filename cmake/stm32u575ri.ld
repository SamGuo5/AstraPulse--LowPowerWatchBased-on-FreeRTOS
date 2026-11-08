ENTRY(Reset_Handler)
MEMORY
{
  /* Firmware is intentionally confined to Bank 1.  Bank 2 remains outside
     the link image; its last two 8 KiB pages hold the settings journal. */
  ROM (rx)  : ORIGIN = 0x08000000, LENGTH = 1024K
  RAM (xrw) : ORIGIN = 0x20000000, LENGTH = 768K
}
_estack = ORIGIN(RAM) + LENGTH(RAM);
_Min_Heap_Size = 0x10000;
_Min_Stack_Size = 0x2000;
SECTIONS
{
  .isr_vector : { . = ALIGN(8); KEEP(*(.isr_vector)) . = ALIGN(8); } > ROM
  .text : {
    . = ALIGN(8); *(.text) *(.text*) *(.glue_7) *(.glue_7t) *(.eh_frame)
    KEEP(*(.init)) KEEP(*(.fini)) . = ALIGN(8); _etext = .;
  } > ROM
  .rodata : { . = ALIGN(8); *(.rodata) *(.rodata*) . = ALIGN(8); } > ROM
  .ARM.extab (READONLY) : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > ROM
  .ARM (READONLY) : { __exidx_start = .; *(.ARM.exidx*) __exidx_end = .; } > ROM
  .preinit_array (READONLY) : {
    PROVIDE_HIDDEN(__preinit_array_start = .); KEEP(*(.preinit_array*))
    PROVIDE_HIDDEN(__preinit_array_end = .);
  } > ROM
  .init_array (READONLY) : {
    PROVIDE_HIDDEN(__init_array_start = .); KEEP(*(SORT(.init_array.*))) KEEP(*(.init_array*))
    PROVIDE_HIDDEN(__init_array_end = .);
  } > ROM
  .fini_array (READONLY) : {
    PROVIDE_HIDDEN(__fini_array_start = .); KEEP(*(SORT(.fini_array.*))) KEEP(*(.fini_array*))
    PROVIDE_HIDDEN(__fini_array_end = .);
  } > ROM
  _sidata = LOADADDR(.data);
  .data : {
    . = ALIGN(8); _sdata = .; *(.data) *(.data*) . = ALIGN(8); _edata = .;
  } > RAM AT > ROM
  .bss : {
    . = ALIGN(8); _sbss = .; __bss_start__ = .; *(.bss) *(.bss*) *(COMMON)
    . = ALIGN(8); _ebss = .; __bss_end__ = .;
  } > RAM
  ._user_heap_stack : {
    . = ALIGN(8); PROVIDE(end = .); PROVIDE(_end = .); __heap_start = .;
    . = . + _Min_Heap_Size; __heap_limit = .; . = . + _Min_Stack_Size; . = ALIGN(8);
  } > RAM
  .ARM.attributes 0 : { *(.ARM.attributes) }
}
