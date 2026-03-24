# ==============================================================================
# MyOS Build System
# ==============================================================================
# Targets:
#   all       - Build everything (default)
#   run       - Build and launch in QEMU
#   clean     - Remove all build artifacts
#   debug     - Build and launch in QEMU with GDB server
#
# Toolchain: i686-elf-gcc cross compiler (freestanding, 32-bit)
# Assembler: NASM
# ==============================================================================

# --- Toolchain ---
CC      = i686-elf-gcc
LD      = i686-elf-ld
ASM     = nasm
OBJCOPY = i686-elf-objcopy
QEMU    = qemu-system-x86_64

# --- Architecture selection ---
# For now we only build the x86 kernel, but the layout is ready
# to support other architectures under arch/<arch>/ in the future.
ARCH      = x86
ARCH_DIR  = arch/$(ARCH)
KERNELDIR = $(ARCH_DIR)/kernel
BOOTDIR   = $(ARCH_DIR)/boot

# --- Compiler flags ---
# -nostdlib -nostdinc    : no standard library or includes (freestanding)
# -fno-builtin           : don't use GCC built-in function implementations
# -fno-stack-protector   : no stack canary (we have no __stack_chk_fail)
# -nostartfiles          : no crt0.o
# -nodefaultlibs         : no default libraries
# -Wall -Wextra          : all warnings
# -Ikernel         lets any file use #include "lib/kstring.h", "fs/vfs.h", etc.
# -I$(KERNELDIR)   lets any file use #include "memory/vmm.h", "drivers/ata.h", etc.
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
         -nostartfiles -nodefaultlibs -Wall -Wextra \
         -Ikernel -I$(KERNELDIR) \
         -c

ASMFLAGS_BIN = -f bin
ASMFLAGS_ELF = -f elf32

# --- Build directory ---
BUILD = build

$(BUILD):
	mkdir -p $(BUILD)

# ==============================================================================
# Bootloader
# ==============================================================================

$(BUILD)/boot.bin: $(BOOTDIR)/boot.asm
	$(ASM) $(ASMFLAGS_BIN) $< -o $@

$(BUILD)/stage2.bin: $(BOOTDIR)/stage2.asm
	$(ASM) $(ASMFLAGS_BIN) $< -o $@

# ==============================================================================
# Core kernel (ASM)
# ==============================================================================

$(BUILD)/kernel.o: $(KERNELDIR)/core/kernel.asm
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

$(BUILD)/paging.o: $(KERNELDIR)/core/paging.asm
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

$(BUILD)/interrupts.o: $(KERNELDIR)/core/interrupts.asm
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

$(BUILD)/exceptions.o: $(KERNELDIR)/core/exceptions.asm
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

$(BUILD)/pit.o: $(KERNELDIR)/core/pit.asm
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

# Core kernel (C)
$(BUILD)/kernel_c.o: kernel/core/kernel_c.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/gdt.o: kernel/core/gdt.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/gdt_flush.o: $(KERNELDIR)/core/gdt_flush.asm
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

$(BUILD)/usermode.o: $(KERNELDIR)/core/usermode.asm
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

$(BUILD)/arch.o: $(KERNELDIR)/core/arch.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/smp.o: $(KERNELDIR)/core/smp.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/panic.o: kernel/panic.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/node.o: kernel/node.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/i18n.o: kernel/i18n.c
	$(CC) $(CFLAGS) $< -o $@

# ==============================================================================
# Drivers (ASM)
# ==============================================================================

$(BUILD)/screen.o: $(KERNELDIR)/drivers/screen.asm
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

$(BUILD)/keyboard.o: $(KERNELDIR)/drivers/keyboard.asm
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

$(BUILD)/cursor.o: $(KERNELDIR)/drivers/cursor.asm
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

# Drivers (C)
$(BUILD)/keyboard_c.o: $(KERNELDIR)/drivers/keyboard.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/kbd_buffer.o: $(KERNELDIR)/drivers/kbd_buffer.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/ata.o: $(KERNELDIR)/drivers/ata.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/block.o: $(KERNELDIR)/drivers/block.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/ata_test.o: $(KERNELDIR)/drivers/ata_test.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/fb.o: $(KERNELDIR)/drivers/fb.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/compositor.o: kernel/gfx/compositor.c
	$(CC) $(CFLAGS) $< -o $@

# ==============================================================================
# Memory management
# ==============================================================================

$(BUILD)/memory.o: $(KERNELDIR)/memory/memory.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/pmm.o: $(KERNELDIR)/memory/pmm.asm
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

$(BUILD)/vmm.o: $(KERNELDIR)/memory/vmm.asm
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

$(BUILD)/heap.o: $(KERNELDIR)/memory/heap.c
	$(CC) $(CFLAGS) $< -o $@

# ==============================================================================
# Filesystem
# ==============================================================================

$(BUILD)/vfs.o: kernel/fs/vfs.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/ramfs.o: kernel/fs/ramfs.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/diskfs.o: kernel/fs/diskfs.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/elf.o: kernel/fs/elf.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/devfs.o: kernel/fs/devfs.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/pipe.o: kernel/ipc/pipe.c
	$(CC) $(CFLAGS) $< -o $@

# ==============================================================================
# Networking stack
# ==============================================================================

$(BUILD)/net.o: kernel/net/net.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/ethernet.o: kernel/net/ethernet.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/arp.o: kernel/net/arp.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/ip.o: kernel/net/ip.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/icmp.o: kernel/net/icmp.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/udp.o: kernel/net/udp.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/tcp.o: kernel/net/tcp.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/virtio_net.o: kernel/net/virtio_net.c
	$(CC) $(CFLAGS) $< -o $@

# ==============================================================================
# Process management
# ==============================================================================

$(BUILD)/process.o: kernel/sched/process.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/scheduler.o: kernel/sched/scheduler.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/context_switch.o: $(KERNELDIR)/process/context_switch.asm
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

# ==============================================================================
# System calls
# ==============================================================================

$(BUILD)/syscall.o: kernel/sys/syscall.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/syscall_asm.o: kernel/syscall/syscall_asm.asm
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

# ==============================================================================
# Command system and languages
# ==============================================================================

$(BUILD)/commands.o: $(KERNELDIR)/commands/commands.asm
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

$(BUILD)/languages_en.o: $(KERNELDIR)/commands/languages/en.asm
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

$(BUILD)/languages_ca.o: $(KERNELDIR)/commands/languages/ca.asm
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

$(BUILD)/languages_es.o: $(KERNELDIR)/commands/languages/es.asm
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

$(BUILD)/tab_complete.o: $(KERNELDIR)/commands/tab_complete.c
	$(CC) $(CFLAGS) $< -o $@

# ==============================================================================
# Kernel library
# ==============================================================================

$(BUILD)/kstring.o: kernel/lib/kstring.c
	$(CC) $(CFLAGS) $< -o $@

# ==============================================================================
# Link the kernel
# ==============================================================================

# All object files that make up the kernel
KERNEL_OBJS = \
	$(BUILD)/kernel.o \
	$(BUILD)/kernel_c.o \
	$(BUILD)/gdt.o \
	$(BUILD)/gdt_flush.o \
	$(BUILD)/usermode.o \
	$(BUILD)/arch.o \
	$(BUILD)/smp.o \
	$(BUILD)/panic.o \
	$(BUILD)/node.o \
	$(BUILD)/i18n.o \
	$(BUILD)/screen.o \
	$(BUILD)/keyboard.o \
	$(BUILD)/keyboard_c.o \
	$(BUILD)/exceptions.o \
	$(BUILD)/pit.o \
	$(BUILD)/interrupts.o \
	$(BUILD)/kbd_buffer.o \
	$(BUILD)/memory.o \
	$(BUILD)/cursor.o \
	$(BUILD)/paging.o \
	$(BUILD)/pmm.o \
	$(BUILD)/vmm.o \
	$(BUILD)/commands.o \
	$(BUILD)/heap.o \
	$(BUILD)/languages_en.o \
	$(BUILD)/languages_ca.o \
	$(BUILD)/languages_es.o \
	$(BUILD)/tab_complete.o \
	$(BUILD)/vfs.o \
	$(BUILD)/ramfs.o \
	$(BUILD)/diskfs.o \
	$(BUILD)/elf.o \
	$(BUILD)/devfs.o \
	$(BUILD)/pipe.o \
	$(BUILD)/net.o \
	$(BUILD)/ethernet.o \
	$(BUILD)/arp.o \
	$(BUILD)/ip.o \
	$(BUILD)/icmp.o \
	$(BUILD)/udp.o \
	$(BUILD)/tcp.o \
	$(BUILD)/virtio_net.o \
	$(BUILD)/kstring.o \
	$(BUILD)/ata.o \
	$(BUILD)/block.o \
	$(BUILD)/ata_test.o \
	$(BUILD)/fb.o \
	$(BUILD)/compositor.o \
	$(BUILD)/process.o \
	$(BUILD)/scheduler.o \
	$(BUILD)/context_switch.o \
	$(BUILD)/syscall.o \
	$(BUILD)/syscall_asm.o

$(BUILD)/kernel.bin: $(KERNEL_OBJS)
	$(LD) $(LDFLAGS) -T $(KERNELDIR)/linker.ld $(KERNEL_OBJS) -o $(BUILD)/kernel.elf
	$(OBJCOPY) -O binary $(BUILD)/kernel.elf $(BUILD)/kernel.bin

# ==============================================================================
# Disk image
# ==============================================================================
# Layout:
#   Sector 0     : MBR bootloader (boot.bin)
#   Sector 1-2   : Stage 2 bootloader (stage2.bin)
#   Sector 3+    : Kernel binary (kernel.bin)

$(BUILD)/disk.img: $(BUILD)/boot.bin $(BUILD)/stage2.bin $(BUILD)/kernel.bin
	dd if=/dev/zero of=$(BUILD)/disk.img bs=512 count=2880
	dd if=$(BUILD)/boot.bin of=$(BUILD)/disk.img bs=512 count=1 seek=0 conv=notrunc
	dd if=$(BUILD)/stage2.bin of=$(BUILD)/disk.img bs=512 count=2 seek=1 conv=notrunc
	dd if=$(BUILD)/kernel.bin of=$(BUILD)/disk.img bs=512 seek=3 conv=notrunc
	@# Write SimpleFS filesystem with test files (if any exist in diskfiles/)
	@if [ -d diskfiles ] && ls diskfiles/*.txt >/dev/null 2>&1; then \
		python3 tools/mkdiskfs.py $(BUILD)/disk.img diskfiles/*.txt; \
	fi

# ==============================================================================
# Run targets
# ==============================================================================

run: $(BUILD)/disk.img
	$(QEMU) -drive format=raw,file=$(BUILD)/disk.img

debug: $(BUILD)/disk.img
	$(QEMU) -drive format=raw,file=$(BUILD)/disk.img -s -S &
	@echo "GDB server listening on :1234. Connect with: target remote :1234"

# ==============================================================================
# ARM skeleton build and run  (requires arm-none-eabi-gcc and qemu-system-arm)
# ==============================================================================
# Install toolchain on macOS:   brew install arm-none-eabi-gcc qemu
# Usage:                        make run-arm
#
# QEMU machine: -M virt  (generic ARM virtual board)
# CPU:          -cpu cortex-a15
# Memory:       -m 128M
# Console:      -serial stdio (PL011 UART output to terminal)

ARM_CC      = arm-none-eabi-gcc
ARM_LD      = arm-none-eabi-ld
ARM_OBJCOPY = arm-none-eabi-objcopy
ARM_QEMU    = qemu-system-arm

ARM_ARCH_DIR  = arch/arm
ARM_KERN_DIR  = $(ARM_ARCH_DIR)/kernel
ARM_BUILD     = $(BUILD)/arm

ARM_CFLAGS = -mcpu=cortex-a15 -marm -ffreestanding -nostdlib -nostdinc \
             -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs \
             -Wall -Wextra -Ikernel -I$(ARM_KERN_DIR)/core -c

$(ARM_BUILD):
	mkdir -p $(ARM_BUILD)

ARM_CFLAGS_INC = $(ARM_CFLAGS) -I$(ARM_KERN_DIR) -I$(ARM_KERN_DIR)/core \
                 -I$(ARM_KERN_DIR)/memory

$(ARM_BUILD)/arm_boot.o: $(ARM_ARCH_DIR)/boot/boot.S | $(ARM_BUILD)
	$(ARM_CC) $(ARM_CFLAGS_INC) -x assembler-with-cpp $< -o $@

$(ARM_BUILD)/arm_vectors.o: $(ARM_KERN_DIR)/core/vectors.S | $(ARM_BUILD)
	$(ARM_CC) $(ARM_CFLAGS_INC) -x assembler-with-cpp $< -o $@

$(ARM_BUILD)/arm_arch.o: $(ARM_KERN_DIR)/core/arch.c | $(ARM_BUILD)
	$(ARM_CC) $(ARM_CFLAGS_INC) $< -o $@

$(ARM_BUILD)/arm_main.o: $(ARM_KERN_DIR)/core/arm_main.c | $(ARM_BUILD)
	$(ARM_CC) $(ARM_CFLAGS_INC) $< -o $@

$(ARM_BUILD)/arm_irq.o: $(ARM_KERN_DIR)/core/arm_irq.c | $(ARM_BUILD)
	$(ARM_CC) $(ARM_CFLAGS_INC) $< -o $@

$(ARM_BUILD)/arm_pmm.o: $(ARM_KERN_DIR)/memory/arm_pmm.c | $(ARM_BUILD)
	$(ARM_CC) $(ARM_CFLAGS_INC) $< -o $@

$(ARM_BUILD)/arm_vmm.o: $(ARM_KERN_DIR)/memory/arm_vmm.c | $(ARM_BUILD)
	$(ARM_CC) $(ARM_CFLAGS_INC) $< -o $@

$(ARM_BUILD)/arm_ctx.o: $(ARM_KERN_DIR)/process/context_switch_arm.S | $(ARM_BUILD)
	$(ARM_CC) $(ARM_CFLAGS_INC) -x assembler-with-cpp $< -o $@

ARM_OBJS = $(ARM_BUILD)/arm_boot.o \
           $(ARM_BUILD)/arm_vectors.o \
           $(ARM_BUILD)/arm_arch.o \
           $(ARM_BUILD)/arm_main.o \
           $(ARM_BUILD)/arm_irq.o \
           $(ARM_BUILD)/arm_pmm.o \
           $(ARM_BUILD)/arm_vmm.o \
           $(ARM_BUILD)/arm_ctx.o

$(ARM_BUILD)/kernel.elf: $(ARM_OBJS)
	$(ARM_LD) -T $(ARM_KERN_DIR)/linker.ld $(ARM_OBJS) -o $@

run-arm: $(ARM_BUILD)/kernel.elf
	$(ARM_QEMU) -M virt -cpu cortex-a15 -m 128M \
	            -kernel $(ARM_BUILD)/kernel.elf \
	            -serial stdio -display none \
	            -nographic

# ==============================================================================
# Phony targets
# ==============================================================================

all: $(BUILD) $(BUILD)/disk.img

clean:
	rm -f $(BUILD)/*

.PHONY: all run debug clean run-arm
