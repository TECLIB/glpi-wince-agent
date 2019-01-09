/*
 * GLPI Windows CE Agent
 * 
 * Copyright (C) 2019 - Teclib SAS
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

#define MAX_ADDRESS_LENGHT 256

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
		_itoa( addr[i] >> 4, ptr, 16);
		ptr++;

		_itoa( addr[i] & 0x0F, ptr, 16);
		ptr++;

		// Add a colon if we're not at the end
		if (i+1 < addrlen)
		{
			*ptr = ':';
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
		case IF_TYPE_ETHERNET_CSMACD:
		case IF_TYPE_FASTETHER:
		case IF_TYPE_FASTETHER_FX:
		case IF_TYPE_GIGABITETHERNET:
			return "ethernet";
		case IF_TYPE_PPP:
		case IF_TYPE_MODEM:
		case IF_TYPE_ISDN:
			return "dialup";
		case IF_TYPE_IEEE80211:
			return "wifi";
		//case IF_TYPE_SOFTWARE_LOOPBACK:
		//	return "loopback";
	}
	return NULL;
}

void getNetworks(void)
{
	LIST *Network = NULL;
	DWORD Err = 0, Size = 0;
	PIP_ADAPTER_ADDRESSES pAdapterAddresses = NULL, pAdapterAddress = NULL;

	// First get needed buffer size and allocate buffer
	if ((Err = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, pAdapterAddresses, &Size)) != NO_ERROR)
	{
		if ( Err == ERROR_NO_DATA )
		{
			Log("No network Adapter found");
			return;
		}
		else if (Err != ERROR_BUFFER_OVERFLOW)
		{
			Error("GetAdaptersAddresses() sizing failed with error code %d\n", (int)Err);
			return;
		}
	}
	pAdapterAddresses = allocate(Size, "pAdapterAddresses");

	// Then get all adapters adresses
	if ((Err = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, pAdapterAddresses, &Size)) != NO_ERROR)
	{
		Error("GetAdaptersAddresses() failed with error code %d\n", (int)Err);
		free(pAdapterAddresses);
		return;
	}

	// Enumerate through each retuned adapter and return first available one
	pAdapterAddress = pAdapterAddresses;
	while (pAdapterAddress)
	{
		LPCSTR type = NULL;

		Debug("Analysing adapter <%s> with type <%d>", pAdapterAddress->AdapterName, pAdapterAddress->IfType);

		// Check type
		type = SupportedAdapterType(pAdapterAddress->IfType);
		if (type != NULL)
		{
			LPSTR buffer = NULL;

			// Prepare Networks list
			Network = createList("NETWORKS");

			// Add recognized type
			addField( Network, "TYPE", (LPSTR)type );

			// convert and add description
			Size = wcslen(pAdapterAddress->Description)+1;
			buffer = allocate(Size, "Description");
			wcstombs(buffer, pAdapterAddress->Description, Size);
			addField( Network, "DESCRIPTION", buffer );
			free(buffer);

			// Add model
			addField( Network, "MODEL", pAdapterAddress->AdapterName );

			// Add macaddress field
			Size = pAdapterAddress->PhysicalAddressLength;
			if (Size)
			{
				buffer = allocate( Size * 3, "MacAddress");
				if (buffer != NULL)
				{
					FormatPhysicalAddress(pAdapterAddress->PhysicalAddress, Size, buffer);
					addField( Network, "MACADDR", buffer );
				}
				free(buffer);
			}

			if (pAdapterAddress->FirstUnicastAddress != NULL)
			{
				LPTSTR wAddressString = NULL;
				PIP_ADAPTER_UNICAST_ADDRESS Unicast = pAdapterAddress->FirstUnicastAddress;
				wAddressString = allocate( 2*MAX_ADDRESS_LENGHT, "IP Address");
				Size = MAX_ADDRESS_LENGHT - 1;
				WSAAddressToString(Unicast->Address.lpSockaddr, Unicast->Address.iSockaddrLength, NULL, wAddressString, &Size);
				if (WSAAddressToString(Unicast->Address.lpSockaddr, Unicast->Address.iSockaddrLength, NULL, wAddressString, &Size) == 0)
				{
					buffer = allocate( MAX_ADDRESS_LENGHT, "IP Address");
					wcstombs(buffer, wAddressString, Size);
					// Add IP address
					addField( Network, "IPADDRESS", buffer );
					free(buffer);
				}
				else
				{
					DebugError("Unexpected ws2 error %d looking for address (size=%d)", WSAGetLastError(), Size);
				}
				free(wAddressString);
			}

			// Finally add netry to Networks inventory
			InventoryAdd( "NETWORKS", Network );
		}

		pAdapterAddress = pAdapterAddress->Next;
	}

	free(pAdapterAddresses);
}
