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

// Direct import from SDK file: getdeviceuniqueid.h
#define GETDEVICEUNIQUEID_V1                1
#define GETDEVICEUNIQUEID_V1_MIN_APPDATA    8
#define GETDEVICEUNIQUEID_V1_OUTPUT         20

WINBASEAPI HRESULT WINAPI GetDeviceUniqueID(LPBYTE,DWORD,DWORD,LPBYTE,DWORD);

void getBios(void)
{
	LIST *Bios = NULL;

	// Initialize HARDWARE list
	Bios = createList("BIOS");

	// Add fields

	// Get serialnumber
	addField( Bios, "SKUNUMBER", "Not implemented" );

	// Insert it in inventory
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
			DebugError("Can't import GetDeviceUniqueID() API");
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
	}

	// Insert it in inventory
	InventoryAdd( "HARDWARE", Hardware );
}
