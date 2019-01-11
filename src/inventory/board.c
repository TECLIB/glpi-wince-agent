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
	Man_Symbol,
	Man_END
};

MANUFACTURER Manufacturers[] = {
	{	Man_nosupport,	"Not supported" },
	{	Man_HTC,		"HTC" 			},
	{	Man_DataLogic,	"Datalogic"		},
	{	Man_Motorola,	"Motorola"		},
	{	Man_Microsoft,	"Microsoft"		},
	{	Man_Symbol,		"SYMBOL"		},
	{	Man_END,		NULL			},
};

void getBios(void)
{
	UINT buflen = DEFAULT_BUFSIZE;
	LIST *Bios = NULL;
	LPWSTR wInfo = NULL, wVersion = NULL;
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

		// Find sOEMInfo first word length to manufacturer name check
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
		FARPROC DeviceGetVersionInfo  = NULL;

		Debug2("Loading DataLogic GetSerialNumber API...");
		DeviceGetSerialNumber = GetProcAddress( hOEMDll, L"DeviceGetSerialNumber" );
		if (DeviceGetSerialNumber == NULL)
			DebugError("Can't import DataLogic GetSerialNumber() API");
		else
		{
			buflen = SERIAL_NUMBER_SIZE + 1 ;
			wInfo = allocate( 2*buflen, "DataLogic SerialNumber" );
			memset(wInfo, 0, 2*buflen);
			if (DeviceGetSerialNumber( wInfo, SERIAL_NUMBER_SIZE ))
			{
				Info = allocate( buflen+1, "DataLogic SerialNumber");
				memset(Info, 0, buflen+1);
				wcstombs(Info, wInfo, buflen);
				if (strlen(Info))
				{
					addField( Bios, "SSN", Info );
				}
				else
				{
					Debug("Got empty DataLogic SerialNumber");
				}
				free(Info);
			}
			free(wInfo);

			DeviceGetSerialNumber = NULL;
		}

		Debug2("Loading DataLogic DeviceGetVersionInfo API...");
		DeviceGetVersionInfo = GetProcAddress( hOEMDll, L"DeviceGetVersionInfo" );
		if (DeviceGetVersionInfo == NULL)
			DebugError("Can't import DataLogic DeviceGetVersionInfo() API");
		else
		{
			buflen = VERSION_LABEL_SIZE + 1 ;
			wInfo = allocate( 2*buflen, "DataLogic Firmware Label" );
			memset(wInfo, 0, 2*buflen);
			buflen = VERSION_SIZE + 1 ;
			wVersion = allocate( 2*buflen, "DataLogic Firmware Version" );
			memset(wVersion, 0, 2*buflen);
			if (DeviceGetVersionInfo( VERSION_INDEX_FIRMWARE,
				  wInfo, VERSION_LABEL_SIZE, wVersion, VERSION_SIZE ))
			{
				// Retrieve version label
				Info = allocate( VERSION_LABEL_SIZE+1, "DataLogic Firmware Label");
				memset(Info, 0, VERSION_LABEL_SIZE+1);
				wcstombs(Info, wInfo, VERSION_LABEL_SIZE);

				// Retrieve version number
				sOEMInfo = allocate( VERSION_SIZE+1, "DataLogic Firmware Label");
				memset(sOEMInfo, 0, VERSION_SIZE+1);
				wcstombs(sOEMInfo, wVersion, VERSION_SIZE);

				if (strlen(Info))
				{
					if (strlen(sOEMInfo))
					{
						addField(Bios, "BVERSION", vsPrintf("%s %s", Info, sOEMInfo));
					}
					else
					{
						Debug("Got empty DataLogic Version for Number");
						addField(Bios, "BVERSION", Info);
					}
				}
				else
				{
					if (strlen(sOEMInfo))
					{
						Debug("Got empty DataLogic Version for Label");
						addField(Bios, "BVERSION", sOEMInfo);
					}
					else
					{
						Debug("Got empty DataLogic Version for Label & Number");
					}
				}
				free(Info);
				free(sOEMInfo);
			}
			free(wInfo);
			free(wVersion);

			DeviceGetVersionInfo = NULL;
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

LPSTR getUUID(BOOL tryDeprecated)
{
	int i = 0, uuidlen = sizeof(UUID);
	LPSTR pos = NULL;
	BYTE uuid[DEFAULT_BUFSIZE/2];
	static char strUUID[DEFAULT_BUFSIZE];

	strUUID[0] = '\0';

	if (!SystemParametersInfo(SPI_GETUUID, sizeof(UUID), uuid, 0))
	{
		HINSTANCE hCoreDll = NULL;
		uuidlen = 0;

		if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
		{
			DebugError("getUUID: Can't get Platform UUID via SystemParametersInfo (err=%d)", GetLastError());
		}
		else
		{
			DebugError("getUUID: Insufficient buffer size for SystemParametersInfo");
		}

		hCoreDll = LoadLibrary(L"coredll.dll");
		if (hCoreDll != NULL)
		{
			FARPROC GetDeviceUniqueID = NULL;
			Debug2("Loading GetDeviceUniqueID API...");
			GetDeviceUniqueID = GetProcAddress( hCoreDll, L"GetDeviceUniqueID" );
			if (GetDeviceUniqueID == NULL)
			{
				DebugError("Can't import GetDeviceUniqueID() API (err=%d)", GetLastError());
			}
			else
			{
				uuidlen = GETDEVICEUNIQUEID_V1_OUTPUT;
				if (GetDeviceUniqueID( (LPBYTE)AppName, strlen(AppName), GETDEVICEUNIQUEID_V1, uuid, &uuidlen ) == E_INVALIDARG )
				{
					uuidlen = 0;
					DebugError("Failed to use GetDeviceUniqueID");
				}
			}
			FreeLibrary(hCoreDll);
		}
		else
		{
			DebugError("Can't load coredll.dll");
		}

		if (!uuidlen)
		{
			// Get Motorola UniqueUnitId if available
			HINSTANCE hOEMDll = LoadLibrary(wMOTOROLA_DLL);
			if (hOEMDll != NULL)
			{
				FARPROC RCM_GetUniqueUnitId = NULL;

				Debug2("Loading Motorola GetUniqueUnitId API...");
				RCM_GetUniqueUnitId = GetProcAddress( hOEMDll, L"RCM_GetUniqueUnitId" );
				if (RCM_GetUniqueUnitId == NULL)
					DebugError("Can't import Motorola GetUniqueUnitId() API");
				else
				{
					DWORD ret;
					ret = RCM_GetUniqueUnitId( (LPUNITID)&uuid );
					if (ret != E_RCM_SUCCESS)
					{
						DebugError("Motorola GetUniqueUnitId() API returned: %x", ret);
					}
					else
					{
						uuidlen = sizeof(UNITID);
					}
					RCM_GetUniqueUnitId = NULL;
				}

				// Free DLL
				FreeLibrary(hOEMDll);
			}
			else
				DebugError("Can't load "sMOTOROLA_DLL" DLL");
		}

		// Eventually try deprecated method
		if (!uuidlen)
		{
			if (KernelIoControl(IOCTL_HAL_GET_UUID, NULL, 0, &uuid, sizeof(UUID), NULL))
			{
				uuidlen = sizeof(UUID);
			}
			else
			{
				DebugError("Deprecated UUID IOCL call failed (%d)", GetLastError());
			}
		}
	}

	if (!uuidlen)
		return NULL;

	Debug2("Computing Device UUID... (size=%d)", uuidlen);

	for ( i=0, pos = strUUID ; i<uuidlen ; pos += 2, i++ )
		sprintf( pos, "%02x", uuid[i] );

	Debug2("UUID: %s", strUUID);

	return strUUID;
}

void getHardware(void)
{
	LPSTR platform = NULL;
	OSVERSIONINFO VersionInformation ;
	LIST *Hardware = NULL;
	LPSTR Name = NULL, uuid = NULL;

	// Initialize HARDWARE
	Hardware = createList("HARDWARE");

	// We can search name first as TerminalID
	if (getRegValue(HKEY_LOCAL_MACHINE,
	                "\\Software\\"EDITOR"\\"APPNAME, "SearchTerminalID"))
	{
		// Search name first as Avalanche TerminalID
		Name = getRegString( HKEY_LOCAL_MACHINE,
			"\\Software\\Wavelink\\Avalanche\\State",
			"TerminalID");
	}
	// Finally set name from terminal hostname as default
	if (Name == NULL)
		Name = getHostname();

	// Add fields
	addField( Hardware, "NAME", Name );
	VersionInformation.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
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

	uuid = getUUID(FALSE);
	if (uuid)
	{
		addField( Hardware, "UUID", uuid );
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
			DebugError("Can't retrieve Device_ID structure");
			return;
		}
	}
	else
	{
		DebugError("Unexpected response while requesting Device_ID structure size");
		return;
	}

	pDeviceID = allocate( DeviceID.dwSize, "Device ID" );
	pDeviceID->dwSize = DeviceID.dwSize;

	bGet = KernelIoControl(IOCTL_HAL_GET_DEVICEID, NULL, 0, pDeviceID, pDeviceID->dwSize, &dwOutSizeOf);

	if ( bGet && dwOutSizeOf > 0 )
	{
		RawDebug("DeviceID struct: %s", (LPBYTE)pDeviceID, DeviceID.dwSize);

		if ( pDeviceID->dwPresetIDBytes > 0 )
		{
			Debug("Size of Terminal ID: %d", pDeviceID->dwPresetIDBytes);
			pTerminalID = (PULONG)((BYTE *)pDeviceID + pDeviceID->dwPresetIDOffset);
			Debug("Found TerminalID: %lu", *pTerminalID);
			RawDebug("Raw TerminalID: %s", (LPBYTE)pTerminalID, pDeviceID->dwPresetIDBytes);
		}
		else
		{
			Log("No TerminalID available");
		}

		if ( pDeviceID->dwPlatformIDBytes > 0 )
		{
			Debug("Size of PlatformID: %d", pDeviceID->dwPlatformIDBytes);
			size = pDeviceID->dwPlatformIDBytes / sizeof(WCHAR);
			szPlatformID = allocate(size+1, "PlatformID");

			if ( szPlatformID != NULL )
			{
				pPlatformID = (PULONG)((LPBYTE)pDeviceID + pDeviceID->dwPlatformIDOffset);
				wcstombs(szPlatformID, (LPWSTR)pPlatformID, size);
				Debug("Found PlatformID: %s", szPlatformID);
				free(szPlatformID);
				RawDebug("Raw PlatformID: %s", (LPBYTE)pPlatformID, pDeviceID->dwPlatformIDBytes);
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
			Debug("Found HTC IMEI: %s", sIMEI);
			addField( list, "IMEI", sIMEI );
		}
	}
	else
	{
		DebugError("Error with KernelIOControl: %d bytes returned", dwOutSizeOf);
	}

	free(pDeviceID);
}
