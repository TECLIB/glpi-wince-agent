
/*
 * Extracted from DataLogic SDK: DL_DEVICEAPI.H
 */
#define wDATALOGIC_DLL	L"DLCEDevice.dll"
#define sDATALOGIC_DLL	"DLCEDevice.dll"

#ifndef DLDEVICE_API
#define DLDEVICE_API __declspec(dllimport)
#endif

DLDEVICE_API size_t DLDEVICE_GetSerialNumber(TCHAR* szBuf, size_t nStrDim);


/*
 * Extracted from Motorola EMDK for C SDK: Strucinf.h, rcmdef.h & RcmCAPI.h
 */
#define wMOTOROLA_DLL	L"RcmApi32.dll"
#define sMOTOROLA_DLL	"RcmApi32.dll"

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
