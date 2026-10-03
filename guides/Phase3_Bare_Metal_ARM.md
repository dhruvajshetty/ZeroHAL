# Phase 3: Removing the Training Wheels
**Bare-Metal ARM Cortex-M Programming — The Complete Engineer's Reference**

This document takes you from Arduino abstractions to direct silicon control. You will understand every layer between your C code and the physical transistors in the chip. By the end, you will be able to pick up any ARM Cortex-M microcontroller, read its reference manual, and write a complete firmware from scratch — no vendor HAL, no libraries, no training wheels.

---

# PART I: THE ARM CORTEX-M ARCHITECTURE

## Chapter 1: Why ARM?

ARM (Advanced RISC Machines) doesn't manufacture chips. They design CPU architectures and license them to hundreds of companies. This means the same core instruction set and programming model works across chips from:

| Manufacturer | Popular Chip Families | Market |
|---|---|---|
| STMicroelectronics | STM32F0/F1/F4/F7/H7/L4 | Industrial, automotive, consumer |
| NXP (Freescale) | LPC, Kinetis, i.MX RT | Automotive, IoT |
| Texas Instruments | MSP432, TM4C (Tiva-C) | Industrial, education |
| Microchip (Atmel) | SAM D/E/S/V series | IoT, consumer |
| Nordic Semiconductor | nRF52/nRF53/nRF91 | Bluetooth Low Energy, cellular IoT |
| Espressif | ESP32-S3/C3 (RISC-V + ARM) | WiFi/BLE IoT |
| Raspberry Pi | RP2040 (Dual Cortex-M0+) | Maker, education |

**Learn one ARM Cortex-M, and you can program all of them.** The peripheral register addresses change, but the CPU core, interrupt model, debugging interface, and programming concepts are identical.

### The Cortex-M Family

| Core | Bits | Pipeline | FPU | Target | Example Chips |
|---|---|---|---|---|---|
| Cortex-M0 | 32 | 3-stage | No | Ultra-low cost (\$0.30) | STM32F0, nRF51 |
| Cortex-M0+ | 32 | 2-stage | No | Ultra-low power | RP2040, SAM D21 |
| Cortex-M3 | 32 | 3-stage | No | General purpose | STM32F1, LPC1768 |
| Cortex-M4 | 32 | 3-stage | Optional (SP) | DSP + motor control | STM32F4, nRF52840 |
| Cortex-M7 | 32 | 6-stage | Yes (DP) | High performance | STM32F7/H7, i.MX RT |
| Cortex-M33 | 32 | 3-stage | Optional | Security (TrustZone) | STM32L5, nRF5340 |
| Cortex-M55 | 32 | — | Yes | AI/ML at the edge | — |

SP = Single Precision floating point. DP = Double Precision. DSP = Digital Signal Processing instructions.

---

## Chapter 2: The CPU Core — Registers and Execution

### The Programmer's Model

The Cortex-M4 CPU has **16 general-purpose registers** (R0-R15) and several special registers. Every instruction operates on these registers.

```
    ┌─────────────────────────────────────────────┐
    │          ARM Cortex-M4 Register File         │
    ├─────────────────────────────────────────────┤
    │  R0  │ General purpose / Function argument 1 │
    │  R1  │ General purpose / Function argument 2 │
    │  R2  │ General purpose / Function argument 3 │
    │  R3  │ General purpose / Function argument 4 │
    │  R4  │ General purpose (callee-saved)         │
    │  R5  │ General purpose (callee-saved)         │
    │  R6  │ General purpose (callee-saved)         │
    │  R7  │ General purpose (callee-saved)         │
    │  R8  │ General purpose (callee-saved)         │
    │  R9  │ General purpose (callee-saved)         │
    │  R10 │ General purpose (callee-saved)         │
    │  R11 │ General purpose (callee-saved)         │
    │  R12 │ Intra-Procedure-call scratch register  │
    │  R13 │ SP  — Stack Pointer                    │
    │  R14 │ LR  — Link Register (return address)   │
    │  R15 │ PC  — Program Counter (current instr)  │
    ├─────────────────────────────────────────────┤
    │ xPSR │ Program Status Register (flags: N,Z,C,V)│
    │PRIMASK│ Interrupt mask (disable all IRQs)      │
    │BASEPRI│ Priority-based interrupt masking       │
    │CONTROL│ Stack pointer selection, privilege      │
    └─────────────────────────────────────────────┘
```

**Key Registers Explained:**

*   **R0-R3:** Used to pass the first 4 arguments to a function and return values. The compiler follows the ARM AAPCS (Procedure Call Standard).
*   **R4-R11:** "Callee-saved" — if a function uses these, it must save their original values to the stack and restore them before returning.
*   **R13 / SP (Stack Pointer):** Points to the top of the stack in SRAM. The stack grows *downward* (from high addresses to low addresses). Every function call pushes data onto the stack; every return pops it off.
*   **R14 / LR (Link Register):** When you call a function with `BL` (Branch with Link), the CPU saves the return address in LR so it knows where to go back to.
*   **R15 / PC (Program Counter):** Contains the address of the *next* instruction to execute. Writing to PC causes a jump/branch.
*   **xPSR (Program Status Register):** Contains the ALU condition flags:
    *   **N** (Negative): Result was negative
    *   **Z** (Zero): Result was zero
    *   **C** (Carry): Unsigned overflow occurred
    *   **V** (oVerflow): Signed overflow occurred

### Thumb-2 Instruction Set

Cortex-M processors use the **Thumb-2** instruction set — a mix of 16-bit and 32-bit instructions. This provides good code density (small binary size) while maintaining the performance of 32-bit operations.

```assembly
    ; Example: Add two numbers and store result
    LDR   R0, =0x40020014    ; Load address of GPIOA_ODR into R0
    LDR   R1, [R0]           ; Read the 32-bit value at that address into R1
    ORR   R1, R1, #(1 << 5)  ; Set bit 5 (OR with bitmask)
    STR   R1, [R0]           ; Write modified value back to GPIOA_ODR

    ; This is EXACTLY what your C code does:
    ;   GPIOA_ODR |= (1 << 5);
```

The compiler translates every line of C into sequences like this. Understanding this helps you debug hard faults and optimize critical code paths.

### The Pipeline

The Cortex-M4 has a **3-stage pipeline**: Fetch → Decode → Execute.

```
    Clock Cycle:    1     2     3     4     5     6
                  ┌─────┬─────┬─────┬─────┬─────┬─────┐
    Instruction 1 │Fetch│Decode│ Exec│     │     │     │
    Instruction 2 │     │Fetch│Decode│ Exec│     │     │
    Instruction 3 │     │     │Fetch│Decode│ Exec│     │
    Instruction 4 │     │     │     │Fetch│Decode│ Exec│
                  └─────┴─────┴─────┴─────┴─────┴─────┘
```

After the pipeline fills (3 cycles), one instruction completes *every clock cycle*. This is why the PC is always 4 bytes ahead of the currently executing instruction (it's already fetching future instructions).

**Pipeline Hazard — Branching:** When the CPU encounters a branch (like an `if` statement), it doesn't know which instruction comes next until the branch is evaluated. The pipeline must be flushed and refilled, wasting 1-2 cycles. This is called a **pipeline stall** or **branch penalty**.

### Processor Modes

The Cortex-M has two execution modes:

| Mode | When Active | Privilege | Stack |
|---|---|---|---|
| **Thread Mode** | Normal code execution (your `main()` and application code) | Privileged or Unprivileged | MSP or PSP |
| **Handler Mode** | Inside an exception/interrupt handler (ISR) | Always Privileged | Always MSP |

*   **MSP (Main Stack Pointer):** Used by the OS kernel and interrupt handlers.
*   **PSP (Process Stack Pointer):** Used by application tasks (in an RTOS, each task gets its own PSP).

For bare-metal without an RTOS, you typically use MSP for everything.

---

## Chapter 3: The Memory Map

The ARM Cortex-M defines a **fixed, standardized memory map**. Every Cortex-M chip uses the same address ranges for the same types of memory. This is one of ARM's greatest design decisions — it means your knowledge transfers across all vendors.

```
    0xFFFFFFFF ┌──────────────────────────────┐
               │    Vendor-Specific           │
    0xE0100000 ├──────────────────────────────┤
               │    Private Peripheral Bus    │  ← NVIC, SysTick, SCB, Debug
               │    (System Control Space)    │     (Same on ALL Cortex-M chips)
    0xE0000000 ├──────────────────────────────┤
               │    External Device           │
    0xA0000000 ├──────────────────────────────┤
               │    External RAM              │  ← SDRAM (if present)
    0x60000000 ├──────────────────────────────┤
               │    Peripheral                │  ← APB1, APB2, AHB1, AHB2, AHB3
               │    (GPIO, UART, SPI, I2C,    │     peripherals. Vendor-specific
               │     Timers, ADC, DMA, etc.)  │     addresses within this range.
    0x40000000 ├──────────────────────────────┤
               │    SRAM                      │  ← Working RAM (variables, stack,
               │    (up to 512 MB region)     │     heap). STM32F4: 112KB + 16KB
    0x20000000 ├──────────────────────────────┤
               │    Code (Flash)              │  ← Program storage. STM32F4: up to
               │    (up to 512 MB region)     │     1MB. Executes in place (XIP).
    0x00000000 └──────────────────────────────┘
```

### STM32F4 Specific Memory Map (Peripheral Region Detail)

```
    0x50060800 ┌────────────────────────────┐
               │  AHB2: USB OTG FS          │
    0x50000000 ├────────────────────────────┤
               │  AHB1: DMA, GPIO, RCC,     │
               │  Flash Interface, CRC       │
    0x40020000 ├────────────────────────────┤  ← GPIOA starts at 0x40020000
               │  APB2: USART1/6, SPI1,     │     GPIOB starts at 0x40020400
               │  TIM1/8/9/10/11, ADC,      │     GPIOC starts at 0x40020800
               │  SYSCFG, EXTI              │     ...
    0x40010000 ├────────────────────────────┤
               │  APB1: USART2/3, SPI2/3,   │  ← USART2 starts at 0x40004400
               │  I2C1/2/3, TIM2/3/4/5,     │     I2C1 starts at 0x40005400
               │  IWDG, WWDG, PWR, DAC      │     TIM2 starts at 0x40000000
    0x40000000 └────────────────────────────┘
```

**This is why you write `#define USART2_BASE 0x40004400` in your code.** The silicon manufacturer physically wired the USART2 peripheral's control registers to respond at those memory addresses.

### Memory-Mapped I/O: The Key Insight

When you write:
```c
*((volatile uint32_t *)0x40020014) = 0x00000020;
```

The CPU generates a **store** instruction to address `0x40020014`. The bus matrix looks at this address and says: *"That's in the AHB1 peripheral range. Route this write to the GPIOA peripheral."* The GPIOA hardware block receives the data (`0x00000020` = bit 5 set) and physically changes the voltage on Pin 5.

**There is no difference between accessing RAM and accessing hardware.** The CPU uses the same `LDR`/`STR` instructions. The bus matrix routes the access to the correct destination based on the address.

---

## Chapter 4: The Bus Matrix (AHB/APB)

The bus matrix is the "highway system" inside the chip that connects the CPU to all peripherals and memories.

```
    ┌──────────┐     ┌─────────────────────────────────────────┐
    │          │     │            BUS MATRIX                    │
    │   CPU    │────▶│  AHB (Advanced High-performance Bus)    │
    │ Cortex-M4│     │  ┌──────────────────────────────────┐   │
    │          │     │  │ AHB1: GPIO, DMA, RCC, Flash      │   │
    └──────────┘     │  │       (Full CPU speed: 168 MHz)  │   │
                     │  └──────────────────────────────────┘   │
    ┌──────────┐     │  ┌──────────────────────────────────┐   │
    │  Flash   │────▶│  │ AHB2: USB OTG, RNG, Camera      │   │
    │ (1 MB)   │     │  └──────────────────────────────────┘   │
    └──────────┘     │                                         │
                     │  ┌──────────────────────────────────┐   │
    ┌──────────┐     │  │ APB2: USART1/6, SPI1, ADC,      │   │
    │  SRAM    │────▶│  │  TIM1/8, SYSCFG, EXTI            │   │
    │ (128 KB) │     │  │  (Up to 84 MHz)                  │   │
    └──────────┘     │  └──────────────────────────────────┘   │
                     │                                         │
    ┌──────────┐     │  ┌──────────────────────────────────┐   │
    │   DMA1   │────▶│  │ APB1: USART2/3, SPI2/3, I2C,    │   │
    │   DMA2   │     │  │  TIM2/3/4/5, IWDG, PWR, DAC     │   │
    └──────────┘     │  │  (Up to 42 MHz)                  │   │
                     │  └──────────────────────────────────┘   │
                     └─────────────────────────────────────────┘
```

**Why multiple buses?**
*   **AHB (Advanced High-performance Bus):** Runs at the full CPU clock speed. Used for high-bandwidth peripherals (GPIO, DMA, Flash).
*   **APB (Advanced Peripheral Bus):** Runs at a fraction of the CPU clock. Used for slower peripherals (UART, I2C, SPI, Timers). This reduces power consumption and simplifies peripheral design.
*   **APB1 vs APB2:** APB2 runs faster (84 MHz vs 42 MHz on STM32F4). Higher-speed peripherals like USART1 and ADC are on APB2.

**The bus speed matters for baud rate calculation.** When you set USART2's baud rate register, the formula uses the APB1 clock (not the CPU clock) because USART2 is on the APB1 bus.

---

# PART II: THE BOOT PROCESS

## Chapter 5: What Happens at Power-On

When you apply power to an ARM Cortex-M chip, a deterministic boot sequence executes:

### Step 1: Hardware Reset

The reset circuit holds the CPU in reset for a few milliseconds while the power supply stabilizes. Once the voltage reaches the threshold, the reset line is released.

### Step 2: Vector Table Fetch

The CPU reads two 32-bit values from the very beginning of Flash memory (address `0x00000000`):

```
    Flash Address    Contents              Purpose
    ──────────────   ─────────────────     ──────────────────────────
    0x00000000       0x20020000            Initial Stack Pointer (top of SRAM)
    0x00000004       0x08000141            Reset Handler address (entry point)
    0x00000008       0x08000XXX            NMI Handler address
    0x0000000C       0x08000XXX            HardFault Handler address
    0x00000010       0x08000XXX            MemManage Handler address
    0x00000014       0x08000XXX            BusFault Handler address
    0x00000018       0x08000XXX            UsageFault Handler address
    ...              ...                   ...
    0x0000003C       0x08000XXX            SysTick Handler address
    0x00000040       0x08000XXX            IRQ 0 (Window Watchdog)
    0x00000044       0x08000XXX            IRQ 1 (PVD)
    ...              ...                   ...
    0x000000D8       0x08000XXX            IRQ 38 (USART2)
    ...
```

*   The CPU loads the value at `0x00000000` into the **Stack Pointer (SP/R13)**.
*   The CPU loads the value at `0x00000004` into the **Program Counter (PC/R15)** and begins executing from there.

### Step 3: Reset Handler Executes

The Reset Handler is the true entry point of your firmware. It must:

1. Copy initialized global variables from Flash to SRAM (the `.data` section).
2. Zero-fill uninitialized global variables in SRAM (the `.bss` section).
3. Optionally initialize the FPU (Floating Point Unit).
4. Call `main()`.

### The Vector Table in C

```c
// This array is placed at the very start of Flash by the linker script
__attribute__((section(".isr_vector")))
const uint32_t vector_table[] = {
    (uint32_t)&_estack,          // 0x00: Initial Stack Pointer
    (uint32_t)Reset_Handler,     // 0x04: Reset
    (uint32_t)NMI_Handler,       // 0x08: Non-Maskable Interrupt
    (uint32_t)HardFault_Handler, // 0x0C: Hard Fault
    (uint32_t)MemManage_Handler, // 0x10: Memory Management Fault
    (uint32_t)BusFault_Handler,  // 0x14: Bus Fault
    (uint32_t)UsageFault_Handler,// 0x18: Usage Fault
    0, 0, 0, 0,                  // 0x1C-0x28: Reserved
    (uint32_t)SVC_Handler,       // 0x2C: Supervisor Call
    (uint32_t)DebugMon_Handler,  // 0x30: Debug Monitor
    0,                           // 0x34: Reserved
    (uint32_t)PendSV_Handler,    // 0x38: PendSV (used by RTOS)
    (uint32_t)SysTick_Handler,   // 0x3C: SysTick Timer
    // External Interrupts (vendor-specific, from here on):
    (uint32_t)WWDG_IRQHandler,         // IRQ 0
    (uint32_t)PVD_IRQHandler,          // IRQ 1
    // ... up to IRQ 81 for STM32F4
    (uint32_t)USART2_IRQHandler,       // IRQ 38
    // ...
};
```

**The `__attribute__((weak))` Pattern:**

In your startup code, every handler is declared as a `weak` symbol that defaults to an infinite loop:

```c
void __attribute__((weak)) USART2_IRQHandler(void) {
    while (1); // Default: hang if unhandled
}
```

When you define `USART2_IRQHandler` in your `main.c`, the linker replaces the weak default with your real implementation. This is how your ISR gets called without any explicit registration — its address is in the vector table at compile time.

---

## Chapter 6: The Startup Code

### The Reset Handler (Complete Implementation)

```c
// Symbols defined by the linker script
extern uint32_t _sidata;  // Start of .data initializers in Flash
extern uint32_t _sdata;   // Start of .data section in SRAM
extern uint32_t _edata;   // End of .data section in SRAM
extern uint32_t _sbss;    // Start of .bss section in SRAM
extern uint32_t _ebss;    // End of .bss section in SRAM
extern uint32_t _estack;  // Top of stack (highest SRAM address)

// External entry point
extern int main(void);

void Reset_Handler(void) {
    // Step 1: Copy .data section from Flash to SRAM
    // Initialized global variables (like: int counter = 42;)
    // are stored in Flash. We must copy their initial values to SRAM
    // so the program can read and modify them at runtime.
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;
    while (dst < &_edata) {
        *dst++ = *src++;
    }

    // Step 2: Zero-fill the .bss section in SRAM
    // Uninitialized globals (like: int count;) must start at 0
    // per the C standard. Flash doesn't store zeros (waste of space),
    // so we fill them ourselves.
    dst = &_sbss;
    while (dst < &_ebss) {
        *dst++ = 0;
    }

    // Step 3: Enable the FPU (Cortex-M4F only)
    // The FPU is disabled by default. If any code uses float/double,
    // a UsageFault will fire unless we enable it here.
    // CPACR register: set CP10 and CP11 to Full Access (0b11)
    *((volatile uint32_t *)0xE000ED88) |= (0xF << 20);

    // Step 4: Call the application entry point
    main();

    // Step 5: If main() ever returns, hang here
    while (1);
}
```

### Why .data and .bss Matter

```c
// CASE 1: Initialized global variable
int sensor_offset = 42;
// → Stored in .data section
// → Initial value (42) is stored in Flash
// → At boot, Reset_Handler copies 42 from Flash to SRAM
// → At runtime, sensor_offset lives in SRAM (readable AND writable)

// CASE 2: Uninitialized global variable
int error_count;
// → Stored in .bss section
// → No space used in Flash (we just need to know the address and size)
// → At boot, Reset_Handler fills this with 0
// → At runtime, error_count = 0, lives in SRAM

// CASE 3: const global variable
const char firmware_version[] = "v2.1";
// → Stored in .rodata section (read-only data)
// → Lives in Flash permanently. Never copied to SRAM.
// → Writing to this causes a HardFault (Flash is read-only at runtime)

// CASE 4: Local variable
void foo(void) {
    int temp = 10;
    // → Stored on the STACK (in SRAM)
    // → Created when foo() is called, destroyed when foo() returns
    // → The compiler uses registers (R0-R12) when possible for speed
}
```

---

## Chapter 7: The Linker Script

The linker script tells the linker where to place each section of your compiled code in the chip's memory. Without it, the linker doesn't know how much Flash or SRAM the chip has, or where they begin.

### Complete Linker Script (STM32F411, annotated)

```ld
/* Define the chip's memory regions */
MEMORY
{
    /* Flash: 512KB starting at 0x08000000 */
    /* STM32 maps Flash at both 0x00000000 and 0x08000000. */
    /* We use 0x08000000 (the physical address). */
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K

    /* SRAM: 128KB starting at 0x20000000 */
    SRAM  (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

/* The initial stack pointer = top of SRAM */
_estack = ORIGIN(SRAM) + LENGTH(SRAM);  /* = 0x20020000 */

/* Entry point symbol (used by debugger) */
ENTRY(Reset_Handler)

SECTIONS
{
    /* === FLASH SECTIONS (read-only, execute) === */

    /* Vector table MUST be first in Flash */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))  /* KEEP prevents garbage collection */
        . = ALIGN(4);
    } > FLASH

    /* Executable code */
    .text :
    {
        . = ALIGN(4);
        *(.text)           /* All compiled functions */
        *(.text*)          /* Including compiler-generated sections */
        *(.glue_7)         /* ARM/Thumb interworking glue */
        *(.glue_7t)
        . = ALIGN(4);
    } > FLASH

    /* Read-only data (const variables, string literals) */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    /* Store the Flash address where .data initializers begin */
    _sidata = LOADADDR(.data);

    /* === SRAM SECTIONS (read-write) === */

    /* Initialized global/static variables */
    /* AT>FLASH means: store the initial values in Flash */
    /* > SRAM means: at runtime, these variables live in SRAM */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;        /* Start of .data in SRAM */
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;        /* End of .data in SRAM */
    } > SRAM AT> FLASH

    /* Zero-initialized global/static variables */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;         /* Start of .bss */
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;         /* End of .bss */
    } > SRAM

    /* Stack and Heap live in remaining SRAM */
    /* The stack starts at _estack and grows DOWN toward .bss */
    /* If the stack grows into .bss, you get a silent stack overflow — */
    /* one of the hardest bugs to diagnose in embedded systems */
}
```

### Memory Layout Visualization

```
    Flash (0x08000000 - 0x0807FFFF)
    ┌──────────────────────────────┐ 0x08000000
    │  .isr_vector (Vector Table)  │  ~0x200 bytes (IRQ table)
    ├──────────────────────────────┤
    │  .text (Code)                │  Your compiled functions
    ├──────────────────────────────┤
    │  .rodata (Constants)         │  const char[], lookup tables
    ├──────────────────────────────┤
    │  .data initializers          │  Initial values for globals
    ├──────────────────────────────┤
    │  (unused Flash)              │
    └──────────────────────────────┘ 0x0807FFFF

    SRAM (0x20000000 - 0x2001FFFF)
    ┌──────────────────────────────┐ 0x20000000
    │  .data (copied from Flash)   │  Initialized globals
    ├──────────────────────────────┤
    │  .bss (zeroed at boot)       │  Uninitialized globals
    ├──────────────────────────────┤
    │                              │
    │  ↓ Heap (grows UP)           │  malloc() allocations (rarely used)
    │                              │
    │         (free space)         │
    │                              │
    │  ↑ Stack (grows DOWN)        │  Function locals, return addresses
    │                              │
    └──────────────────────────────┘ 0x2001FFFF ← _estack (initial SP)
```

---

# PART III: THE CLOCK SYSTEM

## Chapter 8: Oscillators and the PLL

The clock is the heartbeat of the MCU. Every operation — every instruction, every register read, every peripheral transaction — happens on a clock edge. Getting the clock configuration right is the first thing you must do in any bare-metal firmware.

### Clock Sources on STM32F4

```
    ┌─────────────────────────────────────────────────────────┐
    │                    CLOCK TREE                            │
    │                                                         │
    │  ┌──────────┐                                           │
    │  │ HSI      │ 16 MHz Internal RC Oscillator             │
    │  │ (always  │ Accuracy: ±1% (good enough for UART)      │
    │  │  on at   │ No external components needed             │
    │  │  reset)  ├──────────┐                                │
    │  └──────────┘          │                                │
    │                        ▼                                │
    │  ┌──────────┐     ┌─────────┐     ┌──────────────┐     │
    │  │ HSE      │     │   PLL   │     │  SYSCLK      │     │
    │  │ 8 MHz    │────▶│  (Phase │────▶│  (System     │     │
    │  │ Crystal  │     │  Locked │     │   Clock)     │     │
    │  │ (ext.)   ├────▶│  Loop)  │     │  Up to       │     │
    │  └──────────┘     │         │     │  168 MHz     │     │
    │                   │ M,N,P,Q │     └──────┬───────┘     │
    │  ┌──────────┐     │ dividers│            │              │
    │  │ LSI      │     └─────────┘            │              │
    │  │ 32 kHz   │                            ▼              │
    │  │ Internal │              ┌─────────────────────┐      │
    │  │ (IWDG,   │              │  AHB Prescaler      │      │
    │  │  RTC)    │              │  /1, /2, /4... /512 │      │
    │  └──────────┘              └──────────┬──────────┘      │
    │                                       │                 │
    │  ┌──────────┐              ┌──────────┼──────────┐      │
    │  │ LSE      │              │          │          │      │
    │  │ 32.768   │              ▼          ▼          ▼      │
    │  │ kHz      │         ┌────────┐ ┌────────┐ ┌────────┐ │
    │  │ Crystal  │         │ HCLK   │ │APB1    │ │APB2    │ │
    │  │ (RTC)    │         │168 MHz │ │Prescaler│ │Prescaler││
    │  └──────────┘         │(CPU,   │ │/4=42MHz│ │/2=84MHz││
    │                       │ AHB,   │ │(USART2,│ │(USART1,││
    │                       │ memory)│ │ I2C,   │ │ SPI1,  ││
    │                       └────────┘ │ TIM2-5)│ │ ADC)   ││
    │                                  └────────┘ └────────┘ │
    └─────────────────────────────────────────────────────────┘
```

### HSI (High-Speed Internal)
*   **16 MHz** RC oscillator built into the chip.
*   **Available immediately** at power-on — no setup needed.
*   Accuracy: ±1% (varies with temperature). Good enough for UART but not for USB.
*   Your ZeroHAL project uses HSI because it requires zero external components.

### HSE (High-Speed External)
*   Typically an **8 MHz crystal** soldered to the board.
*   Accuracy: ±20-50 ppm (parts per million) — far more precise than HSI.
*   Required for USB (needs ±0.25% accuracy) and precise timing applications.

### PLL (Phase-Locked Loop)
The PLL is a frequency multiplier. It takes a low input frequency and generates a much higher, stable output frequency.

**STM32F4 PLL Formula:**

$$f_{VCO} = f_{input} \times \frac{N}{M}$$

$$f_{SYSCLK} = \frac{f_{VCO}}{P}$$

$$f_{USB} = \frac{f_{VCO}}{Q}$$

**Example: HSI (16 MHz) → 168 MHz SYSCLK:**

| Parameter | Value | Meaning |
|---|---|---|
| M | 16 | Divide input: 16 MHz / 16 = 1 MHz |
| N | 336 | Multiply VCO: 1 MHz × 336 = 336 MHz |
| P | 2 | Divide for SYSCLK: 336 / 2 = **168 MHz** |
| Q | 7 | Divide for USB: 336 / 7 = **48 MHz** (USB requires exactly 48 MHz) |

### Complete Clock Configuration Code

```c
void clock_init_168mhz(void) {
    // ─── Step 1: Enable HSE and wait for it to stabilize ───
    RCC->CR |= (1 << 16);            // HSEON: Enable HSE
    while (!(RCC->CR & (1 << 17)));   // Wait for HSERDY flag

    // ─── Step 2: Configure Flash wait states ───
    // At 168 MHz, Flash memory needs 5 wait states (it can't keep up)
    // Also enable prefetch buffer and instruction/data caches
    FLASH->ACR = (5 << 0)    // 5 wait states
               | (1 << 8)    // Prefetch enable
               | (1 << 9)    // Instruction cache enable
               | (1 << 10);  // Data cache enable

    // ─── Step 3: Configure PLL ───
    // Source: HSE (8 MHz)
    // M=8, N=336, P=2, Q=7 → SYSCLK = 168 MHz, USB = 48 MHz
    RCC->PLLCFGR = (8 << 0)      // PLLM = 8
                 | (336 << 6)     // PLLN = 336
                 | (0 << 16)      // PLLP = 2 (0b00 = /2)
                 | (1 << 22)      // PLLSRC = HSE
                 | (7 << 24);     // PLLQ = 7

    // ─── Step 4: Enable PLL and wait for lock ───
    RCC->CR |= (1 << 24);            // PLLON
    while (!(RCC->CR & (1 << 25)));   // Wait for PLLRDY

    // ─── Step 5: Configure bus prescalers ───
    RCC->CFGR = (0 << 4)     // AHB prescaler  = /1 (HCLK = 168 MHz)
              | (5 << 10)    // APB1 prescaler = /4 (PCLK1 = 42 MHz)
              | (4 << 13);   // APB2 prescaler = /2 (PCLK2 = 84 MHz)

    // ─── Step 6: Switch SYSCLK source to PLL ───
    RCC->CFGR |= (2 << 0);           // SW = PLL
    while (((RCC->CFGR >> 2) & 3) != 2); // Wait for SWS = PLL

    // CPU is now running at 168 MHz!
}
```

### Flash Wait States

Flash memory is slower than the CPU. At high clock speeds, the CPU must wait for Flash to respond. This is configured via the Flash ACR (Access Control Register).

| SYSCLK Frequency | Required Wait States (at 3.3V) |
|---|---|
| ≤ 30 MHz | 0 |
| ≤ 60 MHz | 1 |
| ≤ 90 MHz | 2 |
| ≤ 120 MHz | 3 |
| ≤ 150 MHz | 4 |
| ≤ 168 MHz | 5 |

**If you forget to set wait states before increasing the clock, the CPU will read corrupted instructions from Flash and crash with a HardFault.**

---

# PART IV: PERIPHERAL DRIVERS

## Chapter 9: GPIO (General-Purpose Input/Output)

GPIO is the most fundamental peripheral. Every pin on the MCU is a GPIO pin that can be configured for various functions.

### GPIO Registers (per port, e.g., GPIOA)

| Register | Offset | Purpose |
|---|---|---|
| MODER | 0x00 | Mode: Input(00), Output(01), Alternate Function(10), Analog(11) |
| OTYPER | 0x04 | Output type: Push-Pull(0) or Open-Drain(1) |
| OSPEEDR | 0x08 | Output speed: Low(00), Medium(01), High(10), Very High(11) |
| PUPDR | 0x0C | Pull-up/Pull-down: None(00), Pull-up(01), Pull-down(10) |
| IDR | 0x10 | Input Data Register (read pin state) |
| ODR | 0x14 | Output Data Register (set pin state) |
| BSRR | 0x18 | Bit Set/Reset Register (atomic set/clear) |
| LCKR | 0x1C | Lock Register (lock pin configuration) |
| AFRL | 0x20 | Alternate Function Low (pins 0-7, 4 bits each) |
| AFRH | 0x24 | Alternate Function High (pins 8-15, 4 bits each) |

### MODER: Pin Mode Configuration

Each pin uses 2 bits in MODER. For a 16-pin port (PA0-PA15), that's a 32-bit register:

```
    MODER Register (32 bits):
    ┌──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┐
    │15│14│13│12│11│10│ 9│ 8│ 7│ 6│ 5│ 4│ 3│ 2│ 1│ 0│ ← Pin number
    └──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┘
    Each pin has 2 bits:
      00 = Input mode
      01 = General purpose output
      10 = Alternate function (UART, SPI, I2C, Timer, etc.)
      11 = Analog mode (ADC input, DAC output)
```

**Setting PA5 as output:**
```c
// Clear bits [11:10] for pin 5, then set to 01 (output)
GPIOA->MODER &= ~(3U << (5 * 2));   // Clear: AND with inverse mask
GPIOA->MODER |=  (1U << (5 * 2));   // Set: OR with value
```

The pattern `(pin * 2)` is because each pin occupies 2 bits.

### Push-Pull vs Open-Drain Output

```
    Push-Pull (Default):
    ┌─────┐
    │ VDD │
    └──┬──┘
       │
    ┌──┴──┐ P-MOS (ON when output = HIGH)
    └──┬──┘
       ├──── Pin Output
    ┌──┴──┐ N-MOS (ON when output = LOW)
    └──┬──┘
       │
    ┌──┴──┐
    │ GND │
    └─────┘

    → Can drive HIGH (VDD) and LOW (GND) actively.
    → Used for: LED driving, SPI, UART TX, general digital output.


    Open-Drain:
    ┌─────┐
    │ VDD │
    └──┬──┘
       │
    [External Pull-up Resistor required]
       │
       ├──── Pin Output
    ┌──┴──┐ N-MOS only
    └──┬──┘
       │
    ┌──┴──┐
    │ GND │
    └─────┘

    → Can only drive LOW (GND) actively.
    → HIGH state = pin floats, pulled up by external resistor.
    → Used for: I2C (SDA, SCL), level shifting, wired-OR buses.
```

**Why I2C requires Open-Drain:** Multiple devices share the same wire. With push-pull, if one device drives HIGH and another drives LOW simultaneously, you get a short circuit (VDD directly connected to GND). With open-drain, devices can only pull the line LOW. If nobody pulls LOW, the pull-up resistor holds the line HIGH. This is safe and allows multiple devices to share the bus.

### BSRR: Atomic Bit Set/Reset

The ODR register requires a read-modify-write sequence (`ODR |= ...`). If an interrupt fires between the read and the write, it can corrupt the register (called a **race condition**).

BSRR solves this with a single atomic write:

```c
// BSRR layout:
// Bits [31:16] = Reset bits (write 1 to clear the corresponding ODR bit)
// Bits [15:0]  = Set bits (write 1 to set the corresponding ODR bit)

// Set PA5 HIGH (atomically)
GPIOA->BSRR = (1 << 5);       // Set bit 5

// Set PA5 LOW (atomically)
GPIOA->BSRR = (1 << (5 + 16)); // Reset bit 5

// No read-modify-write. No race condition. One instruction. Done.
```

### Alternate Functions

Each pin can be connected to an internal peripheral via one of 16 alternate function channels (AF0-AF15). The mapping is chip-specific and found in the datasheet's "Alternate Function Mapping" table.

```
    STM32F4 Pin PA2 Alternate Functions:
    AF0:  —
    AF1:  TIM2_CH3 / TIM5_CH3
    AF2:  TIM9_CH1
    AF3:  —
    AF4:  I2C3_SMBA
    AF5:  SPI1_SCK
    AF6:  —
    AF7:  USART2_TX    ← This is why we set AF7 for UART
    AF8:  —
    ...
```

**Configuring PA2 for USART2_TX (AF7):**
```c
// 1. Set pin mode to Alternate Function (10)
GPIOA->MODER &= ~(3U << (2 * 2));
GPIOA->MODER |=  (2U << (2 * 2));

// 2. Set alternate function to AF7
// AFRL handles pins 0-7 (4 bits per pin)
// Pin 2: bits [11:8] of AFRL
GPIOA->AFRL &= ~(0xFU << (2 * 4));   // Clear 4 bits for pin 2
GPIOA->AFRL |=  (0x7U << (2 * 4));   // Set AF7
```

---

## Chapter 10: UART Driver (Complete, Production-Quality)

You've already built a UART driver in ZeroHAL. Here is the complete theory and a production-quality implementation.

### Baud Rate Calculation

The USART peripheral generates its timing from the peripheral bus clock. The BRR (Baud Rate Register) contains a fixed-point divisor:

$$USARTDIV = \frac{f_{CK}}{16 \times BaudRate}$$

Where $f_{CK}$ is the peripheral bus clock (APB1 for USART2 = 42 MHz at 168 MHz SYSCLK, or 16 MHz when using HSI default).

**Example: 115200 baud at 16 MHz (HSI default):**

$$USARTDIV = \frac{16{,}000{,}000}{16 \times 115200} = 8.6805...$$

The BRR stores this as a 12.4 fixed-point number:
*   Mantissa (integer part) = 8 → bits [15:4]
*   Fraction = 0.6805 × 16 = 10.888 ≈ 11 → bits [3:0]
*   BRR = (8 << 4) | 11 = 128 + 11 = 139 = **0x008B**

**Example: 115200 baud at 42 MHz (APB1 with PLL at 168 MHz):**

$$USARTDIV = \frac{42{,}000{,}000}{16 \times 115200} = 22.786...$$

*   Mantissa = 22
*   Fraction = 0.786 × 16 = 12.58 ≈ 13
*   BRR = (22 << 4) | 13 = 352 + 13 = 365 = **0x016D**

### Complete UART Driver

```c
typedef struct {
    volatile uint32_t SR;    // 0x00: Status Register
    volatile uint32_t DR;    // 0x04: Data Register
    volatile uint32_t BRR;   // 0x08: Baud Rate Register
    volatile uint32_t CR1;   // 0x0C: Control Register 1
    volatile uint32_t CR2;   // 0x10: Control Register 2
    volatile uint32_t CR3;   // 0x14: Control Register 3
    volatile uint32_t GTPR;  // 0x18: Guard Time and Prescaler
} USART_TypeDef;

#define USART2  ((USART_TypeDef *)0x40004400)

// Status Register (SR) Bit Definitions
#define USART_SR_RXNE   (1 << 5)  // Receive Data Register Not Empty
#define USART_SR_TXE    (1 << 7)  // Transmit Data Register Empty
#define USART_SR_TC     (1 << 6)  // Transmission Complete

// Control Register 1 (CR1) Bit Definitions
#define USART_CR1_RE    (1 << 2)  // Receiver Enable
#define USART_CR1_TE    (1 << 3)  // Transmitter Enable
#define USART_CR1_RXNEIE (1 << 5) // RXNE Interrupt Enable
#define USART_CR1_UE    (1 << 13) // USART Enable

void uart2_init(uint32_t baud, uint32_t pclk1) {
    // Enable clocks
    RCC->AHB1ENR |= (1 << 0);   // GPIOA clock
    RCC->APB1ENR |= (1 << 17);  // USART2 clock

    // Configure PA2 (TX) and PA3 (RX) as Alternate Function 7
    GPIOA->MODER &= ~((3U << 4) | (3U << 6));
    GPIOA->MODER |=  ((2U << 4) | (2U << 6));
    GPIOA->AFRL  &= ~((0xFU << 8) | (0xFU << 12));
    GPIOA->AFRL  |=  ((0x7U << 8) | (0x7U << 12));

    // Pull-up on RX pin
    GPIOA->PUPDR &= ~(3U << 6);
    GPIOA->PUPDR |=  (1U << 6);

    // Calculate BRR value
    // USARTDIV = pclk1 / (16 * baud)
    // BRR = mantissa << 4 | fraction
    uint32_t usartdiv_x16 = (pclk1 + (baud / 2)) / baud; // Round to nearest
    USART2->BRR = usartdiv_x16;

    // Enable USART: receiver, transmitter, RXNE interrupt, and USART enable
    USART2->CR1 = USART_CR1_RE | USART_CR1_TE
                | USART_CR1_RXNEIE | USART_CR1_UE;

    // Enable USART2 IRQ in NVIC (IRQ 38)
    NVIC_ISER[1] |= (1 << (38 - 32));
}

void uart2_putc(char c) {
    while (!(USART2->SR & USART_SR_TXE));  // Wait until TXE
    USART2->DR = (uint32_t)c;
}

void uart2_puts(const char *s) {
    while (*s) uart2_putc(*s++);
}

char uart2_getc(void) {
    while (!(USART2->SR & USART_SR_RXNE)); // Wait until RXNE (blocking)
    return (char)(USART2->DR & 0xFF);
}
```

---

## Chapter 11: Hardware Timers and PWM

### Timer Architecture

STM32F4 has many hardware timers, each with different capabilities:

| Timer | Type | Bits | Channels | Features |
|---|---|---|---|---|
| TIM1, TIM8 | Advanced | 16 | 4 | Complementary outputs, dead-time, break input |
| TIM2, TIM5 | General | 32 | 4 | Full-featured, 32-bit counter |
| TIM3, TIM4 | General | 16 | 4 | Standard timers |
| TIM6, TIM7 | Basic | 16 | 0 | Counting only (no capture/compare) |
| TIM9-TIM14 | Lite | 16 | 1-2 | Reduced feature set |
| SysTick | Core | 24 | 0 | ARM core timer (1ms tick) |

### Timer Counting Modes

```
    Up-counting (default):
    CNT  ╱╲  ╱╲  ╱╲  ╱╲
        ╱  ╲╱  ╲╱  ╲╱  ╲
       0   ARR  0  ARR  0
           │         │
           └─ Update Event (overflow) triggers interrupt

    Down-counting:
    CNT  ╲  ╱╲  ╱╲  ╱╲
          ╲╱  ╲╱  ╲╱  ╲╱
        ARR  0  ARR  0  ARR
              │         │
              └─ Update Event (underflow)

    Center-aligned (up-down):
    CNT    ╱╲    ╱╲    ╱╲
          ╱  ╲  ╱  ╲  ╱  ╲
         ╱    ╲╱    ╲╱    ╲
        0   ARR  0  ARR  0
    → Used for symmetric PWM in motor control
```

### Timer Clock and Prescaler

```
    Timer Clock (from APB)
         │
         ▼
    ┌──────────┐
    │ Prescaler│   Divides clock by (PSC + 1)
    │  (PSC)   │   Example: PSC = 15 → divide by 16
    └────┬─────┘
         │
         ▼
    Timer Counter Clock = APB_CLK / (PSC + 1)
         │
         ▼
    ┌──────────┐
    │ Counter  │   Counts from 0 to ARR, then resets (up-counting mode)
    │  (CNT)   │
    └────┬─────┘
         │
         ▼
    Auto-Reload Register (ARR): Defines the counter period
    Update Event Rate = Timer_Clock / (ARR + 1)
```

**Frequency Formula:**

$$f_{timer} = \frac{f_{APB\:timer\:clock}}{(PSC + 1) \times (ARR + 1)}$$

> **Note:** On STM32, if the APB prescaler is not /1, the timer clock is automatically doubled. So if APB1 = 42 MHz (because of /4 prescaler), the timer clock input is actually 84 MHz.

### PWM Output

PWM uses a Capture/Compare Register (CCR) to define when the output switches:

```
    ARR = 999 (period)
    CCR = 749 (75% duty cycle)

    Counter:  0 ─────────── 749 ──────── 999
                                          │
                             ┌────────────┘
                             │
    Output:   ████████████████████████████░░░░░░░░░░
              ← HIGH (CNT < CCR) →       ←LOW→
              │←──────── 75% ────────→│←25%→│
```

### Complete PWM Driver (TIM3, Channel 1, PA6)

```c
void pwm_init(uint32_t frequency, uint8_t duty_percent) {
    // 1. Enable clocks
    RCC->AHB1ENR |= (1 << 0);   // GPIOA
    RCC->APB1ENR |= (1 << 1);   // TIM3

    // 2. Configure PA6 as AF2 (TIM3_CH1)
    GPIOA->MODER &= ~(3U << (6 * 2));
    GPIOA->MODER |=  (2U << (6 * 2));  // Alternate function
    GPIOA->AFRL  &= ~(0xFU << (6 * 4));
    GPIOA->AFRL  |=  (0x2U << (6 * 4)); // AF2 = TIM3

    // 3. Configure timer
    // Timer clock = 84 MHz (APB1 timer clock with /4 prescaler)
    // We want configurable frequency
    // PSC = 83 → timer counts at 1 MHz (1µs per tick)
    TIM3->PSC = 84 - 1;

    // ARR determines frequency: f = 1MHz / (ARR+1)
    // For 1kHz PWM: ARR = 999
    TIM3->ARR = (1000000 / frequency) - 1;

    // 4. Configure Channel 1 for PWM Mode 1
    // OC1M = 110 (PWM Mode 1): HIGH when CNT < CCR1
    TIM3->CCMR1 &= ~(7U << 4);
    TIM3->CCMR1 |=  (6U << 4);    // PWM Mode 1
    TIM3->CCMR1 |=  (1U << 3);    // OC1PE: Preload enable (smooth updates)

    // 5. Set duty cycle
    TIM3->CCR1 = (TIM3->ARR + 1) * duty_percent / 100;

    // 6. Enable Channel 1 output
    TIM3->CCER |= (1 << 0);  // CC1E: Capture/Compare 1 output enable

    // 7. Enable counter
    TIM3->CR1 |= (1 << 0);   // CEN: Counter enable
}

void pwm_set_duty(uint8_t duty_percent) {
    TIM3->CCR1 = (TIM3->ARR + 1) * duty_percent / 100;
}
```

---

## Chapter 12: The Interrupt System (NVIC)

### How Interrupts Work

Without interrupts, the CPU must constantly poll every peripheral: *"Is there data? No. Is there data? No. Is there data? Yes!"* This wastes CPU cycles.

With interrupts, the peripheral raises a hardware signal when something happens. The CPU stops what it's doing, runs your handler function, and resumes the original code.

```
    Normal Execution:          Interrupt Occurs:

    main():                    main():
    ├── instruction 1          ├── instruction 1
    ├── instruction 2          ├── instruction 2
    ├── instruction 3          ├── instruction 3 ← IRQ fires here!
    ├── instruction 4          │                            │
    ├── instruction 5          │   CPU automatically:       │
    ├── ...                    │   1. Pushes R0-R3,R12,     │
                               │      LR,PC,xPSR to stack  │
                               │   2. Loads ISR address     │
                               │      from vector table     │
                               │   3. Jumps to ISR          │
                               │                            ▼
                               │   USART2_IRQHandler():
                               │   ├── read data register
                               │   ├── push to ring buffer
                               │   └── return (BX LR)
                               │                            │
                               │   CPU automatically:       │
                               │   1. Pops registers        │
                               │   2. Resumes main()        │
                               │                            │
                               ├── instruction 4  ← resumes here
                               ├── instruction 5
                               ├── ...
```

### The NVIC (Nested Vectored Interrupt Controller)

The NVIC is part of the ARM Cortex-M core (not vendor-specific). It manages all interrupts.

**Key NVIC Registers:**

| Register Set | Purpose |
|---|---|
| ISER[0..7] | Interrupt Set-Enable (write 1 to enable an IRQ) |
| ICER[0..7] | Interrupt Clear-Enable (write 1 to disable an IRQ) |
| ISPR[0..7] | Interrupt Set-Pending (force an IRQ to fire) |
| ICPR[0..7] | Interrupt Clear-Pending (cancel a pending IRQ) |
| IPR[0..59] | Interrupt Priority (8 bits per IRQ, but only top 4 used on STM32) |

Each register is 32 bits. ISER[0] handles IRQs 0-31, ISER[1] handles IRQs 32-63, etc.

```c
// Enable USART2 interrupt (IRQ 38)
// 38 / 32 = 1 (ISER[1])
// 38 % 32 = 6 (bit 6)
NVIC->ISER[1] |= (1 << 6);

// Set USART2 priority to 2 (0 = highest, 15 = lowest on STM32)
// Priority register: 8 bits per IRQ, but only top 4 bits implemented
NVIC->IPR[38] = (2 << 4);  // Shift left 4 because bottom 4 bits are ignored
```

### Interrupt Priorities and Preemption

Lower priority number = higher urgency. Priority 0 is the most urgent.

```
    Priority 0 (Highest):  Safety-critical fault handlers
    Priority 1:            Time-critical motor control
    Priority 2:            Communication (UART RX)
    Priority 3:            Background tasks (LED blink)
    Priority 15 (Lowest):  Non-critical logging
```

**Preemption:** If a Priority 1 interrupt fires while a Priority 3 interrupt is being handled, the CPU pauses the Priority 3 handler, runs the Priority 1 handler, and then resumes Priority 3. This is the "Nested" in NVIC.

### The Stacking Mechanism

When an interrupt fires, the CPU automatically pushes 8 registers onto the stack in a specific order (this is called the **exception frame**):

```
    Stack (grows downward):
    ┌─────────┐ ← SP before interrupt
    │  xPSR   │  Program Status Register
    │   PC    │  Return address (where to resume)
    │   LR    │  Link Register
    │   R12   │
    │   R3    │
    │   R2    │
    │   R1    │
    │   R0    │
    ├─────────┤ ← SP during ISR
    │         │  ISR can use stack for its own locals
    └─────────┘
```

This takes **12 clock cycles** on Cortex-M4 (the interrupt latency). The hardware does this automatically — you don't write any assembly.

### Tail-Chaining

If another interrupt is pending when the current ISR returns, the CPU skips the unstacking and restacking process and jumps directly to the next ISR. This saves 12 cycles and is called **tail-chaining**.

### Critical Sections

Sometimes you need to temporarily disable interrupts to prevent race conditions:

```c
// Method 1: Disable ALL interrupts
__disable_irq();        // Sets PRIMASK = 1
// ... critical section: safe to modify shared data ...
__enable_irq();         // Clears PRIMASK = 0

// Method 2: Save and restore (safer, nestable)
uint32_t primask = __get_PRIMASK();
__disable_irq();
// ... critical section ...
__set_PRIMASK(primask);  // Restore previous state

// Method 3: Priority-based masking (disable only low-priority IRQs)
__set_BASEPRI(3 << 4);  // Mask IRQs with priority >= 3
// ... only priority 0, 1, 2 can fire ...
__set_BASEPRI(0);        // Restore: all IRQs enabled
```

---

## Chapter 13: ADC (Analog-to-Digital Converter)

### How an ADC Works Internally

The STM32F4 uses a **Successive Approximation Register (SAR)** ADC. It works by binary search:

```
    Input Voltage: 2.1V (out of 3.3V reference)

    Step 1: Is voltage > 1.65V (half)? YES → MSB = 1
    Step 2: Is voltage > 2.475V (3/4)? NO  → bit = 0
    Step 3: Is voltage > 2.0625V (5/8)? YES → bit = 1
    Step 4: Is voltage > 2.27V? NO → bit = 0
    ... (continues for 12 bits)

    Result: 0b101000... → proportional to 2.1V
```

12 bits → 4096 levels. Resolution = 3.3V / 4096 = **0.806 mV per count**.

### ADC Configuration

```c
void adc1_init(void) {
    // 1. Enable clocks
    RCC->AHB1ENR |= (1 << 0);   // GPIOA
    RCC->APB2ENR |= (1 << 8);   // ADC1

    // 2. Configure PA0 as Analog mode
    GPIOA->MODER |= (3U << (0 * 2));  // 11 = Analog

    // 3. Configure ADC
    ADC1->CR1 = 0;                // 12-bit resolution (default), no scan
    ADC1->CR2 = 0;
    ADC1->CR2 |= (1 << 0);       // ADON: Turn on ADC
    ADC1->CR2 |= (1 << 1);       // CONT: Continuous conversion mode

    // 4. Set sample time for channel 0
    // Longer sample time = more accurate but slower
    // 0b111 = 480 cycles (maximum, most accurate)
    ADC1->SMPR2 |= (7U << (0 * 3)); // Channel 0: 480 cycles

    // 5. Set channel 0 as first (and only) conversion in sequence
    ADC1->SQR3 = 0;  // Channel 0, rank 1
    ADC1->SQR1 = 0;  // 1 conversion in sequence (L=0)

    // 6. Start conversion
    ADC1->CR2 |= (1 << 30);  // SWSTART: Start conversion
}

uint16_t adc1_read(void) {
    // Wait for End Of Conversion
    while (!(ADC1->SR & (1 << 1)));  // EOC flag
    return (uint16_t)ADC1->DR;       // Reading DR clears EOC
}

float adc_to_voltage(uint16_t adc_val) {
    return (float)adc_val * 3.3f / 4095.0f;
}
```

---

## Chapter 14: I2C Driver (Complete From Scratch)

### I2C Protocol Deep Dive

I2C uses two open-drain wires with pull-up resistors:

```
    Timing Diagram for Writing 1 byte to address 0x48:

    SCL ──┐ ┌──┐ ┌──┐ ┌──┐ ┌──┐ ┌──┐ ┌──┐ ┌──┐ ┌──┐ ┌──┐ ┌──┐ ┌──
          └─┘  └─┘  └─┘  └─┘  └─┘  └─┘  └─┘  └─┘  └─┘  └─┘  └─┘

    SDA ─┐ ┌────┐    ┌──────────┐         ┌─┐    ┌───────────┐    ┌──
         └─┘    └────┘          └─────────┘ └────┘           └────┘

         S  A6  A5  A4  A3  A2  A1  A0  RW ACK D7  D6  D5 ... D0 ACK P
         │  │                           │  │                       │  │
         │  └── 7-bit address: 1001000  │  │                       │  │
         │      = 0x48                  │  │                       │  │
         │                              │  │                       │  │
         Start                         W=0 │                       │  Stop
         Condition                         Slave pulls SDA LOW     │
         (SDA↓                             to acknowledge          │
          while                                                    │
          SCL=1)                                                   │
```

### I2C Driver Implementation

```c
typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t OAR1;
    volatile uint32_t OAR2;
    volatile uint32_t DR;
    volatile uint32_t SR1;
    volatile uint32_t SR2;
    volatile uint32_t CCR;
    volatile uint32_t TRISE;
} I2C_TypeDef;

#define I2C1 ((I2C_TypeDef *)0x40005400)

void i2c1_init(void) {
    // 1. Enable clocks
    RCC->AHB1ENR |= (1 << 1);   // GPIOB
    RCC->APB1ENR |= (1 << 21);  // I2C1

    // 2. Configure PB6 (SCL) and PB7 (SDA) as AF4, Open-Drain
    GPIOB->MODER &= ~((3U << 12) | (3U << 14));
    GPIOB->MODER |=  ((2U << 12) | (2U << 14));  // Alternate Function

    GPIOB->OTYPER |= (1 << 6) | (1 << 7);  // Open-drain (REQUIRED for I2C)

    GPIOB->PUPDR &= ~((3U << 12) | (3U << 14));
    GPIOB->PUPDR |=  ((1U << 12) | (1U << 14));  // Internal pull-ups

    GPIOB->AFRL &= ~((0xFU << 24) | (0xFU << 28));
    GPIOB->AFRL |=  ((4U << 24) | (4U << 28));  // AF4 = I2C1

    // 3. Reset I2C peripheral
    I2C1->CR1 |= (1 << 15);   // SWRST
    I2C1->CR1 &= ~(1 << 15);

    // 4. Configure I2C clock
    // APB1 clock = 42 MHz
    I2C1->CR2 = 42;           // FREQ = APB1 clock in MHz

    // 5. Configure for 100kHz Standard Mode
    // CCR = APB1_CLK / (2 * I2C_speed)
    // CCR = 42,000,000 / (2 * 100,000) = 210
    I2C1->CCR = 210;

    // 6. Configure maximum rise time
    // Standard mode: 1000ns max rise time
    // TRISE = (1000ns / (1/APB1_CLK)) + 1 = (42) + 1 = 43
    I2C1->TRISE = 43;

    // 7. Enable I2C
    I2C1->CR1 |= (1 << 0);   // PE: Peripheral Enable
}

void i2c1_start(void) {
    I2C1->CR1 |= (1 << 8);              // Generate START condition
    while (!(I2C1->SR1 & (1 << 0)));     // Wait for SB (Start Bit) flag
}

void i2c1_stop(void) {
    I2C1->CR1 |= (1 << 9);              // Generate STOP condition
}

void i2c1_write_addr(uint8_t addr, uint8_t rw) {
    I2C1->DR = (addr << 1) | rw;        // Send 7-bit address + R/W bit
    while (!(I2C1->SR1 & (1 << 1)));     // Wait for ADDR flag
    // Clear ADDR flag by reading SR1 then SR2 (hardware requirement)
    (void)I2C1->SR1;
    (void)I2C1->SR2;
}

void i2c1_write_byte(uint8_t data) {
    while (!(I2C1->SR1 & (1 << 7)));     // Wait for TXE (TX buffer empty)
    I2C1->DR = data;
    while (!(I2C1->SR1 & (1 << 2)));     // Wait for BTF (Byte Transfer Finished)
}

uint8_t i2c1_read_byte_ack(void) {
    I2C1->CR1 |= (1 << 10);             // ACK = 1 (send ACK after receive)
    while (!(I2C1->SR1 & (1 << 6)));     // Wait for RXNE
    return (uint8_t)I2C1->DR;
}

uint8_t i2c1_read_byte_nack(void) {
    I2C1->CR1 &= ~(1 << 10);            // ACK = 0 (send NACK = last byte)
    i2c1_stop();                          // Generate STOP before reading last byte
    while (!(I2C1->SR1 & (1 << 6)));     // Wait for RXNE
    return (uint8_t)I2C1->DR;
}

// High-level function: Write a register on an I2C device
void i2c_write_reg(uint8_t dev_addr, uint8_t reg, uint8_t value) {
    i2c1_start();
    i2c1_write_addr(dev_addr, 0);  // Write mode
    i2c1_write_byte(reg);
    i2c1_write_byte(value);
    i2c1_stop();
}

// High-level function: Read a register from an I2C device
uint8_t i2c_read_reg(uint8_t dev_addr, uint8_t reg) {
    i2c1_start();
    i2c1_write_addr(dev_addr, 0);  // Write mode (to send register address)
    i2c1_write_byte(reg);

    i2c1_start();                   // Repeated START
    i2c1_write_addr(dev_addr, 1);  // Read mode
    uint8_t data = i2c1_read_byte_nack();
    return data;
}
```

---

## Chapter 15: SPI Driver (Complete From Scratch)

### SPI Protocol

SPI is a full-duplex, synchronous protocol. Data is shifted out on one edge of the clock and sampled on the other.

```
    SPI Transaction (sending 0xA5 = 0b10100101):

    CS   ─────┐                                         ┌─────
              └─────────────────────────────────────────┘

    SCK  ─────┐  ┌──┐  ┌──┐  ┌──┐  ┌──┐  ┌──┐  ┌──┐  ┌──┐  ┌───
              └──┘  └──┘  └──┘  └──┘  └──┘  └──┘  └──┘  └──┘

    MOSI ─────╳──1──╳──0──╳──1──╳──0──╳──0──╳──1──╳──0──╳──1──╳───
              bit7  bit6  bit5  bit4  bit3  bit2  bit1  bit0

    MISO ─────╳──d7─╳──d6─╳──d5─╳──d4─╳──d3─╳──d2─╳──d1─╳──d0─╳───
              (slave sends data back simultaneously)
```

### SPI Clock Polarity and Phase (CPOL/CPHA)

| Mode | CPOL | CPHA | Idle Clock | Sample Edge |
|---|---|---|---|---|
| Mode 0 | 0 | 0 | LOW | Rising edge |
| Mode 1 | 0 | 1 | LOW | Falling edge |
| Mode 2 | 1 | 0 | HIGH | Falling edge |
| Mode 3 | 1 | 1 | HIGH | Rising edge |

**Mode 0** is the most common. Check the slave device's datasheet for the required mode.

### SPI Driver Implementation

```c
typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t CRCPR;
    volatile uint32_t RXCRCR;
    volatile uint32_t TXCRCR;
    volatile uint32_t I2SCFGR;
    volatile uint32_t I2SPR;
} SPI_TypeDef;

#define SPI1 ((SPI_TypeDef *)0x40013000)

void spi1_init(void) {
    // 1. Enable clocks
    RCC->AHB1ENR |= (1 << 0);   // GPIOA
    RCC->APB2ENR |= (1 << 12);  // SPI1

    // 2. Configure PA5 (SCK), PA6 (MISO), PA7 (MOSI) as AF5
    GPIOA->MODER &= ~((3U << 10) | (3U << 12) | (3U << 14));
    GPIOA->MODER |=  ((2U << 10) | (2U << 12) | (2U << 14));
    GPIOA->AFRL  &= ~((0xFU << 20) | (0xFU << 24) | (0xFU << 28));
    GPIOA->AFRL  |=  ((5U << 20) | (5U << 24) | (5U << 28));

    // 3. Configure PA4 as GPIO Output (manual Chip Select)
    GPIOA->MODER &= ~(3U << (4 * 2));
    GPIOA->MODER |=  (1U << (4 * 2));  // Output
    GPIOA->ODR   |= (1 << 4);           // CS HIGH (deselected)

    // 4. Configure SPI1
    SPI1->CR1 = 0;
    SPI1->CR1 |= (1 << 2);     // MSTR: Master mode
    SPI1->CR1 |= (3 << 3);     // BR = 011: fPCLK/16 = 84MHz/16 = 5.25MHz
    // CPOL=0, CPHA=0 (Mode 0) — default, bits already 0
    SPI1->CR1 |= (1 << 9);     // SSM: Software slave management
    SPI1->CR1 |= (1 << 8);     // SSI: Internal slave select (must be HIGH for master)

    // 5. Enable SPI
    SPI1->CR1 |= (1 << 6);     // SPE: SPI Enable
}

void spi1_cs_low(void)  { GPIOA->BSRR = (1 << (4 + 16)); } // Assert CS
void spi1_cs_high(void) { GPIOA->BSRR = (1 << 4); }         // Deassert CS

uint8_t spi1_transfer(uint8_t tx_data) {
    // SPI is full-duplex: sending and receiving happen simultaneously
    while (!(SPI1->SR & (1 << 1)));    // Wait until TXE (TX buffer empty)
    SPI1->DR = tx_data;                 // Write data to send

    while (!(SPI1->SR & (1 << 0)));    // Wait until RXNE (RX buffer full)
    return (uint8_t)SPI1->DR;           // Read received data (clears RXNE)
}

// Example: Read WHO_AM_I register from an SPI sensor
uint8_t spi_read_register(uint8_t reg) {
    spi1_cs_low();
    spi1_transfer(reg | 0x80);         // Bit 7 = 1 means "read" (common convention)
    uint8_t value = spi1_transfer(0xFF); // Send dummy byte to clock in response
    spi1_cs_high();
    return value;
}
```

---

## Chapter 16: DMA (Direct Memory Access)

### The Problem DMA Solves

Without DMA, every byte of data transfer requires CPU involvement:
```
    CPU reads USART2->DR → stores in buffer[0]
    CPU reads USART2->DR → stores in buffer[1]
    CPU reads USART2->DR → stores in buffer[2]
    ... (CPU is stuck doing this, can't do anything else)
```

With DMA, a dedicated hardware engine transfers data directly between peripherals and memory without CPU intervention:
```
    DMA automatically: USART2->DR → buffer[0..N]
    CPU is free to: process previous data, update display, etc.
```

### DMA Architecture

```
    ┌──────────┐            ┌──────────────────┐
    │   CPU    │            │      DMA         │
    │          │            │  ┌────────────┐  │
    │ (free to │            │  │  Channel/  │  │
    │  do other│            │  │  Stream 0  │──┼──→ SRAM (buffer)
    │  work!)  │            │  └────────────┘  │
    │          │            │  ┌────────────┐  │
    └──────────┘            │  │  Stream 1  │──┼──→ ...
                            │  └────────────┘  │
    ┌──────────┐            │  ...             │
    │ USART2   │──(RXNE)──→ │  ┌────────────┐  │
    │          │            │  │  Stream 5  │──┼──→ SPI1 TX
    └──────────┘            │  └────────────┘  │
                            └──────────────────┘
```

### DMA Configuration (UART RX → Memory)

```c
void dma_uart_rx_init(uint8_t *buffer, uint16_t length) {
    // 1. Enable DMA1 clock
    RCC->AHB1ENR |= (1 << 21);  // DMA1

    // 2. Disable stream first (required before configuration)
    DMA1_Stream5->CR &= ~(1 << 0);  // EN = 0
    while (DMA1_Stream5->CR & (1 << 0));  // Wait until disabled

    // 3. Configure DMA1 Stream 5, Channel 4 (USART2_RX)
    DMA1_Stream5->CR = 0;
    DMA1_Stream5->CR |= (4 << 25);    // CHSEL = 4 (USART2_RX)
    DMA1_Stream5->CR |= (1 << 10);    // MINC: Memory address increment
    // DIR = 00: Peripheral-to-memory (default)
    DMA1_Stream5->CR |= (1 << 8);     // CIRC: Circular mode (auto-restart)

    // 4. Set addresses and count
    DMA1_Stream5->PAR  = (uint32_t)&USART2->DR;  // Source: USART2 data reg
    DMA1_Stream5->M0AR = (uint32_t)buffer;         // Destination: RAM buffer
    DMA1_Stream5->NDTR = length;                    // Number of bytes

    // 5. Enable DMA stream
    DMA1_Stream5->CR |= (1 << 0);  // EN = 1

    // 6. Tell USART2 to use DMA for receiving
    USART2->CR3 |= (1 << 6);  // DMAR: DMA enable receiver
}
```

**With circular DMA, the UART continuously receives data into your buffer without any CPU involvement whatsoever.** The CPU can check the buffer at its leisure, or use a DMA transfer-complete interrupt.

---

# PART V: LOW POWER MODES

## Chapter 17: Power Management

Battery-powered devices must minimize power consumption. The STM32F4 offers several sleep modes:

| Mode | CPU | Flash | SRAM | Peripherals | Wake-up Sources | Current |
|---|---|---|---|---|---|---|
| **Run** | ON | ON | ON | ON | — | ~50 mA |
| **Sleep** | OFF | ON | ON | ON | Any interrupt | ~10 mA |
| **Stop** | OFF | OFF | ON | Most OFF | EXTI, RTC | ~500 µA |
| **Standby** | OFF | OFF | OFF | ALL OFF | WKUP pin, RTC | ~2 µA |

```c
// Enter Sleep mode
void enter_sleep(void) {
    // WFI: Wait For Interrupt — CPU halts until any enabled interrupt fires
    __WFI();
    // Execution resumes here after the interrupt handler returns
}

// Enter Stop mode (deep sleep, retains SRAM)
void enter_stop(void) {
    // Select Stop mode in Power Control Register
    PWR->CR |= (1 << 1);     // PDDS = 0 (Stop), LPDS = 1 (low-power regulator)
    SCB->SCR |= (1 << 2);    // SLEEPDEEP bit

    __WFI();

    // After waking: clock reverts to HSI (16 MHz). Must reinit PLL.
    SCB->SCR &= ~(1 << 2);   // Clear SLEEPDEEP
    clock_init_168mhz();      // Reconfigure clocks
}
```

---

# PART VI: DEBUGGING

## Chapter 18: SWD and GDB

### SWD (Serial Wire Debug)

SWD is a 2-wire debugging interface built into every ARM Cortex-M chip. It allows you to:
*   **Halt** the CPU at any point
*   **Step** through code instruction-by-instruction
*   **Read/write** any memory address or register
*   **Set breakpoints** and watchpoints
*   **Flash** new firmware into the chip

**Hardware needed:** An SWD probe (ST-Link V2 clone: ~\$3, or a Segger J-Link EDU: ~\$20).

```
    SWD Connection (only 2 signal wires needed):

    Debugger (ST-Link)          Target (STM32 Board)
    ┌──────────────┐           ┌──────────────────┐
    │ SWDIO  ──────┼───────────┤ SWDIO (PA13)     │
    │ SWCLK  ──────┼───────────┤ SWCLK (PA14)     │
    │ GND    ──────┼───────────┤ GND              │
    │ 3.3V   ──────┼───(opt)───┤ 3.3V             │
    └──────────────┘           └──────────────────┘
```

### Using GDB with OpenOCD

OpenOCD connects your PC to the SWD probe. GDB connects to OpenOCD.

```bash
# Terminal 1: Start OpenOCD
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg

# Terminal 2: Start GDB
arm-none-eabi-gdb build/firmware.elf

# Inside GDB:
(gdb) target remote :3333        # Connect to OpenOCD
(gdb) monitor reset halt         # Reset and halt the CPU
(gdb) load                       # Flash firmware
(gdb) break main                 # Set breakpoint at main()
(gdb) continue                   # Run until breakpoint
(gdb) next                       # Step over (execute one line)
(gdb) step                       # Step into (enter functions)
(gdb) print variable_name        # Print variable value
(gdb) x/16xw 0x40020014         # Examine 16 words at address (read GPIO ODR)
(gdb) info registers             # Show all CPU registers
(gdb) bt                         # Backtrace (show call stack)
```

### HardFault Debugging

A **HardFault** is the ARM Cortex-M's equivalent of a segmentation fault. Common causes:

| Cause | Example |
|---|---|
| Dereferencing NULL pointer | `*(uint32_t *)0 = 42;` |
| Accessing unaligned memory | Reading a uint32_t from an odd address |
| Executing from invalid address | Corrupted function pointer |
| Stack overflow | Deep recursion or large local arrays |
| Writing to read-only memory | Writing to Flash region |
| Bus error | Accessing unmapped peripheral (clock not enabled!) |
| Division by zero | `int x = 10 / 0;` (if trap enabled) |

**Debugging a HardFault:**

```c
void HardFault_Handler(void) {
    // Read the stacked Program Counter to find the faulting instruction
    // The exception frame was pushed to MSP or PSP
    __asm volatile (
        "TST   LR, #4          \n"  // Test bit 2 of EXC_RETURN
        "ITE   EQ               \n"
        "MRSEQ R0, MSP          \n"  // If 0: exception used MSP
        "MRSNE R0, PSP          \n"  // If 1: exception used PSP
        "B     hard_fault_debug \n"
    );
}

void hard_fault_debug(uint32_t *stack_frame) {
    volatile uint32_t r0   = stack_frame[0];
    volatile uint32_t r1   = stack_frame[1];
    volatile uint32_t r2   = stack_frame[2];
    volatile uint32_t r3   = stack_frame[3];
    volatile uint32_t r12  = stack_frame[4];
    volatile uint32_t lr   = stack_frame[5];
    volatile uint32_t pc   = stack_frame[6];  // ← THIS is the faulting instruction
    volatile uint32_t psr  = stack_frame[7];

    // In GDB: "info locals" will show you the faulting PC address
    // Then: "list *0x08001234" to find the source line
    while (1); // Halt here for debugger inspection
}
```

---

# PART VII: THE BUILD SYSTEM

## Chapter 19: The GCC Toolchain

### Compilation Pipeline

```
    Source Files (.c, .h)
         │
         ▼
    ┌─────────────────┐
    │  Preprocessor    │  Handles #include, #define, #ifdef
    │  (cpp)           │  Output: .i files (expanded source)
    └────────┬────────┘
             │
             ▼
    ┌─────────────────┐
    │  Compiler        │  Converts C to assembly
    │  (cc1)           │  Output: .s files (assembly)
    └────────┬────────┘
             │
             ▼
    ┌─────────────────┐
    │  Assembler       │  Converts assembly to machine code
    │  (as)            │  Output: .o files (object files)
    └────────┬────────┘
             │
             ▼
    ┌─────────────────┐
    │  Linker          │  Combines all .o files, resolves symbols,
    │  (ld)            │  applies linker script, generates final binary
    │                  │  Output: .elf file
    └────────┬────────┘
             │
             ▼
    ┌─────────────────┐
    │  objcopy         │  Strips debug info, converts format
    │                  │  Output: .bin (raw binary) or .hex (Intel HEX)
    └─────────────────┘
```

### Essential Compiler Flags

```makefile
# Architecture flags
MCU_FLAGS = -mcpu=cortex-m4    # Target CPU core
            -mthumb            # Use Thumb-2 instruction set
            -mfpu=fpv4-sp-d16  # Single-precision FPU
            -mfloat-abi=hard   # Use hardware FPU for floats

# Compilation flags
CFLAGS = $(MCU_FLAGS)
         -Wall -Wextra -Werror  # Treat all warnings as errors
         -O2                    # Optimization level 2 (good balance)
         -g                     # Include debug symbols (for GDB)
         -ffreestanding         # Don't assume standard library exists
         -fno-builtin           # Don't use GCC built-in functions
         -ffunction-sections    # Place each function in its own section
         -fdata-sections        # Place each data item in its own section

# Linker flags
LDFLAGS = $(MCU_FLAGS)
          -Tlinker.ld           # Use our custom linker script
          -nostdlib              # Don't link standard C library
          -nostartfiles          # Don't use standard startup files
          -Wl,--gc-sections     # Garbage collect unused sections (reduces binary size)
          -Wl,-Map=output.map   # Generate a memory map file
```

### The Map File

The linker generates a `.map` file showing exactly where every function and variable ended up in memory. This is invaluable for:
*   Finding how much Flash and RAM you're using
*   Identifying unexpectedly large functions or data
*   Debugging linker errors

```
    Memory Configuration:
    Name     Origin      Length
    FLASH    0x08000000  0x00080000  (512K)
    SRAM     0x20000000  0x00020000  (128K)

    .text      0x08000200   0x00001A4C   (6732 bytes of code)
    .rodata    0x08001C4C   0x00000128   (296 bytes of constants)
    .data      0x20000000   0x00000020   (32 bytes of initialized data)
    .bss       0x20000020   0x00000148   (328 bytes of zeroed data)

    Total Flash used: 7060 / 524288 (1.3%)
    Total SRAM used:  360 / 131072 (0.3%)
```

### Useful Binary Inspection Commands

```bash
# Show section sizes (text=code, data=initialized, bss=zeroed)
arm-none-eabi-size firmware.elf

# Disassemble the binary (see actual ARM instructions)
arm-none-eabi-objdump -d firmware.elf | less

# List all symbols and their addresses
arm-none-eabi-nm -S --size-sort firmware.elf

# Show section headers
arm-none-eabi-objdump -h firmware.elf
```

---

# PART VIII: READING DATASHEETS

## Chapter 20: How to Read a 1,800-Page Reference Manual

### The Documents You Need

Every MCU family has several official documents:

| Document | Pages | Purpose |
|---|---|---|
| **Datasheet** | 50-200 | Pin assignments, electrical specs, package info, ordering codes |
| **Reference Manual** | 500-1800 | Complete register-level description of every peripheral |
| **Programming Manual** | 200-300 | ARM Cortex-M core documentation (instructions, NVIC, MPU) |
| **Errata** | 10-50 | Known silicon bugs and workarounds |

### Strategy for Reading the Reference Manual

**Don't read it cover to cover.** Use it as a reference:

1. **Start with the Memory Map** (usually Chapter 2). Find the base address of the peripheral you need.
2. **Go to the peripheral chapter** (e.g., Chapter 30: USART). Read the "functional description" section for how the peripheral works.
3. **Find the Register Map table** at the end of the chapter. This lists every register, its offset, and the meaning of every bit.
4. **Cross-reference with the datasheet** for pin alternate function mappings.

### Example: Finding USART2's Baud Rate Register

```
    Step 1: Reference Manual, Chapter 2 (Memory Map)
            → USART2 base address = 0x40004400

    Step 2: Reference Manual, Chapter 30.6 (USART Registers)
            → BRR offset = 0x08
            → BRR address = 0x40004400 + 0x08 = 0x40004408

    Step 3: Read the BRR register description
            → Bits [15:4] = USARTDIV mantissa
            → Bits [3:0]  = USARTDIV fraction

    Step 4: Reference Manual, Chapter 30.3.4 (Baud Rate Calculation)
            → Formula: Baud = fCK / (16 × USARTDIV)
            → For 115200 at 16 MHz: USARTDIV = 8.6805
            → BRR = 0x008B

    Now you can write:
        *((volatile uint32_t *)0x40004408) = 0x008B;
    Or equivalently:
        USART2->BRR = 0x008B;
```

### Decoding Register Descriptions

Reference manuals describe each register with a bit-field diagram:

```
    USART_CR1 (Control Register 1) — Offset 0x0C

    Bit    31..14  13    12    11    10    9     8     7     6     5..4  3    2    1    0
           Reserved UE    M     WAKE  PCE   PS    PEIE  TXEIE TCIE  RXNEIE TE   RE   RWU  SBK

    UE (bit 13): USART Enable
        0: USART prescaler and outputs disabled
        1: USART enabled

    TE (bit 3): Transmitter Enable
        0: Transmitter disabled
        1: Transmitter enabled

    RE (bit 2): Receiver Enable
        0: Receiver disabled
        1: Receiver enabled

    RXNEIE (bit 5): RXNE Interrupt Enable
        0: Interrupt inhibited
        1: An interrupt is generated when RXNE=1 in the SR register
```

**Reading this, you can write:**
```c
USART2->CR1 |= (1 << 13)   // UE: Enable USART
             |  (1 << 3)    // TE: Enable transmitter
             |  (1 << 2)    // RE: Enable receiver
             |  (1 << 5);   // RXNEIE: Enable RX interrupt
```

---

# PART IX: PRACTICAL PROJECTS

## Chapter 21: Project Ideas for Phase 3

These projects will solidify your bare-metal skills. Each one exercises different peripherals:

### Project 1: Digital Thermometer (I2C + UART)
*   Read temperature from an I2C sensor (LM75, TMP102, or BMP280).
*   Display it over UART serial terminal.
*   Add a `temp` command to your ZeroHAL CLI.

### Project 2: LED Brightness Control (ADC + PWM)
*   Read a potentiometer using the ADC.
*   Map the ADC value (0-4095) to a PWM duty cycle (0-100%).
*   Control LED brightness smoothly.

### Project 3: Data Logger (SPI + Timer)
*   Write sensor data to an SPI SD card or SPI Flash chip.
*   Use a hardware timer interrupt to sample at a precise rate (e.g., 100 Hz).
*   Add a `log` command to dump recorded data over UART.

### Project 4: Ultrasonic Distance Sensor (Timer Input Capture)
*   Trigger an HC-SR04 ultrasonic sensor with a GPIO pulse.
*   Measure the echo pulse width using timer input capture.
*   Calculate distance: $d = \frac{time \times 343}{2}$ (speed of sound = 343 m/s).

### Project 5: UART Bootloader
*   Write a small program that lives at the start of Flash.
*   It listens on UART for a new firmware binary.
*   It erases Flash sectors and writes the received binary.
*   It jumps to the new firmware's Reset Handler.
*   This is how real devices are field-updated.

---

# PART X: REFERENCE TABLES

## Appendix A: Common ARM Cortex-M Intrinsics

| Intrinsic | Purpose |
|---|---|
| `__disable_irq()` | Disable all interrupts (PRIMASK=1) |
| `__enable_irq()` | Enable all interrupts (PRIMASK=0) |
| `__WFI()` | Wait For Interrupt (enter sleep) |
| `__WFE()` | Wait For Event |
| `__NOP()` | No Operation (1 cycle delay) |
| `__DSB()` | Data Synchronization Barrier |
| `__ISB()` | Instruction Synchronization Barrier |
| `__DMB()` | Data Memory Barrier |
| `__REV(x)` | Reverse byte order (endian swap) |
| `__CLZ(x)` | Count Leading Zeros |
| `__RBIT(x)` | Reverse Bits |

## Appendix B: STM32F4 RCC Enable Bits (Quick Reference)

### AHB1ENR (RCC offset 0x30)

| Bit | Peripheral |
|---|---|
| 0 | GPIOA |
| 1 | GPIOB |
| 2 | GPIOC |
| 3 | GPIOD |
| 4 | GPIOE |
| 7 | GPIOH |
| 12 | CRC |
| 21 | DMA1 |
| 22 | DMA2 |

### APB1ENR (RCC offset 0x40)

| Bit | Peripheral |
|---|---|
| 0 | TIM2 |
| 1 | TIM3 |
| 2 | TIM4 |
| 3 | TIM5 |
| 11 | WWDG |
| 14 | SPI2 |
| 15 | SPI3 |
| 17 | USART2 |
| 21 | I2C1 |
| 22 | I2C2 |
| 23 | I2C3 |
| 28 | PWR |

### APB2ENR (RCC offset 0x44)

| Bit | Peripheral |
|---|---|
| 0 | TIM1 |
| 4 | USART1 |
| 5 | USART6 |
| 8 | ADC1 |
| 12 | SPI1 |
| 14 | SYSCFG |

## Appendix C: Common Baud Rate BRR Values

### At 16 MHz (HSI, APB1 undivided)

| Baud Rate | USARTDIV | BRR Value |
|---|---|---|
| 9600 | 104.1667 | 0x0683 |
| 19200 | 52.0833 | 0x0341 |
| 38400 | 26.0417 | 0x01A1 |
| 57600 | 17.3611 | 0x0116 |
| 115200 | 8.6806 | 0x008B |

### At 42 MHz (APB1 with 168 MHz SYSCLK, /4 prescaler)

| Baud Rate | USARTDIV | BRR Value |
|---|---|---|
| 9600 | 273.4375 | 0x1117 |
| 115200 | 22.7865 | 0x016D |
| 921600 | 2.8483 | 0x002E |

## Appendix D: GPIO Alternate Function Quick Reference (STM32F411)

| Pin | AF0 | AF1 | AF2 | AF4 | AF5 | AF6 | AF7 | AF8 |
|---|---|---|---|---|---|---|---|---|
| PA0 | — | TIM2_CH1 | TIM5_CH1 | I2C3_SDA | — | — | USART2_CTS | — |
| PA1 | — | TIM2_CH2 | TIM5_CH2 | — | SPI4_MOSI | — | USART2_RTS | — |
| PA2 | — | TIM2_CH3 | TIM5_CH3 | — | — | — | **USART2_TX** | — |
| PA3 | — | TIM2_CH4 | TIM5_CH4 | — | — | — | **USART2_RX** | — |
| PA4 | — | — | — | — | SPI1_NSS | SPI3_NSS | USART2_CK | — |
| PA5 | — | TIM2_CH1 | — | — | **SPI1_SCK** | — | — | — |
| PA6 | — | TIM1_BKIN | **TIM3_CH1** | — | SPI1_MISO | — | — | — |
| PA7 | — | TIM1_CH1N | TIM3_CH2 | — | SPI1_MOSI | — | — | — |
| PA9 | — | TIM1_CH2 | — | I2C3_SMBA | — | — | **USART1_TX** | — |
| PA10 | — | TIM1_CH3 | — | — | — | — | **USART1_RX** | — |
| PB6 | — | TIM4_CH1 | — | **I2C1_SCL** | — | — | USART1_TX | — |
| PB7 | — | TIM4_CH2 | — | **I2C1_SDA** | — | — | USART1_RX | — |

## Appendix E: Fault Status Registers

When a HardFault occurs, check these registers to identify the cause:

| Register | Address | Purpose |
|---|---|---|
| CFSR | 0xE000ED28 | Configurable Fault Status Register (combined) |
| HFSR | 0xE000ED2C | HardFault Status Register |
| DFSR | 0xE000ED30 | Debug Fault Status Register |
| MMFAR | 0xE000ED34 | MemManage Fault Address Register |
| BFAR | 0xE000ED38 | BusFault Address Register |

**CFSR is split into three sub-registers:**

| Bits | Name | Meaning |
|---|---|---|
| [7:0] | MMFSR | MemManage Fault Status |
| [15:8] | BFSR | Bus Fault Status |
| [31:16] | UFSR | Usage Fault Status |

**Common UFSR Flags:**

| Bit | Name | Meaning |
|---|---|---|
| 16 | UNDEFINSTR | Undefined instruction executed |
| 17 | INVSTATE | Invalid state (Thumb bit not set) |
| 18 | INVPC | Invalid PC load on exception return |
| 24 | UNALIGNED | Unaligned memory access |
| 25 | DIVBYZERO | Division by zero |

---

**Congratulations.** If you have read and understood this document, you possess the knowledge to write firmware for any ARM Cortex-M microcontroller on earth. The next step is **Phase 4: RTOS** — where you add preemptive multitasking, mutexes, and message queues to build real-time systems.
