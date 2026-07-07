# x86-64 Memory Management for a Minimal Kernel

This project was completed as part of a university operating systems course in a team of two.

Building on a minimal UEFI bootloader and kernel framework provided by the course, our team designed and implemented a complete memory management subsystem, including a bitmap-based Physical Memory Manager (PMM), a four-level x86-64 Virtual Memory Manager (VMM), page permission management, and paging initialization. The subsystem was integrated into the existing boot process and successfully validated by booting the kernel in QEMU.

## Features

### Physical Memory Manager (PMM)

* Bitmap-based frame allocator
* 4 KiB physical frame management
* Reserved memory protection
* Frame allocation and deallocation
* Free memory tracking

### Virtual Memory Manager (VMM)

* x86-64 four-level paging
  * PML4
  * PDPT
  * Page Directory
  * Page Table
* Identity mapping for kernel memory
* On-demand page table creation
* Virtual page mapping
* Page permission updates
* Physical address lookup
* TLB invalidation after mapping changes

---

## Scope of the Work

The provided project included:

- a minimal UEFI bootloader
- a basic kernel skeleton
- build infrastructure

Our contribution focused on:

- implementing the Physical Memory Manager (PMM)
- implementing the Virtual Memory Manager (VMM)
- integrating both components into the kernel initialization sequence

---

## Project Structure

Created or modified files:

```text
.
├── Makefile
└── src/
    ├── pmm.c / pmm.h
    ├── paging.c / paging.h
    ├── bootloader.c
    └── kernel.c
```

## Technologies

- C
- x86-64
- UEFI
- QEMU
- Clang / LLD

---

## Known Limitations


* Fixed 128 MiB RAM window: PMM_RAM_SIZE is hardcoded, supporting more physical memory would require additional PDPT/PD entries, since the current paging setup only builds enough tables for that range.
* No interrupt handling: There's no IDT yet, everything (including the serial driver) runs in polling mode.
* Minimal error handling: Failures fall back to panic() / an infinite hlt loop; there's no recovery path.

---

# How to Run

```bash
# Requirements (Ubuntu 24.04)
sudo apt install -y clang lld qemu-system-x86 ovmf parted mtools

# Build and boot in QEMU
make run
```

Output (bootloader start, kernel load, paging setup, PMM tests) prints directly to the terminal via serial. The kernel ends in an infinite `hlt` loop. Exit with `Ctrl+A` then `X`, or `Ctrl+C`.

Other targets: `make all` (build only), `make bootimg` (build + disk image), `make clean`.
