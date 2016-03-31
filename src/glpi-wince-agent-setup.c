
/*
 * Defines the entry point for the DLL application.
 */

#include <windows.h>

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

/**
 * Handles tasks done at start of installation
 */
codeINSTALL_INIT
Install_Init(HWND hwndparent, BOOL ffirstcall, BOOL fpreviouslyinstalled,
             LPCTSTR pszinstalldir)
{
	HKEY hKey = NULL;

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"\\Software\\Teclib\\GLPI-Agent",
	                 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
	{
		RegDeleteValue(hKey, L"InstallDir");
		RegDeleteValue(hKey, L"Version");
		RegCloseKey(hKey);
	}

	return codeINSTALL_INIT_CONTINUE;
}

/**
 * Handles tasks done at end of installation
 */
codeINSTALL_EXIT
Install_Exit(HWND hwndparent, LPCTSTR pszinstalldir, WORD cfaileddirs,
             WORD cfailedfiles, WORD cfailedregkeys, WORD cfailedregvals,
             WORD cfailedshortcuts)
{
	return codeINSTALL_EXIT_DONE;
}

/**
 * Handles tasks done at beginning of uninstallation
 */
codeUNINSTALL_INIT
Uninstall_Init(HWND hwndparent, LPCTSTR pszinstalldir)
{
	HKEY hKey = NULL;
	DWORD TeclibSubKeys = 0, TeclibValues = 0;

	// Delete application registry keys
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"\\Software\\Teclib\\GLPI-Agent",
	                 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
	{
		RegDeleteValue(hKey, L"InstallDir");
		RegDeleteValue(hKey, L"Version");
		RegCloseKey(hKey);
	}

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"\\Software\\Teclib",
	                 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
	{
		RegDeleteKey(hKey, L"GLPI-Agent");
		// Count \Software\Teclib subkeys & values
		RegQueryInfoKey(hKey, NULL, NULL, NULL,
		                &TeclibSubKeys, NULL, NULL, &TeclibValues, NULL, NULL,
		                NULL, NULL);
		RegCloseKey(hKey);
	}

	// Check to remove \Software\Teclib when empty
	if (TeclibSubKeys == 0 && TeclibValues == 0 &&
	    RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"\\Software",
	                 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
	{
		RegDeleteKey(hKey, L"Teclib");
		RegCloseKey(hKey);
	}

	return codeUNINSTALL_INIT_CONTINUE;
}

/**
 * Handles tasks done at end of uninstallation
 */
codeUNINSTALL_EXIT
Uninstall_Exit(HWND hwndparent)
{
	return codeUNINSTALL_EXIT_DONE;
}

BOOL APIENTRY
DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	return TRUE;
}
