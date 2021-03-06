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
#ifndef GWA
#include <shlobj.h>
#endif

#include "glpi-wince-agent.h"

LPSTR VarDir = NULL;

void StorageInit(LPCSTR path)
{
	if (VarDir == NULL)
	{
		VarDir = getRegPath( "VarDir" );
#ifdef STDERR
		stderrf( "VarDir='%s'", VarDir );
#endif
	}

#ifndef GWA
	if (VarDir == NULL)
	{
		LPTSTR lpszPath = allocate(2*(MAX_PATH+1), "APPDATA path");
		if (SHGetSpecialFolderPath( NULL, lpszPath, CSIDL_APPDATA, 0))
		{
			int len = wcslen(lpszPath);
			VarDir = allocate(len+strlen(AppName)+2, "VarDir");
			wcstombs(VarDir, lpszPath, len);
			Debug2("Appdata path: %s", VarDir);
			strcat( VarDir, "\\" );
			strcat( VarDir, AppName );
		} else {
			Log("Can't get APPDATA path");
			VarDir = allocate(strlen(path)+strlen(DefaultVarDir)+2, "VarDir");
			sprintf( VarDir, "%s\\%s", path, DefaultVarDir );
		}

		Log("Storage path: %s", VarDir);

		swprintf( lpszPath, L"%hs", VarDir );
		if (!CreateDirectory( lpszPath, NULL ))
		{
			if (GetLastError() != ERROR_FILE_EXISTS)
				DebugError("Failed to create %s folder", VarDir);
		}

		free(lpszPath);
	}
#endif
}

static LPCSTR _getStorageFilename(void)
{
	static char filename[MAX_PATH];

#ifdef GWA
	if (VarDir == NULL)
	{
		StorageInit(NULL);
	}
#endif

	if (VarDir != NULL)
	{
		sprintf( filename, "%s\\%s.dump", VarDir, AppName );
	}
	else
	{
		sprintf( filename, "\\%s.dump", AppName );
	}

#ifdef STDERR
	stderrf( "_getStorageFilename(): %s", filename );
#endif

	return filename;
}

#define STORAGE_MAGIC_TYPE DWORD
#define STORAGE_FILE_MAGIC (STORAGE_MAGIC_TYPE)0xdedaebfe

LPSTR loadState(void)
{
	FILE *hStorage;
	STORAGE_MAGIC_TYPE magic = 0;
	DWORD size = 0;
	LPSTR stateContent = NULL;
	LPSTR deviceid = NULL;

	LPCSTR StorageFile = _getStorageFilename();

#ifdef STDERR
	stderrf( "StorageFile='%s'", StorageFile );
#endif

	hStorage = fopen( StorageFile, "r" );
	if( hStorage == NULL )
	{
		DebugError("Can't read state from %s file", StorageFile);
	}
	else
	{
		// Read state file content
		if (fread( &magic, sizeof(STORAGE_MAGIC_TYPE), 1, hStorage ) != 1)
		{
			Error("Can't read magic bytes from state file");
			fclose(hStorage);
			return NULL;
		}
		// Check magic
		if (magic != STORAGE_FILE_MAGIC)
		{
			Error("Magic bytes do not match in state file");
			fclose(hStorage);
			return NULL;
		}

		// Read header for debugging
		if (fread( &size, sizeof(DWORD), 1, hStorage ) != 1)
		{
			Error("Can't read header size from state file");
			fclose(hStorage);
			return NULL;
		}
		stateContent = allocate(size+1, "StorageFile header");
		if (fread( stateContent, size, 1, hStorage ) != 1)
		{
			Error("Can't read header from state file");
			free(stateContent);
			fclose(hStorage);
			return NULL;
		}
		stateContent[size] = '\0';
		Debug("StorageFile header: %s", stateContent);
		free(stateContent);

		// Read DeviceID
		if (fread( &size, sizeof(DWORD), 1, hStorage ) != 1)
		{
			Error("Can't read deviceid size from state file");
			fclose(hStorage);
			return NULL;
		}

		deviceid = allocate(size+1, "StorageFile deviceid");
		if (fread( deviceid, size, 1, hStorage ) != 1)
		{
			Error("Can't read deviceid from state file");
			free(deviceid);
			fclose(hStorage);
			return NULL ;
		}
		deviceid[size] = '\0';
		Debug("StorageFile deviceid: %s", deviceid);

		if (fread( &magic, sizeof(STORAGE_MAGIC_TYPE), 1, hStorage ) != 1)
		{
			Error("Can't read last magic bytes from state file");
		}
		// Check magic
		if (magic != STORAGE_FILE_MAGIC)
		{
			Error("Last magic bytes do not match in state file");
		}

		// Read maxDelay
		if (fread( &maxDelay, sizeof(maxDelay), 1, hStorage ) != 1)
		{
			Error("Can't read maxDelay from state file");
		}

		// Read nextRunDate
		if (fread( &nextRunDate, sizeof(nextRunDate), 1, hStorage ) != 1)
		{
			Error("Can't read nextRunDate from state file");
		}

		fclose(hStorage);
	}

	return deviceid;
}

void saveState(LPSTR deviceid)
{
	FILE *hStorage;
	STORAGE_MAGIC_TYPE magic = STORAGE_FILE_MAGIC;
	DWORD size = 0;

	LPCSTR StorageFile = _getStorageFilename();

#ifdef STDERR
	stderrf( "StorageFile='%s'", StorageFile );
#endif

	hStorage = fopen( StorageFile, "w" );
	if( hStorage == NULL )
	{
		Error("Can't write state in %s file", StorageFile);
	} else {
		/*
		 * Write state file content:
		 * .1. MAGIC bytes
		 * .2. HEADER size as DWORD
		 * .3. HEADER as string
		 * .4. DEVICEID size as DWORD
		 * .5. DEVICEID as string
		 * .6. MAGIC bytes
		 */
		if (fwrite( &magic, sizeof(STORAGE_MAGIC_TYPE), 1, hStorage ) != 1)
		{
			Error("Can't write magic bytes in state file");
			fclose(hStorage);
			return;
		}

		size = strlen(AgentName);
		if (fwrite( &size, sizeof(DWORD), 1, hStorage ) != 1)
		{
			Error("Can't write header size in state file");
			fclose(hStorage);
			return;
		}
		if (fwrite( AgentName, size, 1, hStorage ) != 1)
		{
			Error("Can't write header in state file");
			fclose(hStorage);
			return;
		}

		// Insert RAW deviceid after putting one DWORD for its size
		size = strlen(deviceid);
		if (fwrite( &size, sizeof(DWORD), 1, hStorage ) != 1)
		{
			Error("Can't write deviceid size in state file");
			fclose(hStorage);
			return;
		}
		if (fwrite( deviceid, size, 1, hStorage ) != 1)
		{
			Error("Can't write deviceid in state file");
			fclose(hStorage);
			return;
		}

		if (fwrite( &magic, sizeof(STORAGE_MAGIC_TYPE), 1, hStorage ) != 1)
		{
			Error("Can't write last magic bytes in state file");
		}

		// Insert maxDelay & nextRunDate
		if (fwrite( &maxDelay, sizeof(maxDelay), 1, hStorage ) != 1)
		{
			Error("Can't write maxDelay in state file");
			fclose(hStorage);
			return;
		}
		if (fwrite( &nextRunDate, sizeof(nextRunDate), 1, hStorage ) != 1)
		{
			Error("Can't write nextRunDate in state file");
			fclose(hStorage);
			return;
		}

		fclose(hStorage);
	}
}

void StorageQuit(void)
{
	Debug2("Freeing Storage");
	free(VarDir);

	VarDir = NULL;
}
