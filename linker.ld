/**
 * ==============================================================================
 * Project: ZeroHAL
 * File: linker.ld
 * Description: Bare-metal Linker Script for STM32F4 (ARM Cortex-M4)
 * 
 * Target Architecture: ARMv7E-M (Cortex-M4F)
 * Target Hardware: STM32F4 series (e.g., STM32F401, STM32F411, STM32F446)
 * ==============================================================================
 * 
 * In safety-critical embedded systems (ISO 26262), every byte of memory must be
 * deterministically accounted for. This linker script defines the physical
 * layout of non-volatile Flash memory and volatile SRAM.
 */

/* Entry Point: Execution begins at the Reset Handler */
ENTRY(Reset_Handler)

/* End of Stack: Full descending stack pointing to the very top of SRAM */
_estack = ORIGIN(SRAM) + LENGTH(SRAM);

/* Required Minimum Sizes for Heap and Stack */
_Min_Leftover_RAM = 0x400; /* 1KB guard band */

/* ==============================================================================
 * MEMORY REGIONS (Physical Silicon Boundaries)
 * ==============================================================================
 * FLASH: Non-volatile storage starting at 0x08000000 (aliased to 0x00000000 at boot)
 * SRAM:  High-speed volatile RAM starting at 0x20000000
 */
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
    SRAM  (rwx) : ORIGIN = 0x20000000, LENGTH = 96K
}

/* ==============================================================================
 * OUTPUT SECTIONS (Mapping object files into physical memory)
 * ============================================================================== */
SECTIONS
{
    /* --------------------------------------------------------------------------
     * 1. THE VECTOR TABLE (.isr_vector)
     * --------------------------------------------------------------------------
     * The ARM Cortex-M4 CPU hardware expects the Vector Table at the lowest address
     * of Flash (0x08000000). The first 32-bit word is the initial Stack Pointer,
     * and the second 32-bit word is the address of the Reset_Handler.
     */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector)) /* KEEP prevents the linker from discarding it with --gc-sections */
        . = ALIGN(4);
    } > FLASH

    /* --------------------------------------------------------------------------
     * 2. EXECUTABLE CODE & CONSTANTS (.text & .rodata)
     * --------------------------------------------------------------------------
     * All compiled C machine instructions and string literals reside here.
     */
    .text :
    {
        . = ALIGN(4);
        *(.text)           /* .text sections (code) */
        *(.text*)          /* .text* sections (code from all input files) */
        *(.glue_7)         /* Glue arm to thumb code */
        *(.glue_7t)        /* Glue thumb to arm code */
        *(.eh_frame)

        KEEP (*(.init))
        KEEP (*(.fini))

        . = ALIGN(4);
        _etext = .;        /* Global symbol marking the end of executable code */
    } > FLASH

    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)         /* Read-only constants, string literals, lookup tables */
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    /* ARM exception unwinding tables (needed for C/C++ cross-compilation) */
    .ARM.extab : { 
        *(.ARM.extab* .gnu.linkonce.armextab.*) 
    } > FLASH

    .ARM : {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH

    /* --------------------------------------------------------------------------
     * 3. INITIALIZED DATA (.data)
     * --------------------------------------------------------------------------
     * Variables with initial non-zero values (e.g., int counter = 42;).
     * 
     * DUAL-ADDRESSING CONCEPT:
     * - LMA (Load Memory Address): Lives in FLASH so values survive power loss.
     * - VMA (Virtual Memory Address): Lives in SRAM so the CPU can read/write them.
     * The Reset_Handler copies bytes from LMA (Flash) to VMA (SRAM) at startup!
     */
    _sidata = LOADADDR(.data); /* Start address of .data image in FLASH */

    .data : 
    {
        . = ALIGN(4);
        _sdata = .;        /* Start of .data in SRAM */
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;        /* End of .data in SRAM */
    } > SRAM AT > FLASH

    /* --------------------------------------------------------------------------
     * 4. ZERO-INITIALIZED DATA (.bss)
     * --------------------------------------------------------------------------
     * Variables that start at zero or are uninitialized (e.g., static char buf[64];).
     * They do NOT take space in Flash. At startup, the Reset_Handler zeroes out
     * the SRAM region between _sbss and _ebss.
     */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;         /* Start of .bss in SRAM */
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;         /* End of .bss in SRAM */
        __bss_end__ = _ebss;
    } > SRAM

    /* --------------------------------------------------------------------------
     * 5. STACK OVERFLOW SAFETY GUARD
     * --------------------------------------------------------------------------
     * Verify there is enough RAM left for the stack between .bss and end of SRAM.
     */
    ._check_stack :
    {
        . = ALIGN(4);
        . = . + _Min_Leftover_RAM;
        . = ALIGN(4);
    } > SRAM

    /* Discard debug information not needed on silicon */
    /DISCARD/ :
    {
        libc.a ( * )
        libm.a ( * )
        libgcc.a ( * )
    }
}
