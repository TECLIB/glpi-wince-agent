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

#include "glpi-wince-agent.h"

LPSYSTEMTIME lpLocalTime = NULL;

static DWORD dwStartTick = 0;
static LPSTR sHostname = NULL;

PFIXED_INFO pFixedInfo = NULL;

void *allocate(ULONG size, LPCSTR reason )
{
	void *pointer = NULL;

	if (size == 0) {
		Error("[%s] No need to allocate memory", reason);
	} else {
		// Allocate memory from sizing information
		if ((pointer = malloc(size)) == NULL)
		{
			if (reason != NULL)
				Error("[%s] Can't allocate %lu bytes", reason, size);
			else
				Error("Can't allocate %lu bytes", size);
			Abort();
		}
#ifdef DEBUG
		else
		{
			if (reason != NULL)
				Debug2("[%s] Allocated %i bytes", reason, size);
		}
#endif
	}
	return pointer;
}

PFIXED_INFO getNetworkParams(void)
{
	static DWORD expiration = 0;
	DWORD	Err = 0, FixedInfoSize = 0;

	if (pFixedInfo != NULL && GetTickCount() <= expiration)
		return pFixedInfo;

	/*
	 *  Get the main IP configuration information for this machine using
	 * the FIXED_INFO structure
	 */
	if ((Err = GetNetworkParams(NULL, &FixedInfoSize)) != 0)
	{
		if ((Err != ERROR_BUFFER_OVERFLOW) && (Err != ERROR_INSUFFICIENT_BUFFER))
		{
			Error("GetNetworkParams() sizing failed with error %lu\n", Err);
			Abort();
		}
	}

	// Allocate memory
	free(pFixedInfo);
	pFixedInfo = allocate(FixedInfoSize, "GetNetworkParams");

	// Retreive network params
	if ((Err = GetNetworkParams(pFixedInfo, &FixedInfoSize)) != 0) {
		Error("GetNetworkParams() failed with error code %lu\n", Err);
		Abort();
	}

	expiration = GetTickCount() - dwStartTick + EXPIRATION_DELAY;

	return pFixedInfo;
}

LPSTR getHostname(void)
{
	static DWORD expiration = 0;
	int buflen = 1;

	if (sHostname == NULL || GetTickCount() > expiration)
	{
		getNetworkParams();

		// Allocate memory & copy hostname
		free(sHostname);
		buflen += strlen(pFixedInfo->HostName);
		sHostname = allocate( buflen, "Hostname");
		strcpy( sHostname, pFixedInfo->HostName);

		expiration = GetTickCount() - dwStartTick + EXPIRATION_DELAY;
	}

	return sHostname;
}

LPSYSTEMTIME getLocalTime(void)
{
	free(lpLocalTime);
	lpLocalTime = allocate(sizeof(SYSTEMTIME), "LocalTime");
	GetLocalTime(lpLocalTime);
	return lpLocalTime;
}

void ToolsInit(void)
{
	dwStartTick = GetTickCount();
}

void ToolsQuit(void)
{
	free(lpLocalTime);
	free(pFixedInfo);
	free(sHostname);
}
