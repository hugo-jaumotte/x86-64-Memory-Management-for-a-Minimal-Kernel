# Makefile for the kernel, UEFI bootloader, disk image, and emulator

# Declare phony targets.
.PHONY: all build clean kernel kernel-c bootloader bootimg run

BUILD_DIR := build
SRC_DIR   := src

# Tools
CLANG   := clang
LD_LLD  := ld.lld
OBJDUMP := objdump

# OVMF / QEMU
OVMF_CODE := /usr/share/OVMF/OVMF_CODE_4M.fd
OVMF_VARS := /usr/share/OVMF/OVMF_VARS_4M.fd

# Output files
KERNEL_BIN := $(BUILD_DIR)/kernel.bin
KERNEL_MAP := $(BUILD_DIR)/link.map
EFI_BIN    := $(BUILD_DIR)/BOOTX64.EFI
DISK_IMG   := $(BUILD_DIR)/disk.img

# --- KERNEL (Freestanding ELF -> flat binary) ---
CFLAGS_KERNEL := -target x86_64-elf \
	-ffreestanding -nostdlib \
	-fno-pie -fno-pic -mno-red-zone \
	-fno-stack-protector \
	-Wall -Wextra -O0 -g

KERNEL_LD := $(SRC_DIR)/kernel.ld

KERNEL_SRCS := \
	$(SRC_DIR)/paging.c \
	$(SRC_DIR)/pci.c \
	$(SRC_DIR)/pmm.c \
	$(SRC_DIR)/kernel.c

# Generate the list of object files from the kernel sources.
KERNEL_OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(KERNEL_SRCS))


# --- UEFI BOOTLOADER (PE/COFF) ---
# Compile and link the bootloader as a UEFI application.
CFLAGS_EFI := -target x86_64-pc-win32-coff -ffreestanding -fshort-wchar \
	-mno-red-zone -fno-stack-protector -nostdlib \
	-fuse-ld=lld-link

LDFLAGS_EFI := -Wl,-entry:efi_main -Wl,-subsystem:efi_application

BOOTLOADER_SRC := $(SRC_DIR)/bootloader.c

# Default target.
all: build bootloader kernel-c

build:
	mkdir -p $(BUILD_DIR)

# Convenience alias.
kernel: kernel-c

# Compile kernel source files.
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | build
	$(CLANG) $(CFLAGS_KERNEL) -c $< -o $@

# Link the kernel as a flat binary.
# The linker script defines the memory layout.
# A link map is generated for debugging purposes.
kernel-c: $(KERNEL_OBJS)
	$(LD_LLD) -Ttext=0x100000 --oformat=binary -T $(KERNEL_LD) --Map=$(KERNEL_MAP) \
		$(KERNEL_OBJS) -o $(KERNEL_BIN)
	$(OBJDUMP) -b binary -m i386:x86-64 --adjust-vma=0x100000 -D $(KERNEL_BIN) | head -n 30

# Build the UEFI bootloader.
bootloader: $(EFI_BIN)

$(EFI_BIN): $(BOOTLOADER_SRC) | build
	$(CLANG) $(CFLAGS_EFI) $(LDFLAGS_EFI) $< -o $@

# Build a GPT disk image containing an EFI System Partition (ESP).
# The bootloader is copied to the standard UEFI path (/EFI/BOOT/BOOTX64.EFI),
# while the kernel is placed at the root of the partition.
bootimg: all
	# Create a 64 MiB disk image.
	dd if=/dev/zero of=$(DISK_IMG) bs=1M count=64

	# Create a GPT partition table with an EFI System Partition.
	parted -s $(DISK_IMG) mklabel gpt 2>/dev/null
	parted -s $(DISK_IMG) mkpart ESP fat32 1MiB 100%
	parted -s $(DISK_IMG) set 1 esp on 2>/dev/null

	# Format the EFI partition as FAT32.
	mformat -i $(DISK_IMG)@@1048576 -F ::

	# Populate the EFI partition.
	mmd   -i $(DISK_IMG)@@1048576 ::/EFI
	mmd   -i $(DISK_IMG)@@1048576 ::/EFI/BOOT
	mcopy -i $(DISK_IMG)@@1048576 $(EFI_BIN) ::/EFI/BOOT/
	mcopy -i $(DISK_IMG)@@1048576 $(KERNEL_BIN) ::/

# Run the system using QEMU with OVMF (UEFI firmware).
# A virtual disk image is used as the boot device.
run: bootimg
	cp $(OVMF_VARS) $(BUILD_DIR)/OVMF_VARS.fd
	qemu-system-x86_64 \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE) \
		-drive if=pflash,format=raw,file=$(BUILD_DIR)/OVMF_VARS.fd \
		-drive if=virtio,format=raw,file=$(DISK_IMG) \
		-nographic -serial mon:stdio -no-reboot -no-shutdown \
		-monitor unix:/tmp/qemu-monitor,server,nowait

clean:
	rm -rf $(BUILD_DIR)