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
#include <libgen.h>

#include "glpi-wince-agent.h"

LPSTR FileName = NULL;
LPSTR CurrentPath = NULL;
LPSTR DeviceID = NULL;

DWORD dwStartTick = 0;

// local functions
static LPSTR computeDeviceID(void);

void Init(void)
{
	int buflen = 0 ;
	LPWSTR wFileName = NULL;

	dwStartTick = GetTickCount();

	wFileName = allocate( 2*(MAX_PATH+1), "wFileName" );
	if (!GetModuleFileNameW(NULL, wFileName, MAX_PATH))
	{
		MessageBox( NULL, L"Can't get filename", L"Agent Init", MB_OK | MB_ICONINFORMATION );
	}
	buflen = wcslen(wFileName) + 1;

	// Compute filename & currentpath from program path
	FileName = allocate( buflen, "FileName" );
	CurrentPath = allocate( buflen, "CurrentPath" );
	wcstombs(FileName, wFileName, buflen);
	strcpy(CurrentPath, FileName);
	CurrentPath = dirname(CurrentPath);
	free(wFileName);

	LoggerInit(CurrentPath);

	Log("%s started", AgentName);

#ifdef DEBUG
	Debug2("MAX_PATH=%lu", MAX_PATH);
	Debug2("FileName: %s", FileName);
	Debug2("CurrentPath: %hs", CurrentPath);
#endif

	// Storage: use CurrentPath

	// TODO: implement loadState as it can provide still computed DeviceID
	DeviceID = computeDeviceID();
	// TODO: implement saveState so we can keep DeviceID over the time
	Debug2("Current DeviceID=%s", DeviceID);
}

void Run(void)
{
	Log( "Running..." );
}

void Quit(void)
{
	free(FileName);
	free(CurrentPath);
	free(DeviceID);

	Log( "Quitting..." );
	ToolsQuit();
	LoggerQuit();
}

static LPSTR computeDeviceID(void)
{
	int buflen = 21 ;
	LPSTR hostname = NULL;
	LPSYSTEMTIME lpSystemTime = NULL;

	hostname = getHostname();
	buflen += strlen(hostname);

	DeviceID = allocate( buflen, "DeviceID");

	lpSystemTime = getLocalTime();

	if (sprintf( DeviceID, "%s-%02d-%02d-%02d-%02d-%02d-%02d", hostname,
			lpSystemTime->wYear,
			lpSystemTime->wMonth,
			lpSystemTime->wDay,
			lpSystemTime->wHour,
			lpSystemTime->wMinute,
			lpSystemTime->wSecond) > buflen)
	{
		Error("Bad computed DeviceID");
		Abort();
	}

	Debug2("Computed DeviceID=%s", DeviceID);

	return DeviceID;
}
