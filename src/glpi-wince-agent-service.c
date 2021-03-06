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

#include <stdio.h>
#include <windows.h>
#include <winsock.h>
#include <service.h>
#include <winioctl.h>

#include "glpi-wince-agent.h"

/**
 * Defines the entry point for the DLL application.
 */
#include "glpi-wince-agent-service.h"

LPCSTR hdr = APPNAME "-Service (Built: " __DATE__ ", " __TIME__ ")";

BOOL  bServicesStarted = FALSE;
DWORD dwServiceState = SERVICE_STATE_UNINITIALIZED;

// Handle to the worker thread
HANDLE hWorkerThread = NULL;

/**
 * Handles service initialization & deinit
 */
DWORD GWA_Init( DWORD dwContext )
{
	DWORD ret = 0;

#ifdef DEBUG
	SystemDebug("%s: GWA_Init(%d)", hdr, dwContext);
#endif

	/*
	 * Check Context: 1 = Services init as set for super-service in
	 * HKLM,\Software\Services\%AppName%,Context during installation
	 */
	if (dwContext == 1 && dwServiceState == SERVICE_STATE_UNINITIALIZED)
	{
		Log("Starting " APPNAME " service...");
		dwServiceState = SERVICE_STATE_STARTING_UP;
		ret = 1;
	}
	else
	{
		Debug("Unsupported Init Call: Context=%d State=%d", dwContext, dwServiceState);
	}

	return ret;
}

BOOL GWA_Deinit( DWORD dwContext )
{
	BOOL ret = FALSE;

#ifdef DEBUG
	SystemDebug("%s: GWA_Deinit(0x%08x)", hdr, dwContext);
#endif

	if (dwContext == 1)
	{
		dwServiceState = SERVICE_STATE_UNLOADING;
		Log("Stopping " APPNAME " v" VERSION " service...");
		Stop();
#ifdef STDERR
		stderrf(APPNAME " v" VERSION " service stopped");
#endif
		ret = TRUE;
	}
	else
	{
		Debug("Unsupported Deinit Call: Context=%d State=%d", dwContext, dwServiceState);
	}

	return ret;
}

/**
 * Handles IOControl signals
 */
BOOL GWA_IOControl( DWORD dwData, DWORD dwCode, PBYTE pBufIn,
                          DWORD dwLenIn, PBYTE pBufOut, DWORD dwLenOut,
                          PDWORD pdwActualOut )
{
	DWORD dwError = ERROR_INVALID_PARAMETER;

#ifdef DEBUG
	Debug2("Received IOControl: 0x%08x 0x%08x", dwData, dwCode);
#endif

	/*
	 * Inspired from implementation found at
	 * https://msdn.microsoft.com/en-us/library/aa446909.aspx
	 */
	switch(dwCode)
	{
		case IOCTL_SERVICE_CONNECTION:
		{
			SOCKET sock;
			Debug2("Got connection");
			if (dwLenIn != sizeof(SOCKET) || !pBufIn)
			{
				Error("Got bad connection");
				dwError = ERROR_INVALID_PARAMETER;
				break;
			}

			sock = *((SOCKET*)pBufIn);

			if (dwServiceState != SERVICE_STATE_ON)
			{
				closesocket(sock);
				dwError = ERROR_SERVICE_NOT_ACTIVE;
				break;
			}

			Log("TODO: Handle connection request");
			// TODO: Handle the connection
			closesocket(sock);

			if (bServicesStarted)
			{
				// Anyway check if its time to run inventory
				Log("Looking to run inventory...");
				Run(FALSE);
			}

			dwError = ERROR_SUCCESS;
			break;
		}
		case IOCTL_SERVICE_CONTROL:
			Debug2("Got control request");
			break;
		case IOCTL_SERVICE_REGISTER_SOCKADDR:
			Debug2("Got sockaddr register");
			if ((dwServiceState == SERVICE_STATE_STARTING_UP) ||
			    (dwServiceState == SERVICE_STATE_ON))
			{
				dwServiceState = SERVICE_STATE_ON;
				dwError = ERROR_SUCCESS;
				Log(APPNAME " service started");
				Start();
				if (bServicesStarted)
				{
					Log("Looking to run inventory...");
					Run(FALSE);
					// Finally check to resume trigger thread
					Resume();
				}
			}
			else
			{
				Error("Failed to start " APPNAME);
				dwError = ERROR_SERVICE_NOT_ACTIVE;
			}
			break;
		case IOCTL_SERVICE_DEREGISTER_SOCKADDR:
			Debug2("Got sockaddr deregister");
			if ((dwServiceState == SERVICE_STATE_STARTING_UP) ||
			    (dwServiceState == SERVICE_STATE_ON))
			{
				dwError = ERROR_SUCCESS;
				dwServiceState = SERVICE_STATE_STARTING_UP;
				Suspend();
				Log(APPNAME " waiting for listening socket");
			}
			else
			{
				Error("Got unexpected sockaddr deregister, current state: 0x%x", dwServiceState);
				dwError = ERROR_SERVICE_NOT_ACTIVE;
			}
			break;
		case IOCTL_SERVICE_NOTIFY_ADDR_CHANGE:
			Debug2("Got addr change");
			if ((dwServiceState == SERVICE_STATE_STARTING_UP) ||
			    (dwServiceState == SERVICE_STATE_ON))
			{
				if (dwServiceState == SERVICE_STATE_STARTING_UP)
				{
					Start();
					dwServiceState = SERVICE_STATE_ON;
				}
				dwError = ERROR_SUCCESS;
				if (bServicesStarted)
				{
					Log("Looking to run inventory...");
					Run(FALSE);
					// Finally check to resume trigger thread
					Resume();
				}
			}
			else
			{
				Error("Got unexpected sockaddr deregister");
				dwError = ERROR_SERVICE_NOT_ACTIVE;
			}
			break;
		case IOCTL_SERVICE_QUERY_CAN_DEINIT:
			Debug2("Got can deinit request");
			if (dwLenOut != sizeof(DWORD))
			{
				Error("Expected buffer does not have the right size");
			}
			/**
			 * Set service to be unloaded
			 * see https://msdn.microsoft.com/en-us/library/aa450329.aspx
			 */
			Stop();
			dwError = ERROR_SUCCESS;
			*(DWORD *)pBufOut = 1;
			dwServiceState = SERVICE_STATE_UNLOADING;
			break;
		case IOCTL_SERVICE_REFRESH:
			Debug2("Got refresh");
			if (dwServiceState != SERVICE_STATE_ON)
			{
				dwError = ERROR_SERVICE_NOT_ACTIVE;
			}
			else
			{
				Refresh();
				dwError = ERROR_SUCCESS;

				if (bServicesStarted)
				{
					// Also check if its time to run inventory
					Log("Looking to run inventory...");
					Run(FALSE);
				}
			}
			break;
		case IOCTL_SERVICE_START:
			Debug2("Got service start");
			if (dwServiceState != SERVICE_STATE_OFF)
			{
				dwError = ERROR_SERVICE_ALREADY_RUNNING;
			}
			else
			{
				dwServiceState = SERVICE_STATE_ON;
				dwError = ERROR_SUCCESS;
				Start();
			}
			break;
		case IOCTL_SERVICE_STARTED:
			Debug2("Got started");
			if (dwServiceState == SERVICE_STATE_ON ||
			    dwServiceState == SERVICE_STATE_STARTING_UP)
			{
				Log(APPNAME " super-server started");
				dwError = ERROR_SUCCESS;
				bServicesStarted = TRUE;
			}
			else
			{
				Error("Got unexpected super-server started");
				dwError = ERROR_SERVICE_NOT_ACTIVE;
			}
			break;
		case IOCTL_SERVICE_STATUS:
			Debug2("Got status request");
			if (pBufOut != NULL && dwLenOut == sizeof(DWORD))
			{
				*(DWORD *)pBufOut = dwServiceState;
				if (pdwActualOut)
					*pdwActualOut = sizeof(DWORD);
				dwError = ERROR_SUCCESS;
			}
			break;
		case IOCTL_SERVICE_STOP:
			Debug2("Got service stop");
			if (dwServiceState != SERVICE_STATE_ON)
			{
				dwError = ERROR_SERVICE_NOT_ACTIVE;
			}
			else
			{
				bServicesStarted = FALSE;
				dwServiceState = SERVICE_STATE_OFF;
				dwError = ERROR_SUCCESS;
				Stop();
				Log(APPNAME " v" VERSION " service stopped");
			}
			break;
		case IOCTL_SERVICE_UNLOAD:
			Debug2("Got service unload");
			Stop();
			Log(APPNAME " v" VERSION " service stopped to be unloaded");
			dwError = ERROR_SUCCESS;
			break;
		case IOCTL_SERVICE_INSTALL:
			Debug2("Got service install");
			dwError = ERROR_SUCCESS;
			break;
		case IOCTL_SERVICE_UNINSTALL:
			Debug2("Got service uninstall");
			Stop();
			Log(APPNAME " v" VERSION " service stopped to be uninstalled");
			dwError = ERROR_SUCCESS;
			break;
		default:
			Debug2("0x%08x: Got unsupported 0x%08x request", dwData, dwCode);
			break;
	}

	if (dwError != ERROR_SUCCESS)
		SetLastError(dwError);

	return (dwError==ERROR_SUCCESS);
}

/**
 * Handles file requests
 */
BOOL GWA_Open( DWORD dwData, DWORD dwAccess, DWORD dwShareMode )
{
#ifdef DEBUG
	SystemDebug("%s: GWA_Open(0x%08x,0x%08x,0x%08x)", hdr, dwData, dwAccess, dwShareMode);
#endif

	return TRUE;
}

DWORD GWA_Read( DWORD dwData, LPVOID pBuf, DWORD dwLen )
{
#ifdef DEBUG
	SystemDebug("%s: GWA_Read((0x%08x,...,0x%08x)", hdr, dwData, dwLen);
#endif

	return 0;
}

DWORD GWA_Write( DWORD dwData, LPVOID pInBuf, DWORD dwInLen )
{
#ifdef DEBUG
	SystemDebug("%s: GWA_Write(0x%08x,...,0x%08x)", hdr, dwData, dwInLen);
#endif

	if (dwInLen < 16)
	{
		LPSTR buffer = allocate( 16, NULL );
		memset(buffer, 0, 16);
		memcpy(buffer, pInBuf, dwInLen);
		if (lpstrcmp(buffer, "RUN"))
		{
			Debug("Received run request...");
			if (bServicesStarted)
			{
				Run(TRUE);
			}
		}
#ifdef TEST
		else if (lpstrcmp(buffer, "DEBUGRUN"))
		{
			Debug("Received debug run request...");
			RunDebugInventory();
		}
#endif
		else
		{
			DebugError("Received unsupported request: %s", buffer);
		}
		free(buffer);
	}
	else
	{
		Debug("Got unexpected request, len=%d", dwInLen);
	}

	return dwInLen;
}

BOOL GWA_Close( DWORD dwData )
{
#ifdef DEBUG
	SystemDebug("%s: GWA_Close(0x%08x)", hdr, dwData);
#endif

	return TRUE;
}

/**
 * Main entry point
 */
BOOL APIENTRY
DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
#ifdef DEBUG
	SystemDebug("%s: DllMain(dwReason=0x%08x)", hdr, dwReason);
#endif

	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
#ifdef STDERR
			hStdErr = freopen( STDERRFILE, "w", stderr );
			stderrf("DllMain: loading %s library", hdr);
#endif
			if (lpReserved == NULL)
			{
				Debug("%s library dynamically loaded", hdr);
			}
			else
			{
				Debug("%s library statically loaded", hdr);
			}

			if (!DisableThreadLibraryCalls(hModule))
			{
				Error("Failed to disable thread library calls");
			}

			break;
		case DLL_THREAD_ATTACH:
			Debug("%s: thread attached", hdr);
			break;
		case DLL_THREAD_DETACH:
			Debug("%s: thread detached", hdr);
			break;
		case DLL_PROCESS_DETACH:
#ifdef STDERR
			stderrf("%s: DllMain: unloading library", hdr);
			if (lpReserved == NULL)
			{
				stderrf("%s: unloading library", hdr);
			}
			else
			{
				stderrf("%s: unloading library on process termination", hdr);
			}

			if (hStdErr != NULL)
			{
				fclose(hStdErr);
			}
#endif
			break;
	}

	return TRUE;
}
