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

#include "../glpi-wince-agent.h"

#define DEFAULT_BUFSIZE 64

void getBios(void)
{
	UINT buflen = DEFAULT_BUFSIZE;
	LIST *Bios = NULL;
	LPWSTR wInfo = NULL;
	LPSTR Info = NULL;
	HINSTANCE hOEMDll = NULL;

	// Initialize HARDWARE list
	Bios = createList("BIOS");

	// Get OEM Info
	wInfo = allocate( buflen, "GetOemInfo" );
	while (wInfo != NULL && !SystemParametersInfo(SPI_GETOEMINFO, buflen, wInfo, 0))
	{
		free(wInfo);
		if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
			Error("Can't get OEM information");
		else if (buflen>=1024*1024)
			Error("Too big OEM information, aborting");
		else
			wInfo = allocate( buflen *= 2, "GetOemInfo" );
	}

	if (wInfo != NULL)
	{
		// Convert string
		buflen = wcslen(wInfo) + 1 ;
		Info = allocate(buflen, "Converted GetOemInfo");
		wcstombs(Info, wInfo, buflen);
		free(wInfo);

		// Add as system manufacturer
		addField( Bios, "SMANUFACTURER", Info );
		free(Info);
	}

	// Get Platform Type
	buflen = DEFAULT_BUFSIZE;
	wInfo = allocate( buflen, "GetPlatformType" );
	while (!SystemParametersInfo(SPI_GETPLATFORMTYPE, buflen, wInfo, 0))
	{
		free(wInfo);
		if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
			Error("Can't get Platform Type");
		else if (buflen>=1024*1024)
			Error("Too big Platform Type, aborting");
		else
			wInfo = allocate( buflen *= 2, "GetPlatformType" );
	}

	if (wInfo != NULL)
	{
		// Convert string
		buflen = wcslen(wInfo) + 1 ;
		Info = allocate(buflen, "Converted GetPlatformType");
		wcstombs(Info, wInfo, buflen);
		free(wInfo);

		// Add as system model
		addField( Bios, "SMODEL", Info );
		free(Info);
	}

	// Get DataLogic SerialNumber if available
	hOEMDll = LoadLibrary(L"DLCEDevice.dll");
	if (hOEMDll != NULL)
	{
		FARPROC DLDEVICE_GetSerialNumber = NULL;

		Debug2("Loading DataLogic GetSerialNumber API...");
		DLDEVICE_GetSerialNumber = GetProcAddress( hOEMDll, L"DLDEVICE_GetSerialNumber" );
		if (DLDEVICE_GetSerialNumber == NULL)
			DebugError("Can't import DataLogic GetSerialNumber() API");
		else
		{
			buflen = DLDEVICE_GetSerialNumber( wInfo, 0 );
			if (buflen>0)
			{
				wInfo = allocate( 2*(buflen+1), "DataLogic SerialNumber" );
				DLDEVICE_GetSerialNumber( wInfo, buflen );
				buflen = wcslen(wInfo) ;
				if (buflen)
				{
					Info = allocate( buflen+1, "DataLogic SerialNumber");
					wcstombs(Info, wInfo, buflen);
					addField( Bios, "SSN", Info );
					free(Info);
				}
				free(wInfo);
			}
			DLDEVICE_GetSerialNumber = NULL;
		}

		// Free DLL
		FreeLibrary(hOEMDll);
	}
	else
		DebugError("Can't load DLCEDevice.dll");

	// Insert Bios in inventory
	InventoryAdd( "BIOS", Bios );

}

void getHardware(void)
{
	LPSTR platform = NULL;
	OSVERSIONINFO VersionInformation ;
	LIST *Hardware = NULL;
	HINSTANCE hCoreDll = NULL;
	FARPROC GetDeviceUniqueID = NULL;

	BYTE rgDeviceId[GETDEVICEUNIQUEID_V1_OUTPUT];
	DWORD cbDeviceId = sizeof(rgDeviceId);

	hCoreDll = LoadLibrary(L"coredll.dll");
	if (hCoreDll != NULL)
	{
		Debug2("Loading GetDeviceUniqueID API...");
		GetDeviceUniqueID = GetProcAddress( hCoreDll, L"GetDeviceUniqueID" );
		if (GetDeviceUniqueID == NULL)
		{
			DebugError("Can't import GetDeviceUniqueID() API");
			FreeLibrary(hCoreDll);
		}
	}
	else
		DebugError("Can't load coredll.dll");

	// Initialize HARDWARE
	Hardware = createList("HARDWARE");

	// Add fields
	addField( Hardware, "NAME", getHostname() );
	if (GetVersionEx( &VersionInformation ))
	{
		// CE OS 5.2.1235 (Build 17740.0.2.0)
		if (VersionInformation.dwPlatformId != VER_PLATFORM_WIN32_CE)
			Error("Unsupported Platform Id: %lu", VersionInformation.dwPlatformId);
		else
			addField( Hardware, "OSNAME", "Windows CE OS" );

		platform = allocate( 64 + strlen((LPSTR)VersionInformation.szCSDVersion), "platformid" );
		sprintf( platform, "%lu.%lu.%lu",
			VersionInformation.dwMajorVersion,
			VersionInformation.dwMinorVersion,
			VersionInformation.dwBuildNumber);
		if (strlen((LPSTR)VersionInformation.szCSDVersion))
		{
			strcat( platform, " " );
			strcat( platform, (LPSTR)VersionInformation.szCSDVersion );
		}
		addField( Hardware, "OSVERSION", platform );
		free(platform);
	}

	if ( GetDeviceUniqueID != NULL && GetDeviceUniqueID( (LPBYTE)AppName, strlen(AppName),
		GETDEVICEUNIQUEID_V1, rgDeviceId, &cbDeviceId ) != E_INVALIDARG )
	{
		int i = 0;
		LPSTR uuid = NULL, pos = NULL;
		Debug("Computing UUID...");
		uuid = allocate(sizeof(BYTE)*GETDEVICEUNIQUEID_V1_OUTPUT*2 + 1, NULL);
		for ( i=0, pos = uuid ; i<GETDEVICEUNIQUEID_V1_OUTPUT ; pos += 2, i++ )
			sprintf( pos, "%02x", rgDeviceId[i] );
		addField( Hardware, "UUID", uuid );
		free(uuid);
		GetDeviceUniqueID = NULL;
		FreeLibrary(hCoreDll);
	}

	// Insert it in inventory
	InventoryAdd( "HARDWARE", Hardware );
}
