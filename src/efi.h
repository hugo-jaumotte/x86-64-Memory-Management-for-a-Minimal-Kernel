#ifndef EFI_H
#define EFI_H

#include <stdint.h>

typedef int16_t INT16;
typedef uint8_t  UINT8;
typedef uint16_t UINT16;
typedef uint32_t UINT32;
typedef uint64_t UINT64;
typedef UINT64   UINTN;  // Native machine word size (64-bit here)

typedef uint64_t EFI_PHYSICAL_ADDRESS;
typedef uint64_t EFI_VIRTUAL_ADDRESS;

typedef uint64_t EFI_STATUS;
typedef void* EFI_HANDLE;
typedef uint16_t CHAR16; // prefix L = wchar_t instead of char

typedef struct {
	UINT16 Year;        // 1900 – 9999
	UINT8  Month;       // 1 – 12
	UINT8  Day;         // 1 – 31
	UINT8  Hour;        // 0 – 23
	UINT8  Minute;      // 0 – 59
	UINT8  Second;      // 0 – 59
	UINT8  Pad1;
	UINT32 Nanosecond;  // 0 – 999,999,999
	INT16  TimeZone;    // minutes offset from UTC (-1440 to 1440)
	UINT8  Daylight;    // daylight savings flags
	UINT8  Pad2;
} EFI_TIME;

#define EFIERR(a)  (0x8000000000000000 | (a))
#define EFI_SUCCESS 0
#define EFI_LOAD_ERROR            (EFIERR(1))
#define EFI_INVALID_PARAMETER     (EFIERR(2))
#define EFI_UNSUPPORTED           (EFIERR(3))
#define EFI_BAD_BUFFER_SIZE       (EFIERR(4))
#define EFI_BUFFER_TOO_SMALL      (EFIERR(5))
#define EFI_NOT_READY             (EFIERR(6))
#define EFI_DEVICE_ERROR          (EFIERR(7))
#define EFI_OUT_OF_RESOURCES      (EFIERR(9))

#define EFIAPI __attribute__((ms_abi))


typedef struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
	EFI_STATUS (EFIAPI *Reset)(struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* This, int ExtendedVerification);
	EFI_STATUS (EFIAPI *OutputString)(struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* This, const CHAR16* String);
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef struct EFI_TABLE_HEADER{
	uint64_t Signature;
	uint32_t Revision;
	uint32_t HeaderSize;
	uint32_t CRC32;
	uint32_t Reserved;
} EFI_TABLE_HEADER;


//
// GUID
//
typedef struct EFI_GUID{
	uint32_t Data1;
	uint16_t Data2;
	uint16_t Data3;
	uint8_t  Data4[8];
} EFI_GUID;
//
// File modes
//
#define EFI_FILE_MODE_READ   0x0000000000000001ULL
#define EFI_FILE_MODE_WRITE  0x0000000000000002ULL
#define EFI_FILE_MODE_CREATE 0x8000000000000000ULL
#define EfiLoaderData 2

#define AllocateAnyPages    0
#define AllocateMaxAddress  1
#define AllocateAddress     2

// Protocol GUIDs
static const EFI_GUID LoadedImageProtocolGuid =
{0x5B1B31A1,0x9562,0x11d2,{0x8E,0x3F,0x00,0xA0,0xC9,0x69,0x72,0x3B}};

static const EFI_GUID SimpleFileSystemProtocolGuid =
{0x964E5B22,0x6459,0x11d2,{0x8E,0x39,0x00,0xA0,0xC9,0x69,0x72,0x3B}};

static const EFI_GUID FileInfoGuid =
{0x09576E92,0x6D3F,0x11d2,{0x8E,0x39,0x00,0xA0,0xC9,0x69,0x72,0x3B}};

typedef struct {
	UINT32 Type;
	void*  PhysicalStart;
	void*  VirtualStart;
	UINT64 NumberOfPages;
	UINT64 Attribute;
} EFI_MEMORY_DESCRIPTOR;

typedef struct EFI_FILE_PROTOCOL {
	UINT64                     Revision;
	/*EFI_FILE_OPEN              Open;*/
	EFI_STATUS (EFIAPI *Open)(
		struct EFI_FILE_PROTOCOL* This,
		struct EFI_FILE_PROTOCOL** NewHandle,
		const CHAR16* FileName,
		uint64_t OpenMode,
		uint64_t Attributes
	);
	/*EFI_FILE_CLOSE             Close;*/
	EFI_STATUS (EFIAPI *Close)(struct EFI_FILE_PROTOCOL* This);
	void*            Delete;
	/* EFI_FILE_READ              Read;*/
	EFI_STATUS (EFIAPI *Read)(
		struct EFI_FILE_PROTOCOL* This,
		UINTN* BufferSize,
		void* Buffer
	);
	void*             Write;
	void*      GetPosition;
	void*      SetPosition;
	/*EFI_FILE_GET_INFO          GetInfo*/
	EFI_STATUS (EFIAPI *GetInfo)(
		struct EFI_FILE_PROTOCOL* This,
		EFI_GUID* InformationType,
		UINTN* BufferSize,
		void* Buffer
	);
	void*                      SetInfo;
	void*                      Flush;
	void*                      OpenEx;    // placeholders for the *Ex variants
	void*                      ReadEx;
	void*                      WriteEx;
	void*                      FlushEx;
} EFI_FILE_PROTOCOL;

typedef struct EFI_GRAPHICS_OUTPUT_MODE_INFORMATION{
	UINT32 Version;
	UINT32 HorizontalResolution;
	UINT32 VerticalResolution;
	UINT32 PixelFormat;
	UINT32 PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

typedef struct EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE{
	UINT32 MaxMode;
	UINT32 Mode;
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* Info;
	UINTN SizeOfInfo;
	EFI_PHYSICAL_ADDRESS FrameBufferBase;
	UINTN FrameBufferSize;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;

typedef struct EFI_GRAPHICS_OUTPUT_PROTOCOL {
	void* QueryMode;
	void* SetMode;
	void* Blt;
	EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE* Mode;
} EFI_GRAPHICS_OUTPUT_PROTOCOL;

typedef struct {
	uint64_t Size;
	uint64_t FileSize;
	uint64_t PhysicalSize;
	EFI_TIME CreateTime;
	EFI_TIME LastAccessTime;
	EFI_TIME ModificationTime;
	uint64_t Attribute;
	CHAR16   FileName[1]; // flexible array
} EFI_FILE_INFO;

typedef struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL {
	UINT64 Revision;
	EFI_STATUS (EFIAPI *OpenVolume)(
		struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* This,
		EFI_FILE_PROTOCOL** Root
	);
} EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

#define AllocateAnyPages    0
#define AllocateMaxAddress  1
#define AllocateAddress     2

typedef struct {
	EFI_TABLE_HEADER Hdr;

	// TPL services
	void* RaiseTPL;
	void* RestoreTPL;

	// Memory services
	/*void* AllocatePages;*/
	EFI_STATUS (EFIAPI *AllocatePages)(
		int Type,                        // AllocateAnyPages, AllocateMaxAddress, AllocateAddress
		int MemoryType,                  // EfiLoaderData, EfiBootServicesData, etc.
		UINTN Pages,                     // number of 4 KiB pages
		EFI_PHYSICAL_ADDRESS* Memory     // physical address where the result is stored
	);
	void* FreePages;
	/*void* GetMemoryMap;*/
	EFI_STATUS (EFIAPI *GetMemoryMap)(
		UINTN* MemoryMapSize,
		void* MemoryMap,
		UINTN* MapKey,
		UINTN* DescriptorSize,
		uint32_t* DescriptorVersion
	);
	/*void* AllocatePool;*/
	EFI_STATUS (EFIAPI *AllocatePool)(
		int PoolType,
		UINTN Size,
		void** Buffer
	);
	void* FreePool;

	// Event & timer services
	void* CreateEvent;
	void* SetTimer;
	void* WaitForEvent;
	void* SignalEvent;
	void* CloseEvent;
	void* CheckEvent;

	// Protocol handler services
	void* InstallProtocolInterface;
	void* ReinstallProtocolInterface;
	void* UninstallProtocolInterface;
	/*void* HandleProtocol;*/
	EFI_STATUS (EFIAPI *HandleProtocol)(
		EFI_HANDLE Handle,
		void* Protocol,
		void** Interface
	);
	void* Reserved;
	void* RegisterProtocolNotify;
	void* LocateHandle;
	void* LocateDevicePath;
	void* InstallConfigurationTable;

	// Image services
	void* LoadImage;
	void* StartImage;
	void* Exit;
	void* UnloadImage;
	/*void* ExitBootServices;*/
	EFI_STATUS (EFIAPI *ExitBootServices)(
		EFI_HANDLE ImageHandle,
		UINTN MapKey
	);

	// Misc services
	void* GetNextMonotonicCount;
	void* Stall;
	void* SetWatchdogTimer;

	// Driver support services
	void* ConnectController;
	void* DisconnectController;

	// Open & close protocol services
	void* OpenProtocol;
	void* CloseProtocol;
	void* OpenProtocolInformation;

	// Library services
	void* ProtocolsPerHandle;
	void* LocateHandleBuffer;
	/*void* LocateProtocol;*/
	EFI_STATUS (EFIAPI *LocateProtocol)(
		EFI_GUID *Protocol,
		void *Registration,
		void **Interface
	);

	void* InstallMultipleProtocolInterfaces;
	void* UninstallMultipleProtocolInterfaces;

	// 32-bit CRC services
	void* CalculateCrc32;

	// Misc services (UEFI 2.0+)
	void* CopyMem;
	void* SetMem;
	void* CreateEventEx;
} EFI_BOOT_SERVICES;

typedef struct EFI_SYSTEM_TABLE{
	EFI_TABLE_HEADER                Hdr;              // 24 bytes
	CHAR16                         *FirmwareVendor;   // 8 bytes
	uint32_t                        FirmwareRevision; // 4 bytes
	uint32_t                        _pad;             // alignment padding
	EFI_HANDLE                      ConsoleInHandle;  // 8 bytes
	void                           *ConIn;            // 8
	EFI_HANDLE                      ConsoleOutHandle; // 8
	EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;   // 8
	EFI_HANDLE StdErrHandle;
	EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* StdErr;
	void* RuntimeServices;
	EFI_BOOT_SERVICES* BootServices;
} EFI_SYSTEM_TABLE;

typedef struct {
	UINT32  Revision;
	EFI_HANDLE ParentHandle;
	EFI_SYSTEM_TABLE* SystemTable;

	EFI_HANDLE DeviceHandle;
	void*     FilePath;
	void*     Reserved;

	UINT32    LoadOptionsSize;
	void*     LoadOptions;

	void*     ImageBase;
	UINT64    ImageSize;
	UINT32    ImageCodeType;
	UINT32    ImageDataType;
	void*     Unload;
} EFI_LOADED_IMAGE_PROTOCOL;


#endif