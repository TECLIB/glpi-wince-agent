
/*
 * Defines the entry point for the DLL application.
 */

#include <windows.h>

DECLSPEC_EXPORT BOOL GWA_Close(
	DWORD dwData
);

DECLSPEC_EXPORT BOOL GWA_Deinit(
	DWORD dwData
);

DECLSPEC_EXPORT DWORD GWA_Init(
	DWORD dwData
);

DECLSPEC_EXPORT BOOL GWA_IOControl(
	DWORD dwData,
	DWORD dwCode,
	PBYTE pBufIn,
	DWORD dwLenIn,
	PBYTE pBufOut,
	DWORD dwLenOut,
	PDWORD pdwActualOut
);

DECLSPEC_EXPORT BOOL GWA_Open(
	DWORD dwData,
	DWORD dwAccess,
	DWORD dwShareMode
);

DECLSPEC_EXPORT DWORD GWA_Read(
	DWORD dwData,
	LPVOID pBuf,
	DWORD dwLen
);

/*
DECLSPEC_EXPORT DWORD GWA_Seek(
	DWORD dwData,
	long pos,
	DWORD type
);
*/

DECLSPEC_EXPORT DWORD GWA_Write(
	DWORD dwData,
	LPVOID pInBuf,
	DWORD dwInLen
);

DECLSPEC_EXPORT APIENTRY BOOL DllMain(
	HANDLE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
);
