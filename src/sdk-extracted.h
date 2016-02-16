
/*
 * Extracted from DataLogic SDK: DL_DEVICEAPI.H
 */
#ifndef DLDEVICE_API
#define DLDEVICE_API __declspec(dllimport)
#endif

DLDEVICE_API size_t DLDEVICE_GetSerialNumber(TCHAR* szBuf, size_t nStrDim);
//DLDEVICE_API size_t DLDEVICE_GetName(TCHAR* szBuf, size_t nCapacity);
//DLDEVICE_API size_t DLDEVICE_GetWinCEVersion(TCHAR* szBuf, size_t nCapacity);

/*
 * Extracted from SDK file: getdeviceuniqueid.h
 */
#define GETDEVICEUNIQUEID_V1                1
#define GETDEVICEUNIQUEID_V1_MIN_APPDATA    8
#define GETDEVICEUNIQUEID_V1_OUTPUT         20

WINBASEAPI HRESULT WINAPI GetDeviceUniqueID(LPBYTE,DWORD,DWORD,LPBYTE,DWORD);
