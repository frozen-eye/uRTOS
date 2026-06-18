# uRTOS — multi-architecture Makefile
#
# Supported ports:
#   PORT=aarch64  (default)  QEMU virt ARM64
#   PORT=armhf               QEMU virt 32-bit ARM (hard-float)
#   PORT=riscv64             QEMU virt RISC-V 64-bit
#   PORT=riscv32             QEMU virt RISC-V 32-bit

PORT ?= aarch64

BUILD   := build
TARGET  := $(BUILD)/kernel.elf

COMMON_SRCS := \
	boards/common/boot.c \
	kernel/panic.c \
	kernel/heap.c \
	kernel/scheduler.c \
	examples/basic/main.c

.PHONY: all clean run run-aarch64 run-armhf run-riscv64 run-riscv32

ifeq ($(PORT),aarch64)
  CROSS_COMPILE ?= aarch64-linux-gnu-
  ARCH_DIR      := port/aarch64-qemu-virt
  BOARD_DIR     := boards/qemu-virt-aarch64
  QEMU          := qemu-system-aarch64
  QEMU_FLAGS    := -M virt,gic-version=2 -cpu cortex-a53 -nographic -serial mon:stdio
  ARCH_CFLAGS   := -mgeneral-regs-only
  ASM_SRCS      := $(ARCH_DIR)/start.S $(ARCH_DIR)/vectors.S $(ARCH_DIR)/context_switch.S
  PORT_SRCS     := $(ARCH_DIR)/uart.c $(ARCH_DIR)/gic.c $(ARCH_DIR)/timer.c $(ARCH_DIR)/irq.c
else ifeq ($(PORT),armhf)
  CROSS_COMPILE ?= arm-linux-gnueabihf-
  ARCH_DIR      := port/armhf-qemu-virt
  BOARD_DIR     := boards/qemu-virt-armhf
  QEMU          := qemu-system-arm
  QEMU_FLAGS    := -M virt,highmem=off -cpu cortex-a15 -nographic -serial mon:stdio
  ARCH_CFLAGS   := -march=armv7-a -mfpu=vfpv3 -mfloat-abi=hard
  ASM_SRCS      := $(ARCH_DIR)/start.S $(ARCH_DIR)/vectors.S $(ARCH_DIR)/context_switch.S
  PORT_SRCS     := $(ARCH_DIR)/uart.c $(ARCH_DIR)/gic.c $(ARCH_DIR)/timer.c $(ARCH_DIR)/irq.c
else ifeq ($(PORT),riscv64)
  CROSS_COMPILE ?= riscv64-linux-gnu-
  ARCH_DIR      := port/riscv64-qemu-virt
  BOARD_DIR     := boards/qemu-virt-riscv64
  QEMU          := qemu-system-riscv64
  QEMU_FLAGS    := -M virt -cpu rv64 -nographic -serial mon:stdio
  ARCH_CFLAGS   := -march=rv64imac -mabi=lp64 -mcmodel=medany
  ASM_SRCS      := $(ARCH_DIR)/start.S $(ARCH_DIR)/trap.S $(ARCH_DIR)/context_switch.S
  PORT_SRCS     := $(ARCH_DIR)/uart.c $(ARCH_DIR)/timer.c $(ARCH_DIR)/irq.c
else ifeq ($(PORT),riscv32)
  CROSS_COMPILE ?= riscv32-linux-gnu-
  ARCH_DIR      := port/riscv32-qemu-virt
  BOARD_DIR     := boards/qemu-virt-riscv32
  QEMU          := qemu-system-riscv32
  QEMU_FLAGS    := -M virt -cpu rv32 -nographic -serial mon:stdio
  ARCH_CFLAGS   := -march=rv32imac -mabi=ilp32 -mcmodel=medany
  ASM_SRCS      := $(ARCH_DIR)/start.S $(ARCH_DIR)/trap.S $(ARCH_DIR)/context_switch.S
  PORT_SRCS     := $(ARCH_DIR)/uart.c $(ARCH_DIR)/timer.c $(ARCH_DIR)/irq.c
else
  $(error Unsupported PORT=$(PORT). Use aarch64, armhf, riscv64, or riscv32)
endif

CC      := $(CROSS_COMPILE)gcc
AS      := $(CROSS_COMPILE)gcc

CFLAGS  := -ffreestanding -nostdlib -nostartfiles \
           -Wall -Wextra -O2 -g \
           -Iinclude -I$(ARCH_DIR) \
           $(ARCH_CFLAGS)

ASFLAGS := $(CFLAGS)
LDFLAGS := -nostdlib -static -T $(BOARD_DIR)/linker.ld

C_SRCS  := $(COMMON_SRCS) $(PORT_SRCS)
ASM_OBJS := $(patsubst %.S,$(BUILD)/%.o,$(ASM_SRCS))
C_OBJS   := $(patsubst %.c,$(BUILD)/%.o,$(C_SRCS))
OBJS     := $(ASM_OBJS) $(C_OBJS)

all: $(TARGET)

$(BUILD):
	mkdir -p $(BUILD)/boards/common $(BUILD)/kernel $(BUILD)/examples/basic
	mkdir -p $(patsubst %,$(BUILD)/%,$(sort $(dir $(ASM_SRCS) $(PORT_SRCS))))

$(BUILD)/%.o: %.S | $(BUILD)
	$(AS) $(ASFLAGS) -c $< -o $@

$(BUILD)/%.o: %.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS) $(BOARD_DIR)/linker.ld
	$(CC) $(LDFLAGS) $(OBJS) -o $@

clean:
	rm -rf $(BUILD)

run: $(TARGET)
	$(QEMU) $(QEMU_FLAGS) -kernel $(TARGET)

run-aarch64:
	$(MAKE) clean
	$(MAKE) PORT=aarch64
	$(MAKE) PORT=aarch64 run

run-armhf:
	$(MAKE) clean
	$(MAKE) PORT=armhf
	$(MAKE) PORT=armhf run

run-riscv64:
	$(MAKE) clean
	$(MAKE) PORT=riscv64
	$(MAKE) PORT=riscv64 run

run-riscv32:
	$(MAKE) clean
	$(MAKE) PORT=riscv32
	$(MAKE) PORT=riscv32 run
