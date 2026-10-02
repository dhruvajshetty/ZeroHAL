# ZeroHAL

[![CI Status](https://img.shields.io/badge/CI-Passing-brightgreen?style=flat-square&logo=githubactions)](https://github.com/dhruvajshetty/ZeroHAL/actions)
[![Architecture](https://img.shields.io/badge/Architecture-ARM%20Cortex--M4F-blue?style=flat-square&logo=arm)](https://developer.arm.com/)
[![Standard](https://img.shields.io/badge/Philosophy-ISO%2026262%20Deterministic-orange?style=flat-square)](https://www.iso.org/standard/68383.html)
[![Standard](https://img.shields.io/badge/Language-C99%20Freestanding-blue?style=flat-square)](https://en.wikipedia.org/wiki/C99)
[![Unit Tests](https://img.shields.io/badge/Unit%20Tests-40%2F40%20Passing-success?style=flat-square)](tests/test_runner.c)
[![License](https://img.shields.io/badge/License-MIT-purple?style=flat-square)](LICENSE)

> **A deterministic, bare-metal driver suite, startup runtime, and silicon emulator for ARM Cortex-M microcontrollers — written with zero vendor HAL, zero standard libraries, and 100% volatile memory-mapped register pointers.**

---

## Table of Contents
- [Why ZeroHAL?](#why-zerohal)
- [Comparison: Vendor HAL vs. ZeroHAL](#comparison-vendor-hal-vs-zerohal)
- [System Architecture](#system-architecture)
- [Silicon Memory Map Reference](#silicon-memory-map-reference)
- [Repository Structure](#repository-structure)
- [Quickstart: Host Silicon Simulator (No Hardware Required)](#quickstart-host-silicon-simulator-no-hardware-required)
- [Automated Unit Testing & Verification](#automated-unit-testing--verification)
- [Building Bare-Metal Silicon Firmware](#building-bare-metal-silicon-firmware)
- [Flashing to Hardware](#flashing-to-hardware)
- [Interactive ECU Diagnostic Console](#interactive-ecu-diagnostic-console)
- [Silicon Boot Sequence (Pure C Bootstrap)](#silicon-boot-sequence-pure-c-bootstrap)
- [License](#license)

---

## Why ZeroHAL?

In automotive (ISO 26262 ASIL-D), aerospace (DO-178C), and safety-critical medical devices, vendor-supplied hardware abstraction libraries (like STM32Cube HAL or Arduino core) introduce severe liabilities:
1. **Unbounded Latency & Hidden Polling Loops**: Vendor HAL functions frequently embed indeterminate timeout loops (`while(!flag)`) without guaranteed execution bounds.
2. **Flash & RAM Bloat**: Vendor drivers bundle large configuration structs and unused state machines, consuming scarce SRAM and Flash.
3. **Lack of Determinism**: Deep call graphs and abstraction layers obscure exactly when and how silicon gates and registers are toggled.

**ZeroHAL bypasses all abstractions.** Every peripheral interaction is implemented directly against physical bus registers using exact volatile memory-mapped pointers, fixed-capacity lock-free data structures, and predictable interrupt service routines.

---

## Comparison: Vendor HAL vs. ZeroHAL

| Metric / Attribute | STM32Cube HAL / Vendor Driver | ZeroHAL Bare-Metal Driver |
| :--- | :--- | :--- |
| **Vendor Dependency** | High (proprietary license & CMSIS) | **Zero (100% open, pure C99)** |
| **C Standard Library** | Requires libc / newlib runtime | **Freestanding (`-nostdlib`, `-nostartfiles`)** |
| **Binary Footprint** | ~15 KB to 30 KB minimum | **< 2.5 KB total Flash footprint** |
| **SRAM Static Overhead** | Hundreds of bytes in handle structs | **64 bytes (exact FIFO ring buffer)** |
| **ISR Decoupling** | Blocking callbacks or weak hooks | **Lock-Free Single-Producer Single-Consumer FIFO** |
| **Hardware Safety** | Optional software watchdog handlers | **Silicon IWDG with hardware reboot audit** |
| **Host Simulator** | None (requires hardware debugger) | **Full silicon bus transaction emulator included** |

---

## System Architecture

```
+-------------------------------------------------------------------------------+
|                             ARM Cortex-M4F CORE                               |
|                                                                               |
|   [SysTick Timer: 0xE000E010]                   [NVIC Controller: 0xE000E100] |
|       * 1ms system heartbeat                        * IRQ 38 (USART2 RX)      |
+---------------------------------------+---------------------------------------+
                                        |
                             AHB Bus Matrix (16 MHz)
                                        |
         +------------------------------+-------------------------------+
         |                                                              |
  [FLASH: 0x08000000]                                           [SRAM: 0x20000000]
   * Vector Table (.isr_vector)                                  * .data (VMA) / .bss
   * .text (Machine instructions)                                * Lock-free SPSC FIFO
   * .rodata (Constants & strings)                               * Descending Stack (_estack)
   * .data LMA (Flash Image)                                            |
         |                                                              |
  +------+--------------------------------------------------------------+-------+
  | AHB1 Bus Bridge                                                             |
  |  * RCC Clock Gating: AHB1ENR (0x40023830)                                   |
  |  * GPIO Port A: MODER, PUPDR, AFRL, ODR (0x40020000)                        |
  |      - PA2 -> Alternate Function AF7 -> USART2_TX (Serial Telemetry Out)    |
  |      - PA3 -> Alternate Function AF7 -> USART2_RX (Asynchronous In + Pullup)|
  |      - PA5 -> Output Driver ----------> Green User LED (Heartbeat & Status) |
  +------+----------------------------------------------------------------------+
         |
  +------+----------------------------------------------------------------------+
  | APB1 Bus Bridge                                                             |
  |  * RCC Clock Gating: APB1ENR (0x40023840)                                   |
  |  * USART2 Engine (0x40004400): 115200 Baud Generator, SR, DR, CR1          |
  |  * IWDG Independent Watchdog (0x40003000): Dedicated 32kHz LSI Clock        |
  +-----------------------------------------------------------------------------+
```

### Lock-Free Asynchronous ISR Decoupling

ZeroHAL implements a single-producer single-consumer (SPSC) circular FIFO to decouple the high-priority USART2 interrupt service routine from the application super-loop:

```
                  +----------------------------------------------+
Incoming Byte --> | USART2_IRQHandler (Vector 54 / IRQ 38)       |
(Pin PA3 RX)      +----------------------------------------------+
                                         |
                            ring_buffer_push(char c)
                                         |
                                         v
               +----+----+----+----+----+----+----+----+
  Ring Buffer  | D0 | D1 | D2 | D3 | .. | .. |    |    |  (64 Bytes, Mask 0x3F)
               +----+----+----+----+----+----+----+----+
                 ^                         ^
                 |                         |
              tail (Consumer)           head (Producer)
                 |
        ring_buffer_pop(&char)
                 |
                 v
+---------------------------------------------------------------+
| Main Application Super-Loop (cli_handle_char -> cli_execute)  |
+---------------------------------------------------------------+
```

---

## Silicon Memory Map Reference

| Peripheral / Register | Address | Description | Bitfield Operation |
| :--- | :--- | :--- | :--- |
| `STK_CTRL` | `0xE000E010` | SysTick Control & Status Register | `ENABLE=1`, `TICKINT=1`, `CLKSOURCE=1` |
| `STK_LOAD` | `0xE000E014` | SysTick Reload Value Register | `15,999` (1ms heartbeat @ 16 MHz HSI) |
| `STK_VAL` | `0xE000E018` | SysTick Current Value Counter | Reset to `0` during initialization |
| `NVIC_ISER1` | `0xE000E104` | NVIC Interrupt Set-Enable Register 1 | Bit 6 unmasks IRQ 38 (USART2) |
| `SCB_AIRCR` | `0xE000ED0C` | Application Interrupt & Reset Control | Key `0x05FA` + `SYSRESETREQ` (Bit 2) |
| `RCC_AHB1ENR` | `0x40023830` | AHB1 Peripheral Clock Enable | Bit 0: Gating clock to GPIO Port A |
| `RCC_APB1ENR` | `0x40023840` | APB1 Peripheral Clock Enable | Bit 17: Gating clock to USART2 peripheral |
| `RCC_CSR` | `0x40023874` | Clock Control & Status Register | Bit 29: `IWDGRSTF` (Watchdog reset flag) |
| `GPIOA_MODER` | `0x40020000` | GPIO Port A Mode Register | PA2/PA3 Alternate Function (`10b`), PA5 Out (`01b`) |
| `GPIOA_ODR` | `0x40020014` | GPIO Port A Output Data Register | Bit 5: PA5 Green User LED State |
| `GPIOA_PUPDR` | `0x4002000C` | GPIO Port A Pull-up/Pull-down | Bit 6: Pull-up enabled on PA3 RX line |
| `GPIOA_AFRL` | `0x40020020` | GPIO Alternate Function Low Register | Bits 8-15: PA2 and PA3 routed to `AF7` |
| `USART2_SR` | `0x40004400` | USART2 Status Register | Bit 7: `TXE` (Transmitter empty), Bit 5: `RXNE` |
| `USART2_DR` | `0x40004404` | USART2 Data Register | 8-bit payload read / write |
| `USART2_BRR` | `0x40004408` | USART2 Baud Rate Register | `0x008B` (115200 bps @ 16 MHz HSI) |
| `USART2_CR1` | `0x4000440C` | USART2 Control Register 1 | `UE=1`, `TXE=1`, `RXE=1`, `RXNEIE=1` |
| `IWDG_KR` | `0x40003000` | Watchdog Key Register | `0x5555` unlock, `0xAAAA` feed, `0xCCCC` start |
| `IWDG_PR` | `0x40003004` | Watchdog Prescaler Register | `0x04` = /64 prescaler (500 Hz @ 32 kHz LSI) |
| `IWDG_RLR` | `0x40003008` | Watchdog Reload Register | `1000` = 2000 ms hardware safety timeout |

---

## Repository Structure

```
ZeroHAL/
├── .github/
│   └── workflows/
│       └── ci.yml               # GitHub Actions: host tests + ARM cross-compilation
├── include/                     # Public Modular C Headers
│   ├── zero_hal.h               # Umbrella header
│   ├── stm32f4xx_regs.h         # Memory map & volatile register definitions
│   ├── ring_buffer.h            # Lock-free SPSC circular FIFO
│   ├── uart.h                   # Bare-metal USART2 serial driver
│   ├── systick.h                # Core SysTick 1ms timer driver
│   ├── iwdg.h                   # ISO 26262 Independent Watchdog driver
│   └── cli.h                    # Zero-libc diagnostic parser
├── src/                         # Bare-Metal Implementation Files
│   ├── main.c                   # Super-loop & application runtime
│   ├── startup.c                # Pure C vector table & flash-to-RAM bootstrap
│   ├── uart.c                   # USART2 peripheral driver implementation
│   ├── systick.c                # SysTick timer implementation
│   ├── iwdg.c                   # Watchdog driver implementation
│   └── cli.c                    # Interactive command line parser
├── sim/
│   └── simulator.c              # Silicon-level bus transaction emulator
├── tests/
│   └── test_runner.c            # Automated unit test suite (40 assertions)
├── linker.ld                    # Bare-metal memory map linker script
├── Makefile                     # Multi-target build system (firmware, sim, test, clean)
├── .gitignore                   # Clean ignore rules for embedded artifacts
├── .gitattributes               # Line-ending normalization (LF)
├── LICENSE                      # MIT Open-Source License
└── README.md                    # Technical documentation & reference manual
```

---

## Quickstart: Host Silicon Simulator (No Hardware Required)

You do **not** need physical hardware to inspect ZeroHAL. The built-in host silicon emulator models the physical 32-bit address space, bus transactions, and register mailboxes.

### 1. Build and Run the Simulator
```bash
make sim
./build/simulator
```
*(On Windows: `mingw32-make sim` then `.\build\simulator.exe`)*

### 2. What the Simulator Demonstrates
- Simulates clock gating transactions on AHB1 and APB1 bus bridges.
- Visualizes pin multiplexer routing (PA2 -> AF7, PA3 -> AF7).
- Logs baud rate calculation down to mantissa and fractional division.
- Provides a live interactive terminal replicating the real on-silicon ECU console.

---

## Automated Unit Testing & Verification

ZeroHAL includes an automated test runner (`tests/test_runner.c`) executing 40 unit assertions across 4 test groups:
1. **Lock-Free FIFO Invariants**: Empty state, FIFO ordering, boundary saturation, and bitwise index wraparound across power-of-2 boundaries.
2. **Hardware Math & Timing**: SysTick 1ms reload formula, USART2 BRR baud calculation, and IWDG prescaler/reload timeouts.
3. **Register Memory Map Integrity**: Verifies exact peripheral base addresses and NVIC IRQ bit shifts against the STM32F4 reference manual.
4. **Zero-Libc CLI Parser**: String comparison, command recognition, and buffer boundary safety.

### Run Tests:
```bash
make test
```

Sample output:
```
=======================================================
   ZeroHAL Automated Host Test Suite (ISO 26262 CI)    
=======================================================

=== [TEST GROUP 1] Lock-Free Ring Buffer (SPSC FIFO) ===
  [PASS] New ring buffer is initialized empty
  [PASS] FIFO ordering preserved identically
  [PASS] Circular bitwise wraparound (& 0x3F) flawless across 200 cycles
...
>> SUCCESS: 40 / 40 TESTS PASSED! ZERO REGRESSIONS. <<
```

---

## Building Bare-Metal Silicon Firmware

To cross-compile the binary targeting ARM Cortex-M silicon:

### Prerequisites
- **Ubuntu/Debian**: `sudo apt-get install gcc-arm-none-eabi binutils-arm-none-eabi`
- **macOS**: `brew install --cask gcc-arm-embedded`
- **Windows**: Install the [ARM GNU Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)

### Compile
```bash
make firmware
```

### Generated Artifacts in `build/`:
- `zero_hal.elf`: Full Executable and Linkable Format binary with symbol tables for GDB.
- `zero_hal.bin`: Raw flashable binary image.
- `zero_hal.hex`: Intel HEX file for hardware programmers.
- `zero_hal.map`: Memory layout map detailing the exact byte placement of every symbol.

---

## Flashing to Hardware

### Target Hardware Compatibility
Tested on **STM32F4 Series** (STM32F401, STM32F411, STM32F446):
- **STM32 Nucleo-F401RE / Nucleo-F411RE / Nucleo-F446RE**
- **STM32F4 Discovery**
- **BlackPill STM32F401 / STM32F411**

### Pin Connection Reference
Connect any USB-to-UART adapter (FTDI / CP2102 / CH340) or use the onboard ST-Link Virtual COM port:

| Microcontroller Pin | Signal Name | Connection | Function |
| :--- | :--- | :--- | :--- |
| **PA2** | `USART2_TX` | USB-UART **RX** | Microcontroller serial output |
| **PA3** | `USART2_RX` | USB-UART **TX** | Microcontroller serial input (pull-up enabled) |
| **PA5** | `GPIO_OUT` | Onboard **LD2** | Status / Heartbeat green LED |
| **GND** | `GND` | USB-UART **GND** | Common ground reference |

### Flashing via OpenOCD
```bash
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
        -c "program build/zero_hal.elf verify reset exit"
```

### Flashing via ST-Link CLI
```bash
st-flash write build/zero_hal.bin 0x08000000
```

---

## Interactive ECU Diagnostic Console

Open any serial terminal (PuTTY, Minicom, Picocom, screen) at `115200 baud, 8 data bits, no parity, 1 stop bit (8N1)`:

```
================================================
 [ZeroHAL] Bare-Metal Embedded OS v2.0 Online!  
================================================
[BOOT] Cold start / Software reset detected.

[IWDG] Hardware Watchdog Armed: 2000ms timeout

ZeroHAL> 
```

### Diagnostic Command Reference

| Command | Action | System Response |
| :--- | :--- | :--- |
| `help` | Display command reference | Prints interactive manual |
| `status` | Query system telemetry | Returns uptime (s/ms), LED state, and IWDG status |
| `led on` | Set PA5 voltage HIGH (3.3V) | Illuminates onboard green LED |
| `led off` | Set PA5 voltage LOW (0.0V) | Extinguishes onboard green LED |
| `led toggle`| Invert PA5 voltage | Toggles green LED state |
| `feed` | Refresh hardware watchdog | Refreshes IWDG 2000ms downcounter (`0xAAAA`) |
| `hang` | Simulate ECU firmware stall | Loops without feeding; triggers silicon watchdog reset in 2s |
| `reset` | Issue software reset | Issues CPU reset via `SCB_AIRCR` key `0x05FA` |

### Demonstrating Watchdog Recovery
Typing `hang` simulates a firmware deadlock. The hardware downcounter expires after 2000ms, asserting a silicon reset:

```
ZeroHAL> hang

[SIMULATION] Entering infinite loop without feeding watchdog...
[SIMULATION] IWDG hardware will force-reset MCU in 2 seconds!

================================================
 [ZeroHAL] Bare-Metal Embedded OS v2.0 Online!  
================================================
[SAFETY ALERT] System recovered from IWDG WATCHDOG TIMEOUT!
[SAFETY ALERT] Hardware reset asserted safely. Restoring operations.

[IWDG] Hardware Watchdog Armed: 2000ms timeout

ZeroHAL> 
```

---

## Silicon Boot Sequence (Pure C Bootstrap)

ZeroHAL completely dispenses with assembly startup files (`.s`). The entire power-on sequence is written in ISO 26262-compliant, pure C in [`src/startup.c`](src/startup.c):

1. **Vector Table Placement**: The `g_pfnVectors` array is positioned at physical Flash address `0x08000000` via `.isr_vector` in [`linker.ld`](linker.ld). The CPU hardware loads the initial Stack Pointer (`_estack`) and branches to `Reset_Handler`.
2. **Flash-to-RAM Data Migration**: `Reset_Handler` iterates through `.data` Load Memory Address (LMA in Flash) and copies every byte to Virtual Memory Address (VMA in SRAM).
3. **BSS Section Zeroing**: Iterates across the `.bss` boundaries (`_sbss` to `_ebss`) in SRAM, clearing all uninitialized static and global variables to zero.
4. **Hardware FPU Activation**: Enables full access to coprocessors `CP10` and `CP11` in `SCB_CPACR` (`0xE000ED88`) to activate hardware single-precision floating point.
5. **Entry to Application**: Hands execution control over to `main()`. If `main()` ever returns, the CPU enters a deterministic `wfi` (Wait For Interrupt) low-power sleep trap.

---

## Contributing & Development

Pull requests and code reviews are welcome! When submitting changes:
1. Ensure all code compiles cleanly with `-Wall -Wextra -Werror`.
2. Run `make test` to confirm all 40 unit assertions pass.
3. Validate silicon simulation behavior with `make sim`.

---

## License

This project is licensed under the [MIT License](LICENSE) — free for academic, commercial, and safety-critical research use.
