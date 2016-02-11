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

LPSTR getIPAddress(void)
{
	LPSTR ipaddress = NULL;
	PFIXED_INFO pFixedInfo = NULL;

	pFixedInfo = getNetworkParams();
	return ipaddress;
}

LPSTR getMACAddress(void)
{
#ifdef DEBUG
	LPSTR name = NULL;
	LPSTR macdump = NULL;
#endif
	LPSTR macaddress = NULL;
	DWORD Err = 0, maclen = 0, Size = 0, Index = 0;
	PIP_ADAPTER_ADDRESSES pAdapterAddrs, pAdapt;

	// Enumerate all of the adapter specific information using the
	// IP_ADAPTER_ADDRESSES structure.
	// Note:  IP_ADAPTER_INFO contains a linked list of adapter entries.

	// First get needed buffer size and allocate buffer
	if ((Err = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, NULL, &Size)) != ERROR_SUCCESS)
	{
		if ((Err != ERROR_BUFFER_OVERFLOW) && (Err != ERROR_INSUFFICIENT_BUFFER))
		{
			Error("GetAdaptersAddresses() sizing failed with error code %d\n", (int)Err);
			return NULL;
		}
	}
	pAdapterAddrs = allocate(Size, "AdapterAddrs");

	// Then get all adaptaters
	if ((Err = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, pAdapterAddrs, &Size)) != ERROR_SUCCESS)
	{
		Error("GetAdaptersAddresses() sizing failed with error code %d\n", (int)Err);
		return NULL;
	}

	// Enumerate through each retuned adapter and return first available one
	pAdapt = pAdapterAddrs;
	while (pAdapt)
	{
		maclen = pAdapt->PhysicalAddressLength;
#ifdef DEBUG
		Size = wcslen(pAdapt->FriendlyName);
		name = allocate( Size+1, NULL);
		wcstombs(name, pAdapt->FriendlyName, Size);
		Debug2("Adapter%d: %s (%d)", Index, name, maclen);
		free(name);
		if (maclen)
		{
			macdump = allocate( maclen * 3, NULL);
			FormatPhysicalAddress(pAdapt->PhysicalAddress, maclen, macdump);
			Debug2("Adapter%d: %s", Index, macdump);
			free(macdump);
		}
		Index++;
#endif
		if (maclen && macaddress == NULL)
		{
			macaddress = allocate( maclen * 3, "MacAddress");
			FormatPhysicalAddress(pAdapt->PhysicalAddress, maclen, macaddress);
#ifndef DEBUG
			break;
#endif
		}
		pAdapt = pAdapt->Next;
	}
	return macaddress;
}
