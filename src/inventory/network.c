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
#include <winsock2.h>

#include "../glpi-wince-agent.h"

// Function: FormatPhysicalAddress
// Description:
//    Takes a BYTE array and converts each byte into two hexadecimal
//    digits followed by a dash (-). The converted data is placed
//    into the supplied string buffer.
static void FormatPhysicalAddress(BYTE *addr, int addrlen, LPSTR buf)
{
	int i;
	LPSTR ptr = buf;

	if (addrlen == 0)
		return;

	for(i=0; i < addrlen ;i++)
	{
		// Take each byte and convert to a hex digit
		_itoa( addr[i] >> 4, ptr, sizeof(ptr));
		ptr++;

		_itoa( addr[i] & 0x0F, ptr, sizeof(ptr));
		ptr++;

		// Add the dash if we're not at the end
		if (i+1 < addrlen)
		{
			*ptr = '-';
			ptr++;
		}
	}
	*ptr = '\0';
}

// Function: SupportedAdapterType
// Description:
//    This function takes the adapter type which is a simple integer
//    and returns the string representation for that type.
//    The whole list of adapter types is defined in IPIfCons.h.
static LPCSTR SupportedAdapterType(int type)
{
	switch (type)
	{
		case MIB_IF_TYPE_ETHERNET:
			return "ethernet";
		case MIB_IF_TYPE_PPP|IF_TYPE_MODEM:
			return "dialup";
		case MIB_IF_TYPE_LOOPBACK:
			return "loopback";
	}
	return NULL;
}

LPSTR getIPAddress(void)
{
	LPSTR ipaddress = NULL;
	PFIXED_INFO pFixedInfo = NULL;

	pFixedInfo = getNetworkParams();
	return ipaddress;
}

void getNetworks(void)
{
	LPSTR macaddress = NULL;
	LIST *Network = NULL;
	LPCSTR type;
	DWORD Err = 0, maclen = 0, Size = 0;
	PIP_ADAPTER_INFO pAdapterInfos = NULL, pAdapterInfo = NULL;

	// First get needed buffer size and allocate buffer
	if ((Err = GetAdaptersInfo(pAdapterInfos, &Size)) != NO_ERROR)
	{
		if ( Err == ERROR_NO_DATA )
		{
			Log("No network Adapter found");
			return;
		}
		else if ((Err != ERROR_BUFFER_OVERFLOW) && (Err != ERROR_INSUFFICIENT_BUFFER))
		{
			Error("GetAdaptersInfo() sizing failed with error code %d\n", (int)Err);
			return;
		}
	}
	pAdapterInfos = allocate(Size, "pAdapterInfos");

	// Then get all adaptaters info
	if ((Err = GetAdaptersInfo(pAdapterInfos, &Size)) != NO_ERROR)
	{
		Error("GetAdaptersInfo() failed with error code %d\n", (int)Err);
		free(pAdapterInfos);
		return;
	}

    //NETWORKS         => [ qw/DESCRIPTION MANUFACTURER MODEL MANAGEMENT TYPE
                             //VIRTUALDEV MACADDR WWN DRIVER FIRMWARE PCIID
                             //PCISLOT PNPDEVICEID MTU SPEED STATUS SLAVES BASE
                             //IPADDRESS IPSUBNET IPMASK IPDHCP IPGATEWAY
                             //IPADDRESS6 IPSUBNET6 IPMASK6

	// Enumerate through each retuned adapter and return first available one
	pAdapterInfo = pAdapterInfos;
	while (pAdapterInfo)
	{
		// Check type
		type = SupportedAdapterType(pAdapterInfo->Type);
		if (type != NULL)
		{
			// Prepare Networks list
			Network = createList("NETWORKS");

			// Add recognized type
			addField( Network, "TYPE", (LPSTR)type );

			addField( Network, "DESCRIPTION", pAdapterInfo->Description );

			// Add model
			addField( Network, "MODEL", pAdapterInfo->AdapterName );

			// Add macaddress field
			maclen = pAdapterInfo->AddressLength;
			if (maclen && macaddress == NULL)
			{
				macaddress = allocate( maclen * 3, "MacAddress");
				FormatPhysicalAddress(pAdapterInfo->Address, maclen, macaddress);
				addField( Network, "MACADDR", macaddress );
				free(macaddress);
			}

			// Add IP address
			addField( Network, "IPADDRESS",
				pAdapterInfo->CurrentIpAddress->IpAddress.String );
			addField( Network, "IPMASK",
				pAdapterInfo->CurrentIpAddress->IpMask.String );

			// Finally add netry to Networks inventory
			InventoryAdd( "NETWORKS", Network );
		}

		pAdapterInfo = pAdapterInfo->Next;
	}

	free(pAdapterInfos);
}
