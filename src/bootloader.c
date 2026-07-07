#include "efi.h"

// UEFI bootloader acting as a bridge between UEFI and the kernel.
// It loads kernel.bin from disk into memory and jumps to its entry point.
// The kernel is loaded at a fixed physical address (0x100000).

static void halt(EFI_SYSTEM_TABLE* st, const CHAR16* msg)
{
	st->ConOut->OutputString(st->ConOut, msg);
	for (;;) { __asm __volatile("hlt"); }
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE* systemTable)
{
	EFI_BOOT_SERVICES* bs = systemTable->BootServices;

	systemTable->ConOut->OutputString(systemTable->ConOut, L"UEFI bootloader: start\r\n");

	// Retrieve LoadedImage protocol and the filesystem of the boot device
	EFI_LOADED_IMAGE_PROTOCOL* loaded = 0;
	EFI_STATUS status = bs->HandleProtocol(imageHandle, (void*)&LoadedImageProtocolGuid, (void**)&loaded);
	if (status != EFI_SUCCESS) halt(systemTable, L"HandleProtocol(LoadedImage) failed\r\n");

	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs = 0;
	status = bs->HandleProtocol(loaded->DeviceHandle, (void*)&SimpleFileSystemProtocolGuid, (void**)&fs);
	if (status != EFI_SUCCESS) halt(systemTable, L"HandleProtocol(SimpleFS) failed\r\n");

	EFI_FILE_PROTOCOL* root = 0;
	status = fs->OpenVolume(fs, &root);
	if (status != EFI_SUCCESS) halt(systemTable, L"OpenVolume failed\r\n");

	// Open kernel binary from the EFI partition
	EFI_FILE_PROTOCOL* kf = 0;
	status = root->Open(root, &kf, L"\\kernel.bin", EFI_FILE_MODE_READ, 0);
	if (status != EFI_SUCCESS) halt(systemTable, L"Open \\kernel.bin failed\r\n");

	// Get file size
	UINTN infoSize = 0;
	status = kf->GetInfo(kf, (EFI_GUID*)&FileInfoGuid, &infoSize, 0);
	if (status != EFI_BUFFER_TOO_SMALL || infoSize == 0) halt(systemTable, L"GetInfo(size) failed\r\n");

	EFI_FILE_INFO* info = 0;
	status = bs->AllocatePool(EfiLoaderData, infoSize, (void**)&info);
	if (status != EFI_SUCCESS) halt(systemTable, L"AllocatePool(fileinfo) failed\r\n");

	status = kf->GetInfo(kf, (EFI_GUID*)&FileInfoGuid, &infoSize, info);
	if (status != EFI_SUCCESS) halt(systemTable, L"GetInfo(data) failed\r\n");

	UINTN kernelSize = (UINTN)info->FileSize;

	// Allocate kernel at fixed address 0x100000
	EFI_PHYSICAL_ADDRESS kernelAddr = 0x100000;
	UINTN pages = (kernelSize + 0xFFF) / 0x1000;

	status = bs->AllocatePages(AllocateAddress, EfiLoaderData, pages, &kernelAddr);
	if (status != EFI_SUCCESS) halt(systemTable, L"AllocatePages failed\r\n");

	// Load kernel into memory
	UINTN toRead = kernelSize;
	status = kf->Read(kf, &toRead, (void*)(UINTN)kernelAddr);
	if (status != EFI_SUCCESS || toRead != kernelSize) halt(systemTable, L"Read(kernel) failed\r\n");

	kf->Close(kf);

	systemTable->ConOut->OutputString(systemTable->ConOut, L"Kernel loaded, jumping...\r\n");

	// Jump to kernel entry point
	void (*kernel_entry)(void) = (void(*)(void))(UINTN)kernelAddr;
	kernel_entry();

	halt(systemTable, L"Returned from kernel (unexpected)\r\n");
	return EFI_SUCCESS;
}