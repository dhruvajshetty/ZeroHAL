# ==============================================================================
# Project: ZeroHAL
# File: Makefile
# Description: Bare-metal ARM Cortex-M4 & Host Simulation Multi-Target Build System
# Standard: ISO 26262 Deterministic Compilation & Automated Testing
# ==============================================================================

TARGET    ?= zero_hal
BUILD_DIR ?= build

# Cross-compilation toolchain for ARM Cortex-M4
ARM_CC      ?= arm-none-eabi-gcc
ARM_OBJCOPY ?= arm-none-eabi-objcopy
ARM_SIZE    ?= arm-none-eabi-size

# Host compiler for simulator and test suite
HOST_CC     ?= gcc

# MCU Architecture Flags (ARMv7E-M Cortex-M4 with single-precision hardware FPU)
MCU_FLAGS   = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

# Bare-metal compilation flags (strict warnings, no standard library, freestanding)
ARM_CFLAGS  = $(MCU_FLAGS) -Iinclude -Wall -Wextra -Werror -O2 -g \
              -ffreestanding -fno-builtin -nostdlib -nostartfiles

# Linker flags: deterministic memory placement via custom linker script
ARM_LDFLAGS = $(MCU_FLAGS) -Tlinker.ld -nostdlib -nostartfiles \
              -Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(TARGET).map

# Host flags for simulation and unit test runner
HOST_CFLAGS = -Iinclude -Wall -Wextra -Werror -O2

# Bare-metal firmware sources
SRCS        = src/main.c src/startup.c src/uart.c src/systick.c src/iwdg.c src/cli.c src/os_scheduler.c src/pwm.c
OBJS        = $(patsubst src/%.c, $(BUILD_DIR)/%.o, $(SRCS))

ifeq ($(OS),Windows_NT)
  SHELL := cmd.exe
  MKDIR = if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
  RM    = if exist $(BUILD_DIR) rmdir /s /q $(BUILD_DIR)
  EXE   = .exe
  ARM_CHECK = where $(ARM_CC) >nul 2>nul
else
  SHELL := /bin/sh
  MKDIR = mkdir -p $(BUILD_DIR)
  RM    = rm -rf $(BUILD_DIR)
  EXE   =
  ARM_CHECK = which $(ARM_CC) >/dev/null 2>&1
endif

.PHONY: all firmware sim test check_arm clean help

all: firmware

help:
	@echo ZeroHAL Build Targets:
	@echo   make firmware  - Compile ARM Cortex-M4 bare-metal binary (.elf, .bin, .hex)
	@echo   make sim       - Compile host silicon bus emulator ($(BUILD_DIR)/simulator$(EXE))
	@echo   make test      - Build and run host unit test suite (40+ assertions)
	@echo   make clean     - Remove all build artifacts ($(BUILD_DIR)/)

firmware: check_arm $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/$(TARGET).hex print_size
	@echo [OK] ZeroHAL bare-metal firmware generated successfully in $(BUILD_DIR)/

check_arm:
	@$(ARM_CHECK) || ( \
	  echo ============================================================================== && \
	  echo [NOTE] ARM cross-compiler '$(ARM_CC)' was not found in PATH. && \
	  echo To build the bare-metal silicon binary: && \
	  echo   - Linux/Ubuntu : sudo apt-get install gcc-arm-none-eabi binutils-arm-none-eabi && \
	  echo   - macOS        : brew install --cask gcc-arm-embedded && \
	  echo   - Windows      : Download ARM GNU Toolchain from developer.arm.com && \
	  echo ------------------------------------------------------------------------------ && \
	  echo You can test and inspect ZeroHAL immediately on your host system: && \
	  echo   - 'make sim'   : Run the silicon bus and register emulator && \
	  echo   - 'make test'  : Run the automated unit test suite && \
	  echo ============================================================================== && \
	  exit 1 \
	)

$(BUILD_DIR):
	@$(MKDIR)

$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	@echo [ARM-CC] $<
	@$(ARM_CC) $(ARM_CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJS) linker.ld
	@echo [ARM-LD] $@
	@$(ARM_CC) $(OBJS) $(ARM_LDFLAGS) -o $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	@echo [OBJCOPY] $@
	@$(ARM_OBJCOPY) -O binary $< $@

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	@echo [OBJCOPY] $@
	@$(ARM_OBJCOPY) -O ihex $< $@

print_size: $(BUILD_DIR)/$(TARGET).elf
	@echo [SIZE REPORT]
	@$(ARM_SIZE) $<

# Host silicon emulator target
sim: $(BUILD_DIR)
	@echo [HOST-CC] Compiling silicon bus simulator...
	@$(HOST_CC) $(HOST_CFLAGS) sim/simulator.c -o $(BUILD_DIR)/simulator$(EXE)
	@echo [OK] Silicon simulator built at $(BUILD_DIR)/simulator$(EXE)
	@echo Run it with: ./$(BUILD_DIR)/simulator$(EXE)

# Host unit test runner target
test: $(BUILD_DIR)
	@echo [HOST-CC] Compiling automated test suite...
	@$(HOST_CC) $(HOST_CFLAGS) tests/test_runner.c -o $(BUILD_DIR)/test_runner$(EXE)
	@echo [TEST EXEC] Running tests...
	@$(BUILD_DIR)/test_runner$(EXE)

clean:
	@$(RM)
	@echo [OK] Clean complete.
