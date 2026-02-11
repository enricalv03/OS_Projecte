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

# --- Compiler flags ---
# -nostdlib -nostdinc    : no standard library or includes (freestanding)
# -fno-builtin           : don't use GCC built-in function implementations
# -fno-stack-protector   : no stack canary (we have no __stack_chk_fail)
# -nostartfiles          : no crt0.o
# -nodefaultlibs         : no default libraries
# -Wall -Wextra          : all warnings
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
         -nostartfiles -nodefaultlibs -Wall -Wextra -c

ASMFLAGS_BIN = -f bin
ASMFLAGS_ELF = -f elf32

# --- Build directory ---
BUILD = build

# ==============================================================================
# Bootloader
# ==============================================================================

$(BUILD)/boot.bin: boot/boot.asm
	$(ASM) $(ASMFLAGS_BIN) $< -o $@

$(BUILD)/stage2.bin: boot/stage2.asm
	$(ASM) $(ASMFLAGS_BIN) $< -o $@

# ==============================================================================
# Core kernel (ASM)
# ==============================================================================

$(BUILD)/kernel.o: kernel/core/kernel.asm
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

$(BUILD)/paging.o: kernel/core/paging.asm
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

$(BUILD)/interrupts.o: kernel/core/interrupts.asm
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

$(BUILD)/exceptions.o: kernel/core/exceptions.asm
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

$(BUILD)/pit.o: kernel/core/pit.asm
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

# Core kernel (C)
$(BUILD)/kernel_c.o: kernel/core/kernel_c.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/gdt.o: kernel/core/gdt.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/gdt_flush.o: kernel/core/gdt_flush.asm
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

$(BUILD)/usermode.o: kernel/core/usermode.asm
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

# ==============================================================================
# Drivers (ASM)
# ==============================================================================

$(BUILD)/screen.o: kernel/drivers/screen.asm
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

$(BUILD)/keyboard.o: kernel/drivers/keyboard.asm
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

$(BUILD)/cursor.o: kernel/drivers/cursor.asm
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

# Drivers (C)
$(BUILD)/keyboard_c.o: kernel/drivers/keyboard.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/kbd_buffer.o: kernel/drivers/kbd_buffer.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/ata.o: kernel/drivers/ata.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/block.o: kernel/drivers/block.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/ata_test.o: kernel/drivers/ata_test.c
	$(CC) $(CFLAGS) $< -o $@

# ==============================================================================
# Memory management
# ==============================================================================

$(BUILD)/memory.o: kernel/memory/memory.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/pmm.o: kernel/memory/pmm.asm
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

$(BUILD)/vmm.o: kernel/memory/vmm.asm
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

$(BUILD)/heap.o: kernel/memory/heap.c
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

# ==============================================================================
# Process management
# ==============================================================================

$(BUILD)/process.o: kernel/process/process.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/scheduler.o: kernel/process/scheduler.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/context_switch.o: kernel/process/context_switch.asm
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

# ==============================================================================
# System calls
# ==============================================================================

$(BUILD)/syscall.o: kernel/syscall/syscall.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/syscall_asm.o: kernel/syscall/syscall_asm.asm
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

# ==============================================================================
# Command system and languages
# ==============================================================================

$(BUILD)/commands.o: kernel/commands/commands.asm
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

$(BUILD)/languages_en.o: kernel/commands/languages/en.asm
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

$(BUILD)/languages_ca.o: kernel/commands/languages/ca.asm
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

$(BUILD)/languages_es.o: kernel/commands/languages/es.asm
	$(ASM) $(ASMFLAGS_ELF) $< -o $@

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
	$(BUILD)/vfs.o \
	$(BUILD)/ramfs.o \
	$(BUILD)/diskfs.o \
	$(BUILD)/kstring.o \
	$(BUILD)/ata.o \
	$(BUILD)/block.o \
	$(BUILD)/ata_test.o \
	$(BUILD)/process.o \
	$(BUILD)/scheduler.o \
	$(BUILD)/context_switch.o \
	$(BUILD)/syscall.o \
	$(BUILD)/syscall_asm.o

$(BUILD)/kernel.bin: $(KERNEL_OBJS)
	$(LD) -T kernel/linker.ld $(KERNEL_OBJS) -o $(BUILD)/kernel.elf
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
# Phony targets
# ==============================================================================

all: $(BUILD)/disk.img

clean:
	rm -f $(BUILD)/*

.PHONY: all run debug clean
