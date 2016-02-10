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
#include <shlobj.h>

#include "glpi-wince-agent.h"

LPSTR VarDir = NULL;

void StorageInit(LPCSTR path)
{
	LPTSTR lpszPath = NULL;

	lpszPath = allocate(2*(MAX_PATH+1), "APPDATA path");
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
			Error("Failed to create %s folder", VarDir);
	}

	free(lpszPath);
}



void StorageQuit(void)
{
	free(VarDir);
}
