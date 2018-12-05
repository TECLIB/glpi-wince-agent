
/*
 * Defines the entry point for the DLL application.
 */

#include <windows.h>
#include <shlobj.h>
#include <service.h>
#include <winioctl.h>

#include "glpi-wince-agent.h"

/*
 * cegcc doesn't provide ce_setup.h; the following enum values are
 * taken from MSDN
 */
typedef enum {
	  codeINSTALL_INIT_CONTINUE = 0,
	  codeINSTALL_INIT_CANCEL
} codeINSTALL_INIT;

typedef enum {
	  codeINSTALL_EXIT_DONE = 0,
	  codeINSTALL_EXIT_UNINSTALL
} codeINSTALL_EXIT;

typedef enum {
	  codeUNINSTALL_INIT_CONTINUE = 0,
	  codeUNINSTALL_INIT_CANCEL
} codeUNINSTALL_INIT;

typedef enum {
	codeUNINSTALL_EXIT_DONE = 0
} codeUNINSTALL_EXIT;

DECLSPEC_EXPORT codeINSTALL_INIT
Install_Init(HWND hwndparent, BOOL ffirstcall, BOOL fpreviouslyinstalled,
             LPCTSTR pszinstalldir);

DECLSPEC_EXPORT codeINSTALL_EXIT
Install_Exit(HWND hwndparent, LPCTSTR pszinstalldir,
             WORD cfaileddirs, WORD cfailedfiles, WORD cfailedregkeys,
             WORD cfailedregvals, WORD cfailedshortcuts);

DECLSPEC_EXPORT codeUNINSTALL_INIT
Uninstall_Init(HWND hwndparent, LPCTSTR pszinstalldir);

DECLSPEC_EXPORT codeUNINSTALL_EXIT
Uninstall_Exit(HWND hwndparent);

DECLSPEC_EXPORT APIENTRY BOOL
DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved);

FILE *hFile = NULL;
LPCSTR cstrInstallJournal = APPNAME "-install.txt";
LPCSTR DefaultConfigFile  = APPNAME "-config.txt";
LPTSTR wInstalldir = NULL;
LPTSTR wVardir = NULL;
DWORD keepFiles = 0;

#define SHORT_BUFFER_SIZE 16

// We need to adapt installation against WinCE Version
OSVERSIONINFO os = {
	sizeof(OSVERSIONINFO),
	5, 0, 0, 0, L"\0"
};

/**
 * Logging functions
 */
void Log(LPCSTR format, ...)
{
	SYSTEMTIME ts ;

	GetLocalTime(&ts);
	printf("%4d-%02d-%02d %02d:%02d:%02d: ",
		ts.wYear, ts.wMonth, ts.wDay, ts.wHour, ts.wMinute, ts.wSecond);

	va_list args;
	va_start(args, format);
	vprintf(format, args);
#ifdef TEST
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
#endif
	va_end(args);
	printf("\n");
}

void DumpError(void)
{
	int buflen = 0 ;
	LPSTR lpErrorBuf = NULL;
	LPTSTR lpMsgBuf = NULL;

	if (GetLastError() != NO_ERROR)
	{
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			0, // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL
		);

		if (lpMsgBuf != NULL)
		{
			buflen = wcslen(lpMsgBuf)+1;
			lpErrorBuf = malloc(buflen);
			wcstombs(lpErrorBuf, lpMsgBuf, buflen);
			Log("Error(0x%lx): %s", GetLastError(), lpErrorBuf);
			free(lpErrorBuf);
			LocalFree( lpMsgBuf );
		}
		else
		{
			Log("Error(0x%lx): Can't read error message", GetLastError());
		}
	}
}

void StopAndDisableService(LPCSTR version)
{
	HANDLE hService = NULL;
	DWORD dwDllBuf = 0, dwRet = 0;

	Log("Looking to stop and disable service...");

	if (os.dwMajorVersion < 6)
	{
		hService = GetServiceHandle(L"GWA0:", NULL, &dwDllBuf);
	} else {
		hService = CreateFile(L"GWA0:",GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
	}
	if (hService != INVALID_HANDLE_VALUE)
	{
		if (os.dwMajorVersion < 6)
		{
			if (DeviceIoControl(hService, IOCTL_SERVICE_STOP, NULL, 0, NULL, 0, &dwRet, NULL))
			{
				Log("Service stop requested");
			}
			else
			{
				Log("Failed to transmit stop request");
			}
		}

		if (DeregisterService(hService))
		{
			Log(APPNAME " v%s service disabled", version);
		}
		else
		{
			Log("Failed to stop " APPNAME " v%s service", version);
			DumpError();
		}
		CloseHandle(hService);
	}
	else if (GetLastError() == ERROR_DEV_NOT_EXIST)
	{
		Log(APPNAME " v%s service still disabled", version);
		SetLastError(NO_ERROR);
	}
	else if (GetLastError() == ERROR_NOT_FOUND)
	{
		Log(APPNAME " v%s service was not installed", version);
		SetLastError(NO_ERROR);
	}
	else
	{
		Log("Failed to access " APPNAME " v%s", version);
		DumpError();
	}
}

#ifdef TEST
#define BUFSIZE 512
void DumpRegKey(HKEY hKey, LPWSTR wKey)
{
	DWORD dwBufSize = BUFSIZE, dwType = 0, dwIndex = 0, dwData = BUFSIZE;
	LPWSTR wValueName = malloc(BUFSIZE);
	LPBYTE lpData = malloc(BUFSIZE);

	// Dump values
	while (RegEnumValue(hKey, dwIndex++, wValueName, &dwBufSize,
	                    NULL, &dwType, lpData, &dwData) == ERROR_SUCCESS)
	{
		LPSTR lpValueName = NULL, lpValue = NULL;
		int length = wcslen(wKey)+wcslen(wValueName)+2;
		lpValueName = malloc(length);
		wcstombs(lpValueName, wKey, length);
		strcat(lpValueName,"\\");
		wcstombs(lpValueName+wcslen(wKey)+1, wValueName, length);
		switch (dwType)
		{
			case REG_DWORD:
				Log(
					*((LPDWORD)lpData)>256 ?
						"Found Value: %s=0x%lx": "Found Value: %s=%ld",
						lpValueName, *((LPDWORD)lpData));
				break;
			case REG_SZ:
				lpValue = malloc(dwData+1);
				wcstombs(lpValue, (LPWSTR)lpData, dwData+1);
				Log("Found Value: %s=\"%s\"", lpValueName, lpValue);
				free(lpValue);
				break;
			case REG_BINARY:
				Log("Found Value: %s=(REG_BINARY)", lpValueName);
				break;
			default:
				Log("Found Value: %s=(REG? 0x%x)", lpValueName, dwType);
				break;
		}
		free(lpValueName);
		dwBufSize = dwData = BUFSIZE;
	}
	free(lpData);

	// Dump subkeys
	dwIndex = 0;
	while (RegEnumKeyEx(hKey, dwIndex++, wValueName, &dwBufSize,
	                    NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
	{
		HKEY hSubKey = NULL;
		LPSTR lpValueName = NULL;
		int length = wcslen(wKey)+wcslen(wValueName)+2;
		lpValueName = malloc(length);
		wcstombs(lpValueName, wKey, length);
		strcat(lpValueName,"\\");
		wcstombs(lpValueName+wcslen(wKey)+1, wValueName, length);
		wsprintf(wValueName, L"%hs", lpValueName);
		if (OpenedKey(HKEY_LOCAL_MACHINE, wValueName, &hSubKey))
		{
			Log("Dumping subkey: %s\\", lpValueName);
			DumpRegKey(hSubKey, wValueName);
			RegCloseKey(hSubKey);
			Log("Dumped subkey: %s\\", lpValueName);
		}
		else
		{
			Log("Can't dump found subkey: %s\\", lpValueName);
		}
		free(lpValueName);
	}
	free(wValueName);
}

void DebugRegistry(void)
{
	HKEY hKey = NULL;

	// Dump Services keys
	if (OpenedKey(HKEY_LOCAL_MACHINE, L"\\Services", &hKey))
	{
		Log("Dumping HKEY_LOCAL_MACHINE\\Services...");
		DumpRegKey(hKey, L"\\Services");
		RegCloseKey(hKey);
		Log("Dump finished");
	}
	// Dump Softwares keys
	if (OpenedKey(HKEY_LOCAL_MACHINE, L"\\Software", &hKey))
	{
		Log("Dumping HKEY_LOCAL_MACHINE\\Software...");
		DumpRegKey(hKey, L"\\Software");
		RegCloseKey(hKey);
		Log("Dump finished");
	}
}
#endif

/**
 * Handles tasks done at start of installation
 * Also called first while updating or re-installing
 */
codeINSTALL_INIT
Install_Init(HWND hwndparent, BOOL bFirstcall, BOOL IsInstalled,
             LPCTSTR pszinstalldir)
{
	HKEY hKey = NULL;
	LPSTR logpath = NULL;

	logpath = malloc(MAX_PATH+wcslen(cstrInstallJournal)+1);
	wcstombs(logpath, pszinstalldir, MAX_PATH);
	strcat(logpath, "\\");
	strcat(logpath, cstrInstallJournal);

	// Initializes OS informations
	os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (!GetVersionEx( &os ))
	{
		Log("Failed to get system version");
		// Must not happen, but just default to wince 5 while failing
		os.dwMajorVersion = 5;
	}

	if (bFirstcall)
	{
		// Truncate installation log from here
		if (hFile != NULL)
		{
			fclose(hFile);
			hFile = freopen( logpath, "w", stdout );
		}

#ifdef TEST
		Log("Dumping registry before service installation...");
		DebugRegistry();
#endif
	}

	Log("Current OS %d.%d (build %d)", os.dwMajorVersion, os.dwMinorVersion, os.dwBuildNumber);

	if (IsInstalled)
	{
		keepFiles = 1;

		if (OpenedKey(HKEY_LOCAL_MACHINE, L"\\Software\\"EDITOR"\\"APPNAME, &hKey))
		{
			DWORD dwType = REG_SZ, dwDataSize = 2*SHORT_BUFFER_SIZE;
			LPTSTR Version = malloc(dwDataSize);

			SetLastError(NO_ERROR);

			if (RegQueryValueEx(hKey, L"Version", NULL, &dwType,
			                    (LPBYTE)Version, &dwDataSize) == ERROR_SUCCESS)
			{
				LPSTR lpVersion = NULL;
				int length = wcslen(Version)+1;
				lpVersion = malloc(length);
				wcstombs(lpVersion, Version, length);
				Log("Updating from " APPNAME " v%s...", lpVersion);

				// Still try to disable the service
				StopAndDisableService(lpVersion);

				free(lpVersion);

				// Update KeepFiles to 1 in registry
				dwType = REG_DWORD ;
				dwDataSize = sizeof(keepFiles);
				if (RegSetValueEx(hKey, L"KeepFiles", 0, dwType,
				                  (LPBYTE)&keepFiles, dwDataSize) == ERROR_SUCCESS)
				{
					Log("Files keeping requested");
				}
				else
				{
					Log("Unable to request files keeping");
					DumpError();
				}
			}
			else
			{
				Log("Failed to check previous " APPNAME " version");
				DumpError();
			}
			RegCloseKey(hKey);
			free(Version);
		}
		Log("Updating to " APPNAME " v" VERSION "...");
	}
	else
	{
		Log("Installing " APPNAME " v" VERSION "...");
	}

	free(logpath);

	return codeINSTALL_INIT_CONTINUE;
}

/**
 * Handles tasks done at end of installation
 * Also finally loaded while updating or re-installing
 */
codeINSTALL_EXIT
Install_Exit(HWND hwndparent, LPCTSTR pszinstalldir, WORD cfaileddirs,
             WORD cfailedfiles, WORD cfailedregkeys, WORD cfailedregvals,
             WORD cfailedshortcuts)
{
	LPTSTR wPath = NULL;
	LPSTR path = NULL;
	HANDLE hService = NULL;
	HKEY hKey = NULL, hSubKey = NULL;

	path  = malloc(MAX_PATH);
	wPath = malloc(2*MAX_PATH);

	// Check if install path is the default
	wcstombs(path, pszinstalldir, MAX_PATH);
	Log("Installing " APPNAME " in '%s'...", path);
	if (lpstrcmp(path, DEFAULTINSTALLPATH))
	{
		Log("Installing " APPNAME " in system default path...");
		// Install path is the default in the device, we can still define
		// the storage path to \Application Data\APPNAME
		if (SHGetSpecialFolderPath( NULL, wPath, CSIDL_APPDATA, 0))
		{
			wcstombs(path, wPath, MAX_PATH);
			strcat( path, "\\" APPNAME );
		} else {
			Log("Unable to get system default application path");
			DumpError();
			strcat( path, "\\" DEFAULTVARDIR );
		}
	}
	else
	{
		strcat( path, "\\" DEFAULTVARDIR );
	}

	// Guaranty folder exists
	swprintf( wPath, L"%hs", path );
	if (strlen(path))
	{
		if (CreateDirectory( wPath, NULL ))
		{
			Log("Folder %s created", path);
		}
		else if
			(
				GetLastError() == ERROR_FILE_EXISTS ||
				GetLastError() == ERROR_ALREADY_EXISTS
			)
		{
			Log("Folder %s still exists", path);
			SetLastError(NO_ERROR);
		}
		else
		{
			Log("Can't create %s folder", path);
			DumpError();
		}
	}

	Log("About to set VarDir to %s in registry", path);

	if (OpenedKey(HKEY_LOCAL_MACHINE, L"\\Software\\"EDITOR"\\"APPNAME, &hKey))
	{
#ifdef SERVER_URL
		LPCTSTR wServerUrl = L""SERVER_URL;
#endif
#ifdef SEARCH_TERMINALID
		DWORD dwSearchTerminalID = SEARCH_TERMINALID;
#endif
		DWORD dwType = REG_SZ;
		DWORD dwDataSize = sizeof(TCHAR)*(wcslen(wPath)+1);

		// Set VarDir in registry
		if (RegSetValueEx(hKey, L"VarDir", 0, dwType,
		                 (LPBYTE)wPath, dwDataSize) == ERROR_SUCCESS)
		{
			Log("VarDir set in registry");
		}
		else
		{
			Log("Failed to set VarDir in registry");
			DumpError();
		}

#ifdef SERVER_URL
		// Set ServerAgentSetup in registry
		dwType = REG_SZ;
		dwDataSize = sizeof(TCHAR)*(wcslen(wServerUrl)+1);
		if (RegSetValueEx(hKey, L"ServerAgentSetup", 0, dwType,
		                 (LPBYTE)wServerUrl, dwDataSize) == ERROR_SUCCESS)
		{
			Log("ServerAgentSetup set in registry");
		}
		else
		{
			Log("Failed to set ServerAgentSetup in registry");
			DumpError();
		}
#endif

#ifdef SEARCH_TERMINALID
		// Set ServerAgentSetup in registry
		dwType = REG_DWORD;
		dwDataSize = sizeof(dwSearchTerminalID);
		if (RegSetValueEx(hKey, L"SearchTerminalID", 0, dwType,
		                 (LPBYTE)&dwSearchTerminalID, dwDataSize) == ERROR_SUCCESS)
		{
			Log("SearchTerminalID set in registry");
		}
		else
		{
			Log("Failed to set SearchTerminalID in registry");
			DumpError();
		}
#endif

		RegCloseKey(hKey);
	}
	else
	{
		Log("Can't try to set VarDir in registry");
		DumpError();
	}

	/*
	 * Setup registry from setup DLL
	 */
	Log("About to setup service registry as super-service");

	if (OpenedKey(HKEY_LOCAL_MACHINE, L"\\Services", &hKey))
	{
		DWORD dwDispo = 0, dwType = 0, dwData = 0, dwDataSize = 0 ;
		LPTSTR wData = NULL;

		// Create service key in registry
		if (RegCreateKeyEx(hKey, WAPPNAME, 0, NULL, 0, 0, NULL,
		                 &hSubKey, &dwDispo) == ERROR_SUCCESS)
		{
			switch (dwDispo)
			{
				case REG_CREATED_NEW_KEY:
					Log("HKLM\\Services\\"APPNAME" created");
					break;
				case REG_OPENED_EXISTING_KEY:
				default:
					Log("HKLM\\Services\\"APPNAME" opened");
					break;
			}
			RegCloseKey(hKey);
			hKey = hSubKey;

			/*
			 * Set Context in registry
			 * This value must match the one handled in GWA_Init
			 * 0: Normal service / 1: Super-services
			 * See https://blogs.msdn.microsoft.com/cenet/2005/12/14/what-the-service_init_stopped-flag-really-means/
			 * https://blogs.msdn.microsoft.com/cenet/2006/11/28/services-exe-migration-for-applications-in-ce-6-0/
			 */
			dwType = REG_DWORD;
			dwData = 1;
			dwDataSize = sizeof(dwData);
			if (os.dwMajorVersion < 6)
			{
				if (RegSetValueEx(hKey, L"Context", 0, dwType,
								(LPBYTE)&dwData, dwDataSize) == ERROR_SUCCESS)
				{
					Log("Service Context set to %d in registry", dwData);
				}
				else
				{
					Log("Failed to set service Context in registry");
					DumpError();
				}
			} else {
				if (RegSetValueEx(hKey, L"ServiceContext", 0, dwType,
								(LPBYTE)&dwData, dwDataSize) == ERROR_SUCCESS)
				{
					Log("Service ServiceContext set to %d in registry", dwData);
				}
				else
				{
					Log("Failed to set service ServiceContext in registry");
					DumpError();
				}
			}

			// Set Flags in registry
			dwType = REG_DWORD;
			dwData = 2;
			dwDataSize = sizeof(dwData);
			if (RegSetValueEx(hKey, L"Flags", 0, dwType,
							(LPBYTE)&dwData, dwDataSize) == ERROR_SUCCESS)
			{
				Log("Service Flags set to %d in registry", dwData);
			}
			else
			{
				Log("Failed to set service Flags in registry");
				DumpError();
			}

			// Set Index in registry
			dwType = REG_DWORD;
			dwData = 0;
			dwDataSize = sizeof(dwData);
			if (RegSetValueEx(hKey, L"Index", 0, dwType,
							(LPBYTE)&dwData, dwDataSize) == ERROR_SUCCESS)
			{
				Log("Service Index set to %d in registry", dwData);
			}
			else
			{
				Log("Failed to set service Index in registry");
				DumpError();
			}

			// Set Keep in registry
			dwType = REG_DWORD;
			dwData = 1;
			dwDataSize = sizeof(dwData);
			if (RegSetValueEx(hKey, L"Keep", 0, dwType,
							(LPBYTE)&dwData, dwDataSize) == ERROR_SUCCESS)
			{
				Log("Service Keep set to %d in registry", dwData);
			}
			else
			{
				Log("Failed to set service Keep in registry");
				DumpError();
			}

			// Set Order in registry
			dwType = REG_DWORD;
			dwData = 16;
			dwDataSize = sizeof(dwData);
			if (RegSetValueEx(hKey, L"Order", 0, dwType,
							(LPBYTE)&dwData, dwDataSize) == ERROR_SUCCESS)
			{
				Log("Service Order set to %d in registry", dwData);
			}
			else
			{
				Log("Failed to set service Order in registry");
				DumpError();
			}

			// Set Prefix in registry
			dwType = REG_SZ;
			wData = L"GWA";
			dwDataSize = sizeof(TCHAR)*(wcslen(wData)+1);
			if (RegSetValueEx(hKey, L"Prefix", 0, dwType,
							(LPBYTE)wData, dwDataSize) == ERROR_SUCCESS)
			{
				Log("Service Prefix set in registry");
			}
			else
			{
				Log("Failed to set service Prefix in registry");
				DumpError();
			}

			// Set Dll in registry
			dwType = REG_SZ;
			swprintf(wPath, L"%s%hs", pszinstalldir, "\\glpi-agent.dll");
			dwDataSize = sizeof(TCHAR)*(wcslen(wPath)+1);
			if (RegSetValueEx(hKey, L"Dll", 0, dwType,
							(LPBYTE)wPath, dwDataSize) == ERROR_SUCCESS)
			{
				wcstombs(path, wPath, MAX_PATH);
				Log("Service Dll set in registry to '%s'", path);
			}
			else
			{
				Log("Failed to set service Dll in registry");
				DumpError();
			}

			// Set DisplayName in registry
			dwType = REG_SZ;
			wData = WAPPNAME;
			dwDataSize = sizeof(TCHAR)*(wcslen(wData)+1);
			if (RegSetValueEx(hKey, L"DisplayName", 0, dwType,
							(LPBYTE)wData, dwDataSize) == ERROR_SUCCESS)
			{
				Log("Service DisplayName set in registry");
			}
			else
			{
				Log("Failed to set service DisplayName in registry");
				DumpError();
			}

			// Set Description in registry
			dwType = REG_SZ;
			wData = L"GLPI Agent service v"VERSION;
			dwDataSize = sizeof(TCHAR)*(wcslen(wData)+1);
			if (RegSetValueEx(hKey, L"Description", 0, dwType,
							(LPBYTE)wData, dwDataSize) == ERROR_SUCCESS)
			{
				Log("Service Description set in registry");
			}
			else
			{
				Log("Failed to set service Description in registry");
				DumpError();
			}

			// Create super-service key in registry
			if (RegCreateKeyEx(hKey, L"Accept", 0, NULL, 0, 0, NULL,
							&hSubKey, &dwDispo) == ERROR_SUCCESS)
			{
				switch (dwDispo)
				{
					case REG_CREATED_NEW_KEY:
						Log("HKLM\\Service\\"APPNAME"\\Accept created");
						break;
					case REG_OPENED_EXISTING_KEY:
					default:
						Log("HKLM\\Service\\"APPNAME"\\Accept opened");
						break;
				}
				RegCloseKey(hKey);
				hKey = hSubKey;
				if (RegCreateKeyEx(hKey, L"TCP-62354", 0, NULL, 0, 0, NULL,
								&hSubKey, &dwDispo) == ERROR_SUCCESS)
				{
					switch (dwDispo)
					{
						case REG_CREATED_NEW_KEY:
							Log("HKLM\\Service\\"APPNAME"\\Accept\\TCP-62354 created");
							break;
						case REG_OPENED_EXISTING_KEY:
						default:
							Log("HKLM\\Service\\"APPNAME"\\Accept\\TCP-62354 opened");
							break;
					}
					RegCloseKey(hKey);
					hKey = hSubKey;

					// Set TCP-62354 in registry
					{
						SOCKADDR_IN portData ;
						memset(&portData, 0, sizeof(portData));
						portData.sin_family = SOCK_DGRAM;
						portData.sin_port   = 62354;
						dwType = REG_BINARY;
						dwDataSize = sizeof(portData);
						if (RegSetValueEx(hKey, L"SockAddr", 0, dwType,
										(LPBYTE)&portData, dwDataSize) == ERROR_SUCCESS)
						{
							Log("Service SockAddr set in registry");
						}
						else
						{
							Log("Failed to set service SockAddr in registry");
							DumpError();
						}
					}
				}
				else
				{
					Log("Failed to setup service TCP-62354 key in registry");
					DumpError();
				}
			}
			else
			{
				Log("Failed to setup service Accept key in registry");
				DumpError();
			}
		}
		else
		{
			Log("Failed to create service base key in registry");
			DumpError();
		}
		RegCloseKey(hKey);
	}
	else
	{
		Log("Can't try to setup service in registry");
		DumpError();
	}

	if (cfaileddirs>0)
		Log("Failed dirs      : %d", cfaileddirs);
	if (cfailedfiles>0)
		Log("Failed files     : %d", cfailedfiles);
	if (cfailedregkeys>0)
		Log("Failed regkeys   : %d", cfailedregkeys);
	if (cfailedregvals>0)
		Log("Failed regvals   : %d", cfailedregvals);
	if (cfailedshortcuts>0)
		Log("Failed shortcuts : %d", cfailedshortcuts);

	// Try to load the service
	Log("Activating " APPNAME " " VERSION " service...");

	SetLastError(NO_ERROR);
	hService = ActivateService(WAPPNAME, 0);
	if (hService == INVALID_HANDLE_VALUE)
	{
		Log("Failed to activate " APPNAME " " VERSION " service");
		DumpError();
		cfailedfiles++;
	}
	else
	{
		Log(APPNAME " v" VERSION " service activated");
		CloseHandle(hService);
	}

	cfaileddirs += cfailedfiles+cfailedregkeys+cfailedregvals+cfailedshortcuts;
	if (cfaileddirs == 0)
	{
		Log("Installation completed");
	}
	else
	{
		Log("Installation not fully completed");
	}

#ifdef TEST
	Log("Dumping registry while service installed...");
	DebugRegistry();
#endif

	free(wPath);
	free(path);

	return codeINSTALL_EXIT_DONE;
}

/**
 * Handles tasks done at beginning of uninstallation
 * Also secondly called while updating or re-installing, but from the previous dll version
 */
codeUNINSTALL_INIT
Uninstall_Init(HWND hwndparent, LPCTSTR pszinstalldir)
{
	LPSTR installdir = NULL;
	int length = 0;
	HKEY hKey = NULL;

	// Initializes OS informations
	os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (!GetVersionEx( &os ))
	{
		Log("Failed to get system version");
		// Must not happen, but just default to wince 5 while failing
		os.dwMajorVersion = 5;
	}

	length = wcslen(pszinstalldir)+1;
	installdir = malloc(length);
	wcstombs(installdir, pszinstalldir, length);
	Log("Uninstalling " APPNAME " v" VERSION ": PATH=%s", installdir);
	free(installdir);

	// Keep installdir & vardir for cleanup while leaving uninstall
	if (OpenedKey(HKEY_LOCAL_MACHINE, L"\\Software\\"EDITOR"\\"APPNAME, &hKey))
	{
		DWORD dwType = REG_SZ;
		DWORD dwDataSize = 2*MAX_PATH;
		wInstalldir = malloc(dwDataSize);
		if (!RegQueryValueEx(hKey, L"Installdir", NULL, &dwType,
		                    (LPBYTE)wInstalldir, &dwDataSize) == ERROR_SUCCESS)
		{
			Log("Can't get " APPNAME " install directory");
			DumpError();
		}
		dwDataSize = 2*MAX_PATH;
		wVardir = malloc(dwDataSize);
		if (!RegQueryValueEx(hKey, L"VarDir", NULL, &dwType,
		                    (LPBYTE)wVardir, &dwDataSize) == ERROR_SUCCESS)
		{
			Log("Can't get " APPNAME " var directory");
			DumpError();
		}
		RegCloseKey(hKey);
	}

#ifdef TEST
	Log("Dumping registry before disabling service...");
	DebugRegistry();
#endif

	// Disable the service
	StopAndDisableService(VERSION);

	// Delete service registry keys
	if (OpenedKey(HKEY_LOCAL_MACHINE, L"\\Services\\"APPNAME"\\Accept\\TCP-62354", &hKey))
	{
		Log("Removing TCP server on 62354 port");
		RegDeleteValue(hKey, L"SockAddr");
		RegCloseKey(hKey);
	}

	if (OpenedKey(HKEY_LOCAL_MACHINE, L"\\Services\\"APPNAME"\\Accept", &hKey))
	{
		Log("Removing TCP server");
		RegDeleteKey(hKey, L"TCP-62354");
		RegCloseKey(hKey);
	}

	if (OpenedKey(HKEY_LOCAL_MACHINE, L"\\Services\\"APPNAME, &hKey))
	{
		Log("Removing " APPNAME " service configuration");
		// SubKey to delete
		RegDeleteKey(hKey, L"Accept");
		RegDeleteValue(hKey, L"Dll");
		RegDeleteValue(hKey, L"Order");
		RegDeleteValue(hKey, L"Keep");
		RegDeleteValue(hKey, L"Prefix");
		RegDeleteValue(hKey, L"Index");
		RegDeleteValue(hKey, L"DisplayName");
		RegDeleteValue(hKey, L"Description");
		RegDeleteValue(hKey, L"Flags");
		if (os.dwMajorVersion < 6)
		{
			RegDeleteValue(hKey, L"Context");
		} else {
			RegDeleteValue(hKey, L"ServiceContext");
			// Added by system
			RegDeleteValue(hKey, L"UserProcGroup");
		}
		RegCloseKey(hKey);
	}

	if (OpenedKey(HKEY_LOCAL_MACHINE, L"\\Services", &hKey))
	{
		// Does not work on some devices. Btw this is not critical.
		if (RegDeleteKey(hKey, WAPPNAME) != ERROR_SUCCESS)
		{
			Log("Failure while removing service base registry key");
			DumpError();
		}
		RegCloseKey(hKey);
		Log(APPNAME " service removed");
	}

	// Checking application registry keys
	if (OpenedKey(HKEY_LOCAL_MACHINE, L"\\Software\\"EDITOR"\\"APPNAME, &hKey))
	{
		// Check first if we are requested to keep files
		DWORD dwType = REG_DWORD;
		DWORD dwDataSize = sizeof(keepFiles);
		if (RegQueryValueEx(hKey, L"KeepFiles", NULL, &dwType,
		                    (LPBYTE)&keepFiles, &dwDataSize) == ERROR_SUCCESS)
		{
			if (dwType == REG_DWORD && dwDataSize == sizeof(DWORD))
			{
				if (keepFiles)
				{
					Log("Keeping files requested on update");
				}
				else
				{
					Log("Removing files requested on uninstall");
				}
			}
			else
			{
				Log("Removing files requested as expecting uninstall");
				keepFiles = 0;
			}
		}
		RegCloseKey(hKey);
	}

	if (keepFiles)
	{
		Log("Will keep " APPNAME " related files");
	}
	else
	{
		Log("Have to delete " APPNAME " related files");
	}

	return codeUNINSTALL_INIT_CONTINUE;
}

/**
 * Handles tasks done at end of uninstallation
 */
codeUNINSTALL_EXIT
Uninstall_Exit(HWND hwndparent)
{
	HKEY hKey = NULL;
	DWORD EditorSubKeys = 0, EditorValues = 0;

	// Delete application registry keys
	if (OpenedKey(HKEY_LOCAL_MACHINE, L"\\Software\\"EDITOR"\\"APPNAME, &hKey))
	{
		Log("Removing " APPNAME " install values...");
		RegDeleteValue(hKey, L"InstallDir");
		RegDeleteValue(hKey, L"VarDir");
		RegDeleteValue(hKey, L"Version");
		if (!keepFiles)
		{
			RegDeleteValue(hKey, L"KeepFiles");
		}
		RegCloseKey(hKey);
	}

	if (!keepFiles && OpenedKey(HKEY_LOCAL_MACHINE, L"\\Software\\"EDITOR, &hKey))
	{
		RegDeleteKey(hKey, WAPPNAME);
		// Count \Software\EDITOR subkeys & values
		RegQueryInfoKey(hKey, NULL, NULL, NULL,
		                &EditorSubKeys, NULL, NULL, &EditorValues, NULL, NULL,
		                NULL, NULL);
		RegCloseKey(hKey);
		Log(APPNAME " key removed");
	}

	// Check to remove \Software\EDITOR when empty
	if (EditorSubKeys == 0 && EditorValues == 0 && !keepFiles &&
	    OpenedKey(HKEY_LOCAL_MACHINE, L"\\Software", &hKey))
	{
		RegDeleteKey(hKey, WEDITOR);
		RegCloseKey(hKey);
		Log(EDITOR " key uninstalled");
	}

	if (keepFiles)
	{
		Log("Keeping " APPNAME " related files");
	}
	else if (!wInstalldir || !wcslen(wInstalldir)) {
		Log("Can't delete" APPNAME " related files without installation directory");
	}
	else
	{
		LPTSTR wPath = NULL;
		LPSTR path = NULL;
		wPath = malloc(2*MAX_PATH);
		path = malloc(MAX_PATH);
		Log("Deleting " APPNAME " related files and folders");
		if (!wVardir || !wcslen(wVardir)) {
			Log("Can't delete" APPNAME " some related files without var directory");
		}
		else
		{
#ifndef TEST
			swprintf(wPath, L"%s\\%hs", wVardir, JOURNALBASENAME);
#else
			swprintf(wPath, L"%hs", JOURNALBASENAME);
#endif
			if (!DeleteFile(wPath))
			{
				wcstombs(path, wPath, MAX_PATH);
				Log("Failed to delete '%s'", path);
				DumpError();
			}
#ifndef TEST
			swprintf(wPath, L"%s\\%hs", wVardir, INTERFACEBASENAME);
#else
			swprintf(wPath, L"%hs", INTERFACEBASENAME);
#endif
			if (!DeleteFile(wPath))
			{
				wcstombs(path, wPath, MAX_PATH);
				Log("Failed to delete '%s'", path);
				DumpError();
			}
			swprintf(wPath, L"%s\\%hs", wVardir, DefaultConfigFile);
			if (!DeleteFile(wPath))
			{
				wcstombs(path, wPath, MAX_PATH);
				Log("Failed to delete '%s'", path);
				DumpError();
			}
			swprintf(wPath, L"%s\\%hs.dump", wVardir, APPNAME);
			if (!DeleteFile(wPath))
			{
				wcstombs(path, wPath, MAX_PATH);
				Log("Failed to delete '%s'", path);
				DumpError();
			}
			if (!RemoveDirectory(wVardir))
			{
				wcstombs(path, wVardir, MAX_PATH);
				Log("Failed to remove '%s' folder", path);
				DumpError();
			}
		}
		if (hFile != NULL)
			fclose(hFile);
		swprintf(wPath, L"%s\\%hs", wInstalldir, cstrInstallJournal);
		if (!DeleteFile(wPath))
		{
			wcstombs(path, wPath, MAX_PATH);
			Log("Failed to delete '%s\\%s'", path);
			DumpError();
		}
		// Finish by wInstalldir as wVardir may be a sub-folder
		if(!RemoveDirectory(wInstalldir))
		{
			wcstombs(path, wInstalldir, MAX_PATH);
			Log("Failed to remove '%s' folder", path);
			DumpError();
		}
		free(wPath);
		free(path);
	}
	free(wVardir);
	free(wInstalldir);

	Log(APPNAME " uninstalled");

#ifdef TEST
	Log("Dumping registry on uninstalled service...");
	DebugRegistry();
#endif

	return codeUNINSTALL_EXIT_DONE;
}

BOOL APIENTRY
DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
	LPCSTR hdr = APPNAME "-Setup (Built: " __DATE__ ", " __TIME__ ")";
	LPSTR templogpath = NULL;
	LPTSTR wTempLogPath = NULL;

	templogpath = malloc(strlen(cstrInstallJournal)+7);
	sprintf( templogpath, "\\Temp\\%s", cstrInstallJournal );
	wTempLogPath = malloc(2*(strlen(templogpath)+1));
	swprintf( wTempLogPath, L"%hs", templogpath );

	if (hFile == NULL)
	{
		hFile = freopen( templogpath, "a+", stdout );
	}

	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
#ifdef TEST
			fprintf(stderr, "%s: DllMain: loading\n", hdr);
#endif
			Log("%s: Library loaded", hdr);
			break;
		case DLL_THREAD_ATTACH:
			//Log("%s: thread attached", hdr);
			break;
		case DLL_THREAD_DETACH:
			//Log("%s: thread detached", hdr);
			break;
		case DLL_PROCESS_DETACH:
#ifdef TEST
			fprintf(stderr, "%s: DllMain: unloading\n", hdr);
#endif
			Log("%s: unloading", hdr);
			if (hFile != NULL)
				fclose(hFile);
#ifndef TEST
			// Remove first cstrInstallJournal created file
			DeleteFile(wTempLogPath);
#endif
			break;
	}

	free(wTempLogPath);
	free(templogpath);

	return TRUE;
}
