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
#include <service.h>
#include <winioctl.h>
#include <libgen.h>

#include "glpi-wince-agent.h"

LPSTR FileName = NULL;
LPSTR CurrentPath = NULL;
LPSTR DeviceID = NULL;
BOOL bInitialized = FALSE;
BOOL bSuspended = FALSE;
DWORD maxDelay = DEFAULT_MAX_DELAY ;
FILETIME nextRunDate = { 0, 0 };
#ifdef GWA
HANDLE ThreadTrigger = NULL;
DWORD ServiceId = 0;
DWORD TriggerId = 0;

// Only one thread can try to run an inventory at a time
CRITICAL_SECTION InventoryThreadRunning;
#endif

// local functions
static LPSTR computeDeviceID(void);

void Init(void)
{
	int buflen = 0 ;
	LPWSTR wFileName = NULL;

	ToolsInit();

	wFileName = allocate( 2*(MAX_PATH+1), "wFileName" );
	if (!GetModuleFileNameW(NULL, wFileName, MAX_PATH))
	{
#ifdef GWA
		fprintf(stderr, APPNAME ": Can't get own filename");
#else
		MessageBox( NULL, L"Can't get filename", L"Agent Init", MB_OK | MB_ICONINFORMATION );
#endif
	}
	buflen = wcslen(wFileName) + 1;

	// Compute filename & currentpath from program path
	FileName = allocate( buflen, "FileName" );
	CurrentPath = allocate( buflen, "CurrentPath" );
	wcstombs(FileName, wFileName, buflen);
	strcpy(CurrentPath, FileName);
	CurrentPath = dirname(CurrentPath);
	free(wFileName);

	LoggerInit();

	Log("%s started", AgentName);

#ifdef DEBUG
	Debug2("MAX_PATH=%lu", MAX_PATH);
	Debug2("FileName: %s", FileName);
	Debug2("CurrentPath: %hs", CurrentPath);
#endif

	/*
	 *  Storage path: APPDATA\AppName or CurrentPath\var
	 */
	StorageInit(CurrentPath);

	// Load config as StorageInit() has now initialized VarDir
	conf = ConfigLoad(VarDir);

	// Save returned default config if not loaded from config file
	if (!conf.loaded)
	{
		Debug("Saving default config");
		ConfigSave();
	}

	// Keep DeviceID consistent over the time loading previous state
	DeviceID = loadState();

	if (DeviceID == NULL)
		DeviceID = computeDeviceID();
	Debug2("Current DeviceID=%s", DeviceID);

	// Keep DeviceID consistent over the time saving now current state
	saveState(DeviceID);

	bInitialized = TRUE;
}

#ifdef GWA
static DWORD WINAPI _lpStartTriggerThread(LPVOID lpParameter)
{
	while (1) {
		// Sleep at least one second before try to run
#ifdef DEBUG
		Debug2("Sleeping %d ms", DEFAULT_MINIMAL_SLEEP);
#endif
		Sleep(DEFAULT_MINIMAL_SLEEP);

		Run(FALSE);
	}

	return 0;
}
#endif

void Start(void)
{
	if (bInitialized)
		Quit();
	Init();

#ifdef GWA
	// Service is activated on services event but we need to
	// Start a thread to trigger Run() at regulary time
	if (!ThreadTrigger) {
		// Initialize critical section
		InitializeCriticalSection(&InventoryThreadRunning);

		ServiceId = GetCurrentThreadId();
#ifdef DEBUG
		Debug( "Starting Trigger thread (tid=%d)", ServiceId );
#endif

		ThreadTrigger = CreateThread( NULL, 0,
			_lpStartTriggerThread, &ServiceId, 0, &TriggerId);
		//CeSetThreadPriority( ThreadTrigger, THREAD_PRIORITY_IDLE );
#ifdef DEBUG
		Debug( "Trigger thread started (tid=%d)", TriggerId );
#endif
		bSuspended = FALSE;
	}
#endif
}

#ifdef GWA
void Run(BOOL force)
{
	// Don't run if a thread is still running an inventory
	if (!TryEnterCriticalSection(&InventoryThreadRunning))
	{
		Debug2( "Skipping inventory run, another thread should be running" );
		return;
	}

	// While not forced, check if it's time to submit
	if (force || timeToSubmit())
	{
		// Write local inventory if desired
		if (conf.local != NULL && strlen(conf.local))
		{
			Log( "Running inventory..." );
			RunInventory();

			TargetInit(DeviceID);

			Log( "Saving to file as local target..." );
			WriteLocal(DeviceID);

			// Always set new run date if requesting local inventory
			computeNextRunDate();
		}

		// Send inventory if desired
		if (conf.server != NULL && strlen(conf.server))
		{
			// Only run inventory if still not done, as local inventory
			if (getInventory() == NULL)
			{
				Log( "Running inventory..." );
				RunInventory();

				TargetInit(DeviceID);
			}

			Log( "Submitting to remote target..." );
			if (SendRemote(DeviceID))
			{
				// But only reset next run date while submission failed
				computeNextRunDate();
			}
		}

		TargetQuit();
		FreeInventory();

		Log( "Run finished" );
	}

	LeaveCriticalSection(&InventoryThreadRunning);
}
#else
void RequestRun(void)
{
	HANDLE hService = NULL;

	Log( "Requesting inventory..." );

	Log( "Trying to access the service...");
	hService = CreateFile(L"GWA0:",GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
	if (hService != INVALID_HANDLE_VALUE)
	{
		LPCSTR cmd = "RUN";
		DWORD written = 0;
		if (WriteFile(hService, cmd, strlen(cmd), &written, NULL))
		{
			if (written == strlen(cmd))
			{
				Log("Request transmitted");
			}
			else
			{
				Error("Failed to fully transmit request");
			}
		}
		else
		{
			Error("Failed to transmit request");
		}
		CloseHandle(hService);
	}
	else if (GetLastError() == ERROR_DEV_NOT_EXIST)
	{
		Error(APPNAME " service disabled");
	}
	else
	{
		Error("Failed to access " APPNAME " service");
	}
}
#endif

#ifdef GWA
void Resume(void)
{
	if (bSuspended && ThreadTrigger) {
		ResumeThread(ThreadTrigger);
		bSuspended = FALSE;
	}
}

void Suspend(void)
{
	if (!bSuspended && ThreadTrigger) {
		SuspendThread(ThreadTrigger);
		bSuspended = TRUE;
	}
}
#endif

void Stop(void)
{
#ifdef GWA
	if (ThreadTrigger) {
		Debug( "Stopping trigger thread.." );
		TerminateThread(ThreadTrigger, 0);
		DeleteCriticalSection(&InventoryThreadRunning);
		Debug( "Trigger thread stopped" );
		ThreadTrigger = NULL;
	}
#endif

	Quit();
}

void Refresh(void)
{
#ifdef GWA
	ConfigQuit();
	conf = ConfigLoad(VarDir);
#else
	HANDLE hService = NULL;

	Log( "Requesting config refresh..." );

	Log( "Trying to access the service...");
	hService = CreateFile(L"GWA0:",GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
	if (hService != INVALID_HANDLE_VALUE)
	{
		DWORD IoControl = IOCTL_SERVICE_REFRESH;
		DWORD returned = 0;
		if (DeviceIoControl(hService, IoControl, NULL, 0, NULL, 0, &returned, NULL))
		{
			Log("Config refresh requested");
		}
		else
		{
			Error("Failed to transmit refresh request");
		}
		CloseHandle(hService);
	}
	else if (GetLastError() == ERROR_DEV_NOT_EXIST)
	{
		Error(APPNAME " service disabled");
	}
	else
	{
		Error("Failed to access " APPNAME " service");
	}
#endif
}

void Quit(void)
{
#ifdef TEST
	Log( "Quitting..." );
#endif
	ConfigQuit();
	StorageQuit();
	ToolsQuit();
	LoggerQuit();

#ifdef STDERR
	stderrf("All QUIT() API called");
#endif

	free(FileName);
	free(CurrentPath);
	free(DeviceID);

	FileName = NULL;
	CurrentPath = NULL;
	DeviceID = NULL;

#ifdef STDERR
	stderrf("Ressources freed");
#endif

	bInitialized = FALSE;
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
#ifndef GWA
		Abort();
#endif
	}

	Debug2("Computed DeviceID=%s", DeviceID);

	return DeviceID;
}

LPSTR getCurrentPath(void)
{
	return CurrentPath;
}

void RunDebugInventory(void)
{
#ifdef GWA
#ifdef TEST
	Log( "Running inventory..." );
	RunInventory();

	DebugInventory();
#endif
#else
	HANDLE hService = NULL;

	Log( "Requesting debug inventory..." );

	Log( "Trying to access the service...");
	hService = CreateFile(L"GWA0:",GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
	if (hService != INVALID_HANDLE_VALUE)
	{
		LPCSTR cmd = "DEBUGRUN";
		DWORD written = 0;
		if (WriteFile(hService, cmd, strlen(cmd), &written, NULL))
		{
			if (written == strlen(cmd))
			{
				Log("Request transmitted");
			}
			else
			{
				Error("Failed to fully transmit request");
			}
		}
		else
		{
			Error("Failed to transmit request");
		}
		CloseHandle(hService);
	}
	else if (GetLastError() == ERROR_DEV_NOT_EXIST)
	{
		Error(APPNAME " service disabled");
	}
	else
	{
		Error("Failed to access " APPNAME " service");
	}
#endif
}
