/* STM32F407 Linker Script */
/* FLASH: 1MB @ 0x08000000, RAM: 128KB @ 0x20000000 */

MEMORY
{
    FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 1024K
    SRAM (rwx)      : ORIGIN = 0x20000000, LENGTH = 128K
    CCM (rwx)       : ORIGIN = 0x10000000, LENGTH = 64K
}

SECTIONS
{
    .text :
    {
        KEEP(*(.vector_table))
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .data :
    {
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > SRAM AT > FLASH
    _ldata = LOADADDR(.data);

    .bss :
    {
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > SRAM

    .ccm (NOLOAD) :
    {
        _sccm = .;
        *(.ccm*)
        . = ALIGN(4);
        _eccm = .;
    } > CCM

    /* Heap and stack */
    .heap (NOLOAD) :
    {
        PROVIDE(__heap_start = .);
        . = . + 4K;
        PROVIDE(__heap_end = .);
    } > SRAM

    .stack (NOLOAD) :
    {
        . = ALIGN(8);
        _estack = .;
    } > SRAM
}

ENTRY(Reset_Handler)
