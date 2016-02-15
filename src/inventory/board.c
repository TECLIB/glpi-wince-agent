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
	LPOSVERSIONINFO lpVersionInformation = NULL;
	LIST *Hardware = NULL;
#ifdef GETDEVICEUNIQUEID
	BYTE rgDeviceId[GETDEVICEUNIQUEID_V1_OUTPUT];
	DWORD cbDeviceId = sizeof(rgDeviceId);
#endif

	lpVersionInformation = allocate( sizeof(OSVERSIONINFO), "OSVERSIONINFO" );
	lpVersionInformation->dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	// Initialize HARDWARE list
	Hardware = createList("HARDWARE");

	// Add fields
	addField( Hardware, "NAME", getHostname() );
	if (GetVersionEx( lpVersionInformation ))
	{
		// CE OS 5.2.1235 (Build 17740.0.2.0)
		if (lpVersionInformation->dwPlatformId != VER_PLATFORM_WIN32_CE)
			Error("Unsupported Platform Id: %lu", lpVersionInformation->dwPlatformId);
		else
			addField( Hardware, "OSNAME", "Windows CE OS" );

		platform = allocate( 64 + strlen((LPSTR)lpVersionInformation->szCSDVersion), "platformid" );
		sprintf( platform, "%lu.%lu.%lu",
			lpVersionInformation->dwMajorVersion,
			lpVersionInformation->dwMinorVersion,
			lpVersionInformation->dwBuildNumber);
		if (strlen((LPSTR)lpVersionInformation->szCSDVersion))
		{
			strcat( platform, " " );
			strcat( platform, (LPSTR)lpVersionInformation->szCSDVersion );
		}
		addField( Hardware, "OSVERSION", platform );
		free(platform);
	}

#ifdef GETDEVICEUNIQUEID
	if ( GetDeviceUniqueID( (LPBYTE)AppName, strlen(AppName),
		GETDEVICEUNIQUEID_V1, rgDeviceId, &cbDeviceId ) != E_INVALIDARG )
	{
		int i = 0;
		LPSTR uuid = NULL, pos = NULL;
		uuid = allocate(sizeof(BYTE)*GETDEVICEUNIQUEID_V1_OUTPUT*2 + 1, NULL);
		for ( i=0, pos = uuid ; i<GETDEVICEUNIQUEID_V1_OUTPUT ; pos += 2, i++ )
			sprintf( pos, "%02x", rgDeviceId[i] );
		addField( Hardware, "UUID", uuid );
		free(uuid);
	}
#endif

	// Insert it in inventory
	InventoryAdd( "HARDWARE", Hardware );

	// clean up
	free(lpVersionInformation);
}
