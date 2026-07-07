#include <stdint.h>
#include "paging.h"
#include "pmm.h"

// --- COM1 serial I/O (visible in QEMU via -serial mon:stdio) ---

// Write a byte to an x86 I/O port.
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

// Read a byte from an x86 I/O port.
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Minimal 16550 UART setup for COM1 (0x3F8), polling mode, 38400 8N1.
static void serial_init(void) {
    outb(0x3F8 + 1, 0x00); // disable interrupts
    outb(0x3F8 + 3, 0x80); // enable DLAB
    outb(0x3F8 + 0, 0x03); // divisor low  (38400 baud)
    outb(0x3F8 + 1, 0x00); // divisor high
    outb(0x3F8 + 3, 0x03); // 8 bits, no parity, one stop bit
    outb(0x3F8 + 2, 0xC7); // enable FIFO
    outb(0x3F8 + 4, 0x0B); // IRQs enabled, RTS/DSR set
}

// Send one character over COM1 (blocking until UART is ready).
static void serial_putc(char c) {
    while ((inb(0x3F8 + 5) & 0x20) == 0) { }
    outb(0x3F8, (uint8_t)c);
}

// Print a null-terminated string over serial.
void serial_puts(const char* s) {
    while (*s) serial_putc(*s++);
}

// Print a 64-bit value in hex (0x + 16 digits).
void serial_hex64(uint64_t x) {
    const char* h = "0123456789ABCDEF";
    serial_puts("0x");
    for (int i = 15; i >= 0; i--) {
        serial_putc(h[(x >> (i*4)) & 0xF]);
    }
}

// Kernel stack (kernel.ld maps .stack at 0x200000).
__attribute__((aligned(16), section(".stack")))
static uint8_t g_stack[16 * 1024];

// Halt the kernel on unrecoverable error.
static void panic(void) {
    serial_puts("[PANIC]\n");
    for(;;) __asm__ __volatile__("cli; hlt");
}


__attribute__((noreturn)) void kernel_main(void);


__attribute__((noreturn))
void kboot(void) {
    serial_init();        // so paging.c can log
    pmm_init_simple();    // required before alloc_table_phys()
    init_paging_and_jump(kernel_main);
    __builtin_unreachable();
}


// Kernel entry point (ENTRY(_start) in kernel.ld), called right after the bootloader.
// naked: no compiler-generated prologue, since the stack isn't set up yet.
__attribute__((noreturn, naked, section(".text.start")))
void _start(void) {
    __asm__ volatile(
        "cli\n\t"
        "mov %[stack_top], %%rsp\n\t"
        "jmp kboot\n\t"
        :
        : [stack_top] "r"(g_stack + sizeof(g_stack))
        : "memory"
    );
}


__attribute__((noreturn))
void kernel_main(void)
{
    serial_puts("[KERNEL] booted\n");

    uint64_t a = pmm_alloc_frame();
    uint64_t b = pmm_alloc_frame();
    serial_puts("[PMM] a="); serial_hex64(a); serial_puts(" b="); serial_hex64(b); serial_puts("\n");

    if (a < 0x300000 || b < 0x300000) panic();

    uint64_t before = pmm_count_free();
    pmm_free_frame(a);
    uint64_t after = pmm_count_free();

    serial_puts("[PMM] before="); serial_hex64(before);
    serial_puts(" after="); serial_hex64(after);
    serial_puts("\n");

    if (after != before + 1) panic();

    serial_puts("[OK] PMM tests passed\n");
    for(;;) __asm__ __volatile__("hlt");

}