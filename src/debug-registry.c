
/*
 * Defines the entry point for the DLL application.
 */

#include <windows.h>

#include "glpi-wince-agent.h"

void Log(LPCSTR format, ...)
{
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	vfprintf(stderr, format, args);
	va_end(args);
	printf("\n");
	fprintf(stderr, "\n");
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
				lpValue = malloc(BUFSIZE);
				memset(lpValue, 0, BUFSIZE);
				while (lpData != NULL && dwData-->0)
				{
					length = strlen(lpValue);
					if (length>0)
					{
						strcat(lpValue, ",");
						length++;
					}
					sprintf(lpValue+length, "0x%02x", (BYTE)(*lpData++));
					if (length>70)
					{
						Log("Raw: %s", lpValue);
						*lpValue = '\0';
					}
				}
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

	// Dump Comm keys
	if (OpenedKey(HKEY_LOCAL_MACHINE, L"\\COMM", &hKey))
	{
		Log("Dumping HKEY_LOCAL_MACHINE\\COMM:");
		DumpRegKey(hKey, L"\\COMM");
		RegCloseKey(hKey);
		Log("Dump finished");
	}

	// Dump Init keys
	if (OpenedKey(HKEY_LOCAL_MACHINE, L"\\Init", &hKey))
	{
		Log("Dumping HKEY_LOCAL_MACHINE\\Init:");
		DumpRegKey(hKey, L"\\Init");
		RegCloseKey(hKey);
		Log("Dump finished");
	}
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	FILE *hFile = NULL;
	SYSTEMTIME ts ;
	LPCSTR cstrLog = "\\debug-" APPNAME "-registry.txt";

	hFile = freopen( cstrLog, "a+", stdout );

	GetLocalTime(&ts);
	printf("%4d-%02d-%02d %02d:%02d:%02d: Dumping registry...",
		ts.wYear, ts.wMonth, ts.wDay, ts.wHour, ts.wMinute, ts.wSecond);

	DebugRegistry();

	fclose(hFile);

	return FALSE;
}
