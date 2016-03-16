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
#include <winioctl.h>

#include "../glpi-wince-agent.h"

#define DEFAULT_BUFSIZE 64

enum supported_manufacturer {
	Man_nosupport = 0,
	Man_HTC,
	Man_DataLogic,
	Man_Motorola,
	Man_Microsoft,
	Man_END
};

MANUFACTURER Manufacturers[] = {
	{	Man_nosupport,	"Not supported" },
	{	Man_HTC,		"HTC" 			},
	{	Man_DataLogic,	"Datalogic"		},
	{	Man_Motorola,	"Motorola"		},
	{	Man_Microsoft,	"Microsoft"		},
	{	Man_END,		NULL			},
};

void getBios(void)
{
	UINT buflen = DEFAULT_BUFSIZE;
	LIST *Bios = NULL;
	LPWSTR wInfo = NULL;
	LPSTR Info = NULL, sOEMInfo = NULL;
	HINSTANCE hOEMDll = NULL;
	LPMANUFACTURER manufacturer = &Manufacturers[Man_nosupport];

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
		sOEMInfo = allocate(buflen, "Converted GetOemInfo");
		wcstombs(sOEMInfo, wInfo, buflen);
		free(wInfo);

		// Find sOEMInfo first word length to munafacturer name check
		Info = strchr( sOEMInfo, ' ');
		buflen = Info - sOEMInfo;

		// Search if this manufacturer is known
		while (++manufacturer<&Manufacturers[Man_END])
		{
#ifdef DEBUG
			Debug2("Looking for %s in %s...", manufacturer->name, sOEMInfo);
#endif
			if (strstr(sOEMInfo, manufacturer->name) != NULL)
				break;

			// Also check first word can match but in case insensitive mode
			if (strlen(manufacturer->name) == buflen &&
					_strnicmp(sOEMInfo, manufacturer->name, buflen) == 0)
				break;
		}
		if (manufacturer->id == Man_END)
		{
			Log("No supported manufacturer found in OEMInfo: %s", sOEMInfo);
			manufacturer = &Manufacturers[Man_nosupport];
		}

		// Add as system manufacturer
		addField( Bios, "SMANUFACTURER", (LPSTR)manufacturer->name );
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

		// Reformat when platform type not appears in OEMInfo
		if (strstr(sOEMInfo, Info) == NULL && strstr(sOEMInfo, manufacturer->name) == sOEMInfo)
		{
			LPSTR oeminfo = &sOEMInfo[strlen(manufacturer->name)+1];
			// Add oeminfo + platformtype as system model
			addField( Bios, "SMODEL", vsPrintf("%s %s", oeminfo, Info));
		}
		else
		{
			// Add as system model
			addField( Bios, "SMODEL", Info );
		}
		free(Info);
	}

	free(sOEMInfo);

	// Check possibly interesting structure
	CheckDeviceId(manufacturer, Bios);

	// Get DataLogic SerialNumber if available
	hOEMDll = LoadLibrary(wDATALOGIC_DLL);
	if (hOEMDll != NULL)
	{
		FARPROC DeviceGetSerialNumber = NULL;

		Debug2("Loading DataLogic GetSerialNumber API...");
		DeviceGetSerialNumber = GetProcAddress( hOEMDll, L"DeviceGetSerialNumber" );
		if (DeviceGetSerialNumber == NULL)
			DebugError("Can't import DataLogic GetSerialNumber() API");
		else
		{
			buflen = DATALOGIC_SERIAL_NUMBER_SIZE + 1 ;
			wInfo = allocate( 2*buflen, "DataLogic SerialNumber" );
			DeviceGetSerialNumber( wInfo, DATALOGIC_SERIAL_NUMBER_SIZE );
			buflen = wcslen(wInfo) ;
			if (buflen)
			{
				Info = allocate( buflen+1, "DataLogic SerialNumber");
				wcstombs(Info, wInfo, buflen);
				addField( Bios, "SSN", Info );
				free(Info);
			}
			free(wInfo);

			DeviceGetSerialNumber = NULL;
		}

		// Free DLL
		FreeLibrary(hOEMDll);
	}
	else
		DebugError("Can't load "sDATALOGIC_DLL" DLL");

	// Get Motorola SerialNumber if available
	hOEMDll = LoadLibrary(wMOTOROLA_DLL);
	if (hOEMDll != NULL)
	{
		FARPROC RCM_GetESN = NULL;

		Debug2("Loading Motorola GetSerialNumber API...");
		RCM_GetESN = GetProcAddress( hOEMDll, L"RCM_GetESN" );
		if (RCM_GetESN == NULL)
			DebugError("Can't import Motorola GetESN() API");
		else
		{
			DWORD ret;
			ELECTRONIC_SERIAL_NUMBER eSN;
			memset(&eSN, 0, sizeof(eSN));
			SI_INIT(&eSN);
			ret = RCM_GetESN( &eSN );
			if (ret != E_RCM_SUCCESS)
			{
				DebugError("Motorola GetESN() API returned: %x", ret);
			}
			else
			{
				buflen = wcslen(eSN.wszESN);
				if (buflen)
				{
					Info = allocate( buflen+1, "Motorola SerialNumber");
					wcstombs(Info, eSN.wszESN, buflen+1);
					addField( Bios, "SSN", Info );
					free(Info);
				}
				Debug("ESN Infos: unused ESN slots: %d ; Error flag: %x", eSN.dwAvail, eSN.dwError);
			}
			RCM_GetESN = NULL;
		}

		// Free DLL
		FreeLibrary(hOEMDll);
	}
	else
		DebugError("Can't load "sMOTOROLA_DLL" DLL");

	// Add a bios type to help FI4GLPI proposing a device type
	addField( Bios, "TYPE", "Embedded Terminal" );

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

void CheckDeviceId(LPMANUFACTURER manufacturer, LIST *list)
{
	PULONG pTerminalID, pPlatformID;
	BOOL bGet ;
	LPSTR szPlatformID = NULL;
	DWORD dwOutSizeOf = 0, size = 0;

	DEVICE_ID DeviceID, *pDeviceID ;
	DeviceID.dwSize = 0;

	if (!KernelIoControl(IOCTL_HAL_GET_DEVICEID, NULL, 0, &DeviceID, 0, NULL))
	{
		if (!DeviceID.dwSize)
		{
			DebugError("Failed to retreive DeviceID");
			return;
		}
	}
	else
	{
		DebugError("Unexpected response while requesting DeviceID structure size");
		return;
	}

	pDeviceID = allocate( DeviceID.dwSize, "Device ID" );
	pDeviceID->dwSize = DeviceID.dwSize;

	bGet = KernelIoControl(IOCTL_HAL_GET_DEVICEID, NULL, 0, pDeviceID, pDeviceID->dwSize, &dwOutSizeOf);

	if ( bGet && dwOutSizeOf > 0 )
	{
#ifdef DEBUG
		RawDebug("DeviceID struct: %s", (LPBYTE)pDeviceID, DeviceID.dwSize);
#endif

		if ( pDeviceID->dwPresetIDBytes > 0 )
		{
			Debug2("Size of Terminal ID: %d", pDeviceID->dwPresetIDBytes);
			pTerminalID = (PULONG)((BYTE *)pDeviceID + pDeviceID->dwPresetIDOffset);
			Debug2("Found TerminalID: %lu", *pTerminalID);
#ifdef DEBUG
			RawDebug("Raw TerminalID: %s", (LPBYTE)pTerminalID, pDeviceID->dwPresetIDBytes);
#endif
		}
		else
		{
			Log("No TerminalID available");
		}

		if ( pDeviceID->dwPlatformIDBytes > 0 )
		{
			Debug2("Size of PlatformID: %d", pDeviceID->dwPlatformIDBytes);
			size = pDeviceID->dwPlatformIDBytes / sizeof(WCHAR);
			szPlatformID = allocate(size+1, "PlatformID");

			if ( szPlatformID != NULL )
			{
				pPlatformID = (PULONG)((LPBYTE)pDeviceID + pDeviceID->dwPlatformIDOffset);
				wcstombs(szPlatformID, (LPWSTR)pPlatformID, size);
				Debug("Found PlatformID: %s", szPlatformID);
				free(szPlatformID);
#ifdef DEBUG
				RawDebug("Raw PlatformID: %s", (LPBYTE)pPlatformID, pDeviceID->dwPlatformIDBytes);
#endif
			}
		}
		else
		{
			Log("No PlatformID available");
		}

		// HTC Touch Diamond IMEI
		if (manufacturer->id == Man_HTC)
		{
			LPSTR sIMEI = allocate(16, NULL);
			*sIMEI = '\0';
			strcat(sIMEI, hexstring(((LPBYTE)pDeviceID)+28, 2));
			strcat(sIMEI, hexstring(((LPBYTE)pDeviceID)+32, 6));
			// Last char could be skipped
			sIMEI[strlen(sIMEI)-1] = '\0';
			Debug2("Found HTC IMEI: %s", sIMEI);
			addField( list, "IMEI", sIMEI );
		}
	}
	else
	{
		DebugError("Error with KernelIOControl: %d bytes returned", dwOutSizeOf);
	}

	free(pDeviceID);
}
