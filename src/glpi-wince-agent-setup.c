
/*
 * Defines the entry point for the DLL application.
 */

#include <windows.h>
#include <shlobj.h>
#include <service.h>

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
DWORD keepFiles = 0;

#define SHORT_BUFFER_SIZE 16

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
	DWORD dwDllBuf = 0;

	Log("Looking to stop and disable service...");

	//hService = CreateFile(L"GWA0:",GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
	hService = GetServiceHandle(L"GWA0:", NULL, &dwDllBuf);

	if (hService != INVALID_HANDLE_VALUE)
	{
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

	if (bFirstcall)
	{
		// Truncate installation log from here
		if (hFile != NULL)
		{
			fclose(hFile);
			hFile = freopen( "\\Windows\\" APPNAME "-install.txt", "w", stdout );
		}

#ifdef TEST
		// Dump Services keys
		if (OpenedKey(HKEY_LOCAL_MACHINE, L"\\Services", &hKey))
		{
			Log("Dumping HKEY_LOCAL_MACHINE\\Services:");
			DumpRegKey(hKey, L"\\Services");
			RegCloseKey(hKey);
			Log("Dump finished");
		}
		// Dump Softwares keys
		if (OpenedKey(HKEY_LOCAL_MACHINE, L"\\Software", &hKey))
		{
			Log("Dumping HKEY_LOCAL_MACHINE\\Software:");
			DumpRegKey(hKey, L"\\Software");
			RegCloseKey(hKey);
			Log("Dump finished");
		}
#endif
	}

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
	HKEY hKey = NULL;

#ifdef TEST
	// Dump Services keys
	if (OpenedKey(HKEY_LOCAL_MACHINE, L"\\Services", &hKey))
	{
		Log("Dumping HKEY_LOCAL_MACHINE\\Services before activating service:");
		DumpRegKey(hKey, L"\\Services");
		RegCloseKey(hKey);
		Log("Dump finished");
	}
	// Dump Softwares keys
	if (OpenedKey(HKEY_LOCAL_MACHINE, L"\\Software", &hKey))
	{
		Log("Dumping HKEY_LOCAL_MACHINE\\Software:");
		DumpRegKey(hKey, L"\\Software");
		RegCloseKey(hKey);
		Log("Dump finished");
	}
#endif

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
		RegCloseKey(hKey);
	}
	else
	{
		Log("Can't try to set VarDir in registry");
		DumpError();
	}
	free(wPath);
	free(path);

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
	DWORD EditorSubKeys = 0, EditorValues = 0;

	length = wcslen(pszinstalldir)+1;
	installdir = malloc(length);
	wcstombs(installdir, pszinstalldir, length);
	Log("Uninstalling " APPNAME " v" VERSION ": PATH=%s", installdir);
	free(installdir);

#ifdef TEST
	// Dump Services keys
	if (OpenedKey(HKEY_LOCAL_MACHINE, L"\\Services", &hKey))
	{
		Log("Dumping HKEY_LOCAL_MACHINE\\Services before activating service:");
		DumpRegKey(hKey, L"\\Services");
		RegCloseKey(hKey);
		Log("Dump finished");
	}
	// Dump Softwares keys
	if (OpenedKey(HKEY_LOCAL_MACHINE, L"\\Software", &hKey))
	{
		Log("Dumping HKEY_LOCAL_MACHINE\\Software:");
		DumpRegKey(hKey, L"\\Software");
		RegCloseKey(hKey);
		Log("Dump finished");
	}
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
		// Values installed from inf
		RegDeleteValue(hKey, L"Dll");
		RegDeleteValue(hKey, L"Order");
		RegDeleteValue(hKey, L"Keep");
		RegDeleteValue(hKey, L"Prefix");
		RegDeleteValue(hKey, L"Index");
		RegDeleteValue(hKey, L"Context");
		RegDeleteValue(hKey, L"DisplayName");
		RegDeleteValue(hKey, L"Description");
		RegDeleteValue(hKey, L"Flags");
		RegCloseKey(hKey);
	}

	if (OpenedKey(HKEY_LOCAL_MACHINE, L"\\Services", &hKey))
	{
		RegDeleteKey(hKey, WAPPNAME);
		RegCloseKey(hKey);
		Log(APPNAME " service removed");
	}

	// Delete application registry keys
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
	if (keepFiles)
	{
		Log("Keeping " APPNAME " related files");
	}
	else
	{
		Log("TODO: Deleting " APPNAME " related files and folders");
		// TODO clean-up related folders
	}

	Log(APPNAME " uninstalled");

	return codeUNINSTALL_EXIT_DONE;
}

BOOL APIENTRY
DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
	LPCSTR hdr = APPNAME "-Setup (Built: " __DATE__ ", " __TIME__ ")";

	if (hFile == NULL)
	{
		hFile = freopen( "\\Windows\\" APPNAME "-install.txt", "a+", stdout );
	}

	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
			fprintf(stderr, "%s: DllMain: loading\n", hdr);
			Log("%s: Library loaded", hdr);
			break;
		case DLL_THREAD_ATTACH:
			//Log("%s: thread attached", hdr);
			break;
		case DLL_THREAD_DETACH:
			//Log("%s: thread detached", hdr);
			break;
		case DLL_PROCESS_DETACH:
			fprintf(stderr, "%s: DllMain: unloading\n", hdr);
			Log("%s: unloading", hdr);
			if (hFile != NULL)
				fclose(hFile);
			break;
	}

	return TRUE;
}
