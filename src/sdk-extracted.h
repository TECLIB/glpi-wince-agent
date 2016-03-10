
/*
 * Extracted from DataLogic SDK: DL_DEVICEAPI.H
 */
#define wDATALOGIC_DLL	L"DeviceApi.dll"
#define sDATALOGIC_DLL	"DeviceApi.dll"

#define DATALOGIC_SERIAL_NUMBER_SIZE 17
BOOL WINAPI DeviceGetSerialNumber(TCHAR* szBuf, size_t nStrDim);


/*
 * Extracted from Motorola EMDK for C SDK: Strucinf.h, rcmdef.h & RcmCAPI.h
 */
#define wMOTOROLA_DLL	L"RCMAPI32.dll"
#define sMOTOROLA_DLL	"RCMAPI32.dll"

typedef struct tagSTRUCT_INFO
{
	DWORD	dwAllocated;				// Size of allocated structure in bytes
	DWORD	dwUsed;						// Amount of structure actually used
} STRUCT_INFO;

typedef STRUCT_INFO * LPSTRUCT_INFO;
// Serial Number Structure
typedef struct tagELECTRONIC_SERIAL_NUMBER
{
	STRUCT_INFO StructInfo;				// Information about structure extents

	DWORD	dwAvail;					// Count of unused ESN slots
	DWORD	dwError;					// Error flag
	WCHAR	wszESN[16 + 2];				// Extra padding for null terminator

} ELECTRONIC_SERIAL_NUMBER;

typedef ELECTRONIC_SERIAL_NUMBER FAR * LPELECTRONIC_SERIAL_NUMBER;

// Get Electronic Serial Number
DWORD WINAPI RCM_GetESN(LPELECTRONIC_SERIAL_NUMBER lpESN);	// Pointer to ESN structure to fill

#define VOS_WINDOWSCE					0x00050000L


/*
 * Extracted from SDK file: getdeviceuniqueid.h
 */
#define GETDEVICEUNIQUEID_V1                1
#define GETDEVICEUNIQUEID_V1_MIN_APPDATA    8
#define GETDEVICEUNIQUEID_V1_OUTPUT         20

WINBASEAPI HRESULT WINAPI GetDeviceUniqueID(LPBYTE,DWORD,DWORD,LPBYTE,DWORD);
BOOL KernelIoControl(DWORD dwIoControlCode, LPVOID lpInBuf, DWORD nInBufSize, LPVOID lpOutBuf, DWORD nOutBufSize, LPDWORD lpBytesReturned);

#define IOCTL_HAL_GET_DEVICEID    CTL_CODE(FILE_DEVICE_HAL, 21, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _DEVICE_ID {
	DWORD	dwSize;
	DWORD	dwPresetIDOffset;
	DWORD	dwPresetIDBytes;
	DWORD	dwPlatformIDOffset;
	DWORD	dwPlatformIDBytes;
} DEVICE_ID, *PDEVICE_ID;
