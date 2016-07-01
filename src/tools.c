/*
 * GLPI Windows CE Agent
 * 
 * Copyright (C) 2016 - Teclib SAS
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 */

#include <windows.h>
#include <winbase.h>

#include "glpi-wince-agent.h"

#define MAX_VS_SIZE 256
#define MAX_TS_SIZE 40
#define MAX_WV_SIZE 16

static DWORD dwStartTick = 0;
static LPSTR sHostname = NULL;

PFIXED_INFO pFixedInfo = NULL;

void *allocate(ULONG size, LPCSTR reason )
{
	void *pointer = NULL;

	if (size == 0) {
		Error("[%s] No need to allocate memory", reason);
	} else {
		// Allocate memory from sizing information
		if ((pointer = malloc(size)) == NULL)
		{
			if (reason != NULL)
				Error("[%s] Can't allocate %lu bytes", reason, size);
			else
				Error("Can't allocate %lu bytes", size);
#ifndef GWA
			Abort();
#endif
		}
#ifdef DEBUG
		else
		{
			if (reason != NULL)
				Debug2("[%s] Allocated %i bytes", reason, size);
		}
#endif
	}
	return pointer;
}

PFIXED_INFO getNetworkParams(void)
{
	static DWORD expiration = 0;
	DWORD	Err = 0, FixedInfoSize = 0;

	if (pFixedInfo != NULL && GetTickCount() <= expiration)
		return pFixedInfo;

	/*
	 *  Get the main IP configuration information for this machine using
	 * the FIXED_INFO structure
	 */
	if ((Err = GetNetworkParams(NULL, &FixedInfoSize)) != 0)
	{
		if ((Err != ERROR_BUFFER_OVERFLOW) && (Err != ERROR_INSUFFICIENT_BUFFER))
		{
			Error("GetNetworkParams() sizing failed with error %lu\n", Err);
#ifndef GWA
			Abort();
#endif
		}
	}

	// Allocate memory
	free(pFixedInfo);
	pFixedInfo = allocate(FixedInfoSize, "GetNetworkParams");

	// Retreive network params
	if ((Err = GetNetworkParams(pFixedInfo, &FixedInfoSize)) != 0) {
		Error("GetNetworkParams() failed with error code %lu\n", Err);
#ifndef GWA
		Abort();
#endif
	}

	expiration = GetTickCount() - dwStartTick + EXPIRATION_DELAY;

	return pFixedInfo;
}

LPSTR getHostname(void)
{
	static DWORD expiration = 0;
	int buflen = 1;

	if (sHostname == NULL || GetTickCount() > expiration)
	{
		getNetworkParams();

		// Allocate memory & copy hostname
		free(sHostname);
		buflen += strlen(pFixedInfo->HostName);
		sHostname = allocate( buflen, "Hostname");
		strcpy( sHostname, pFixedInfo->HostName);

		expiration = GetTickCount() - dwStartTick + EXPIRATION_DELAY;
	}

	return sHostname;
}

// Timestamp to be used while debugging
LPSTR getTimestamp(void)
{
	static char timestamp[MAX_TS_SIZE];
	int length = 0;
	LPSYSTEMTIME lpLocalTime = NULL;

	lpLocalTime = getLocalTime();

	if (conf.debug)
	{
		// Insert accurate time since agent was started
		DWORD ticks = GetTickCount() - dwStartTick;
		length = _snprintf(
			timestamp, MAX_TS_SIZE-1,
			"%4d-%02d-%02d %02d:%02d:%02d: %li.%03li: ",
			lpLocalTime->wYear, lpLocalTime->wMonth, lpLocalTime->wDay,
			lpLocalTime->wHour, lpLocalTime->wMinute, lpLocalTime->wSecond,
			ticks/1000, ticks%1000
		);
	}
	else
	{
		length = _snprintf(
			timestamp, MAX_TS_SIZE-1,
			"%4d-%02d-%02d %02d:%02d:%02d: ",
			lpLocalTime->wYear, lpLocalTime->wMonth, lpLocalTime->wDay,
			lpLocalTime->wHour, lpLocalTime->wMinute, lpLocalTime->wSecond
		);
	}

	// Guaranty to keep a NULL terminated string
	if (length == MAX_TS_SIZE-1 || length < 0)
		timestamp[MAX_TS_SIZE-1] = '\0';

	return timestamp;
}

static void addFileTimeMinutes( LPFILETIME lpFt, DWORD dwMinutes )
{
	ULARGE_INTEGER ft ;

	ft.LowPart  = lpFt->dwLowDateTime;
	ft.HighPart = lpFt->dwHighDateTime;

	ft.LowPart += (ULONG)dwMinutes*600000;
	if ( (ULONG)ft.LowPart < (ULONG)lpFt->dwLowDateTime )
	{
		ft.HighPart++;
	}

	lpFt->dwLowDateTime =  ft.LowPart;
	lpFt->dwHighDateTime = ft.HighPart;
}

void computeNextRunDate(void)
{
#ifdef STDERR
	stderrf("Current nextRunDate: 0x%08x:0x%08x",
		nextRunDate.dwHighDateTime, nextRunDate.dwLowDateTime);
#endif
	// Initial delay
	if (nextRunDate.dwLowDateTime == 0 && nextRunDate.dwHighDateTime == 0)
	{
		GetCurrentFT(&nextRunDate);
		addFileTimeMinutes(&nextRunDate, DEFAULT_INITIAL_DELAY/2 +
			Random()%(DEFAULT_INITIAL_DELAY/2));
	}
	else
	{
		addFileTimeMinutes(&nextRunDate, DEFAULT_MAX_DELAY/2 +
			Random()%(DEFAULT_MAX_DELAY/2));
	}
#ifdef STDERR
	stderrf("Computed nextRunDate: 0x%08x:0x%08x",
		nextRunDate.dwHighDateTime, nextRunDate.dwLowDateTime);
#endif
}

BOOL timeToSubmit(void)
{
	FILETIME now = { 0, 0 };
#ifdef STDERR
	stderrf("Getting current FT");
#endif
	GetCurrentFT(&now);
#ifdef STDERR
	stderrf("Comparing current FT to nextRunDate: 0x%08x:0x%08x vs 0x%08x:0x%08x",
		nextRunDate.dwHighDateTime, nextRunDate.dwLowDateTime,
		now.dwHighDateTime, now.dwLowDateTime
	);
#endif
	return (CompareFileTime(&nextRunDate,&now) < 0);
}

LPSYSTEMTIME getLocalTime(void)
{
	static SYSTEMTIME __systemtime ;
	GetLocalTime(&__systemtime);
	return &__systemtime;
}

void ToolsInit(void)
{
	if (dwStartTick == 0)
	{
		dwStartTick = GetTickCount();
	}

	if (nextRunDate.dwLowDateTime == 0 && nextRunDate.dwHighDateTime == 0)
	{
		computeNextRunDate();
	}
}

void ToolsQuit(void)
{
	Debug2("Freeing Tools");
	free(pFixedInfo);
	free(sHostname);

	pFixedInfo = NULL;
	sHostname  = NULL;
}

LPSTR getRegString( HKEY hive, LPCSTR subkey, LPCSTR value )
{
	LPSTR result = NULL;

#ifdef STDERR
	stderrf("Looking for %s/%s string in registry", subkey, value);
#endif
	if (hive != NULL && subkey != NULL && value != NULL)
	{
		LPWSTR wKeypath = NULL;
		HKEY hKey = NULL;
		int length = strlen(subkey)+1;
		wKeypath = allocate(2*length, "subkeypath");
		swprintf( wKeypath, L"%hs", subkey );
		if (OpenedKey(hive, wKeypath, &hKey))
		{
			WCHAR *wValue = NULL;
			LPBYTE lpData = NULL;
			DWORD dwType = REG_SZ, dwDataSize = 2*MAX_PATH;
			length = strlen(value)+1;
			wValue = allocate(2*length, "valuename");
			lpData = allocate(dwDataSize, "getRegString");
			swprintf( wValue, L"%hs", value );
			if (RegQueryValueEx(hKey, wValue, NULL, &dwType,
								lpData, &dwDataSize) == ERROR_SUCCESS)
			{
				if (dwType == REG_SZ)
				{
					dwDataSize /= 2;
					if (dwDataSize && dwDataSize<MAX_PATH)
					{
						result = allocate(dwDataSize, "RegValue");
						memset( result, 0, dwDataSize );
						wcstombs(result, (LPTSTR)lpData, dwDataSize);
						Debug2("Found string %s\\%s='%s'", subkey, value, result);
#ifdef STDERR
						stderrf("Found %s\\%s=%s in reg, len=%d", subkey, value, result, dwDataSize);
#endif
					}
					else
					{
						Debug2("Can't get %s\\%s string", subkey, value);
#ifdef STDERR
						stderrf("Failed to read %s\\%s string in registry (len=%d)", subkey, value, dwDataSize);
#endif
					}
				}
				else
				{
#ifdef STDERR
					stderrf("Failed to read %s\\%s as not a string in registry (len=%d;type=%d)", subkey, value, dwDataSize, dwType);
#endif
					Debug2("Can't get %s\\%s as not a string (%d)", subkey, value, dwType);
				}
			}
			else
			{
				Debug2("%s\\%s string not available", subkey, value);
			}
			free(lpData);
			free(wValue);
			RegCloseKey(hKey);
		}
#ifdef STDERR
		else
		{
			stderrf("Failed to read %s path in registry", value);
		}
#endif
		free(wKeypath);
	}

	return result;
}

DWORD getRegValue( HKEY hive, LPCSTR subkey, LPCSTR value )
{
	DWORD result = 0;

#ifdef STDERR
	stderrf("Looking for %s/%s value in registry", subkey, value);
#endif
	if (hive != NULL && subkey != NULL && value != NULL)
	{
		LPWSTR wKeypath = NULL;
		HKEY hKey = NULL;
		int length = strlen(subkey)+1;
		wKeypath = allocate(2*length, "subkeypath");
		swprintf( wKeypath, L"%hs", subkey );
		if (OpenedKey(hive, wKeypath, &hKey))
		{
			WCHAR *wValue = NULL;
			DWORD dwType = REG_DWORD, dwDataSize = sizeof(result);
			length = strlen(value)+1;
			wValue = allocate(2*length, "valuename");
			swprintf( wValue, L"%hs", value );
			if (RegQueryValueEx(hKey, wValue, NULL, &dwType,
								(LPBYTE)&result, &dwDataSize) == ERROR_SUCCESS)
			{
				if (dwType == REG_DWORD)
				{
					Debug2("Found value %s\\%s=%d", subkey, value, result);
#ifdef STDERR
					stderrf("Found %s\\%s=%d in reg", subkey, value, result);
#endif
				}
				else
				{
					Error("Can't get %s\\%s value", subkey, value);
#ifdef STDERR
					stderrf("Failed to read %s\\%s as not a dword in registry (len=%d;type=%d)", subkey, value, dwDataSize, dwType);
#endif
				}
			}
			else
			{
				Debug2("%s\\%s value not available", subkey, value);
			}
			free(wValue);
			RegCloseKey(hKey);
		}
#ifdef STDERR
		else
		{
			stderrf("Failed to read %s path in registry", value);
		}
#endif
		free(wKeypath);
	}

	return result;
}

LPSTR getRegPath( LPCSTR value )
{
	LPSTR path = NULL;

#ifdef STDERR
	stderrf("Looking for %s path in registry", value);
#endif
	if (value != NULL && strlen(value) > 0 && strlen(value)<MAX_WV_SIZE)
	{
		HKEY hKey = NULL;
		if (OpenedKey(HKEY_LOCAL_MACHINE, L"\\Software\\"EDITOR"\\"APPNAME, &hKey))
		{
			WCHAR wValue[MAX_WV_SIZE];
			DWORD dwType = REG_SZ, dwDataSize = 2*MAX_PATH;
			LPBYTE lpData = allocate(dwDataSize, "getRegPath");
			swprintf( wValue, L"%hs", value );
			if (RegQueryValueEx(hKey, wValue, NULL, &dwType,
								lpData, &dwDataSize) == ERROR_SUCCESS)
			{
				dwDataSize /= 2;
				if (dwDataSize && dwDataSize<MAX_PATH)
				{
					path = allocate(dwDataSize, "RegPath");
					memset( path, 0, dwDataSize );
					wcstombs(path, (LPTSTR)lpData, dwDataSize);
#ifdef STDERR
					stderrf("Found %s=%s in reg, len=%d", value, path, dwDataSize);
				}
				else
				{
					stderrf("Failed to read %s path in registry (len=%d)", value, dwDataSize);
#endif
				}
			}
			free(lpData);
			RegCloseKey(hKey);
		}
#ifdef STDERR
		else
		{
			stderrf("Failed to read %s path in registry", value);
		}
#endif
	}

	return path;
}

LPSTR vsPrintf( LPCSTR fmt, ... ) {
	va_list argptr;
	static char text[MAX_VS_SIZE];
	text[MAX_VS_SIZE-1] = '\0';

	va_start (argptr, fmt);
	_vsnprintf (text, MAX_VS_SIZE-1, fmt, argptr);
	va_end (argptr);

	return text;
}

LPSTR hexstring(BYTE *addr, int addrlen)
{
	int i;
	static char buf[MAX_VS_SIZE];
	LPSTR ptr = buf;

	if (addrlen > 0 && addrlen<MAX_VS_SIZE/2)
	{
		for(i=0; i < addrlen ;i++)
		{
			// Take each byte and convert to a hex digit
			_itoa( addr[i] >> 4, ptr, 16);
			ptr++;

			_itoa( addr[i] & 0x0F, ptr, 16);
			ptr++;
		}
	}

	*ptr = '\0';

	return buf;
}
