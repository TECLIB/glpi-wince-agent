
/*
 * Extracted from DataLogic SDK: deviceApi.h
 */
#define wDATALOGIC_DLL	L"DeviceApi.dll"
#define sDATALOGIC_DLL	"DeviceApi.dll"

#define VERSION_LABEL_SIZE		24		// Maximum version label length (including NULL)
#define VERSION_SIZE			5		// Short version string: X.XX + NULL terminator
#define SERIAL_NUMBER_SIZE		17		// Serial number plus NULL terminator

#define VERSION_INDEX_FIRMWARE	0		// Index for firmware version

BOOL WINAPI DeviceGetVersionInfo(DWORD index, WCHAR *label, int labelLen, WCHAR *version, int versionLen);
BOOL WINAPI DeviceGetSerialNumber(TCHAR* szBuf, size_t nStrDim);


/*
 * Extracted from Motorola EMDK for C SDK: Strucinf.h, rcmdef.h & RcmCAPI.h
 */
#define wMOTOROLA_DLL	L"RCMAPI32.dll"
#define sMOTOROLA_DLL	"RCMAPI32.dll"

#define E_RCM_SUCCESS	0x0

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

#define SI_ALLOC_ALL(ptr)  { (ptr)->StructInfo.dwAllocated = sizeof(*(ptr)); }
#define SI_USED_NONE(ptr)  { (ptr)->StructInfo.dwUsed = sizeof(STRUCT_INFO); }
#define SI_INIT(ptr)       { SI_ALLOC_ALL(ptr); SI_USED_NONE(ptr); }

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

/**
 * Missing from wce500 SDK
 */
#define SERVICE_SUCCESS				0
//
// Service startup state (value passed on xxx_Init())
//
#define SERVICE_INIT_STARTED	0x00000000
// Service is a super-service, should not spin its own accept() threads.
#define SERVICE_INIT_STOPPED	0x00000001
// Service is being run in an isolated services.exe.  Interprocess communication
// via IOCTLs or streaming interface is not supported in this configuration.
#define SERVICE_INIT_STANDALONE 0x00000002

HANDLE GetServiceHandle(LPWSTR szPrefix, LPWSTR szDllName, DWORD *pdwDllBuf);
void GetCurrentFT(LPFILETIME lpFileTime);
DWORD Random();

//
//	Service is interfaced via series of IOCTL calls that define service life cycle.
//	Actual implementation is service-specific.
//

//
//	Start the service that has been in inactive state. Return code: SERVICE_SUCCESS or error code.
//
#define IOCTL_SERVICE_START		CTL_CODE(FILE_DEVICE_SERVICE, 1, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
//	Stop service, but do not unload service's DLL
//
#define IOCTL_SERVICE_STOP		CTL_CODE(FILE_DEVICE_SERVICE, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
//	Refresh service's state from registry or other configuration storage
//
#define IOCTL_SERVICE_REFRESH	CTL_CODE(FILE_DEVICE_SERVICE, 3, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
//	Have service configure its registry for auto-load
//
#define IOCTL_SERVICE_INSTALL	CTL_CODE(FILE_DEVICE_SERVICE, 4, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
//	Remove registry configuration
//
#define IOCTL_SERVICE_UNINSTALL	CTL_CODE(FILE_DEVICE_SERVICE, 5, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
//	Unload the service which should be stopped.
//
#define IOCTL_SERVICE_UNLOAD	CTL_CODE(FILE_DEVICE_SERVICE, 6, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
//	Supply a configuration or command string and code to the service.
//
#define IOCTL_SERVICE_CONTROL	CTL_CODE(FILE_DEVICE_SERVICE, 7, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
//	Return service status.
//
#define IOCTL_SERVICE_STATUS	CTL_CODE(FILE_DEVICE_SERVICE, 8, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
//	Set service's debug zone mask (parameter is sizeof(DWORD) and contains the mask)
//
#define IOCTL_SERVICE_DEBUG	CTL_CODE(FILE_DEVICE_SERVICE, 9, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
//	Toggle service's console on/off (string "on" on no parameters is ON, "off" means off)
//
#define IOCTL_SERVICE_CONSOLE	CTL_CODE(FILE_DEVICE_SERVICE, 10, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// Notify service a service request has arrived.  Input contains connected socket.
//
#define IOCTL_SERVICE_REGISTER_SOCKADDR   CTL_CODE(FILE_DEVICE_SERVICE, 11, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// Notify service a service request has arrived.  For now all sockets associated with service will be closed at once.
//
#define IOCTL_SERVICE_DEREGISTER_SOCKADDR   CTL_CODE(FILE_DEVICE_SERVICE, 12, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// Notify service a another socket has bound to it.  Input contains an accepted socket.
//
#define IOCTL_SERVICE_CONNECTION   CTL_CODE(FILE_DEVICE_SERVICE, 13, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// Service has finished initial control setup.  Ready to start.
//
#define IOCTL_SERVICE_STARTED      CTL_CODE(FILE_DEVICE_SERVICE, 14, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// Service is called with IOCTL_SERVICE_CAN_DEINIT immediatly before xxx_Deinit is called during DeregisterService.
// If xxx_IOControl returns TRUE and sets buffer in pBufOut to zero, service instance will remain loaded and
// xxx_Deinit will not be called.
//
#define IOCTL_SERVICE_QUERY_CAN_DEINIT   CTL_CODE(FILE_DEVICE_SERVICE, 15, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// If iphlpapi is present, services receive this notification when table that
// maps IP addresses changes.
// Input contains PIP_ADAPTER_ADDRESSES retrieved from a call to GetAdaptersAddresses().
// As soon as the service returns from this call, this buffer is not valid.  The buffer
// must not be pointed to and it MUST NOT BE COPIED (because internally it has pointers
// to other structures inside it).  If this data is required after the service returns,
// the service must make its own call to GetAdaptersAddresses().
#define IOCTL_SERVICE_NOTIFY_ADDR_CHANGE  CTL_CODE(FILE_DEVICE_SERVICE, 16, METHOD_BUFFERED, FILE_ANY_ACCESS)


//
// Services.exe supports a set of callbacks that can only be called from a running service (not API).
// pBufIn data structure contains a pointer to ServiceCallbackFunctions data structure
// IOCTL_SERVICE_CALLBACKS will sent to service during its initial load, and only if there
// are supported callbacks for service's mode of operation.
//
#define IOCTL_SERVICE_CALLBACK_FUNCTIONS  CTL_CODE(FILE_DEVICE_SERVICE, 17, METHOD_BUFFERED, FILE_ANY_ACCESS)
