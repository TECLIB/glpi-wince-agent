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

LIST *Xml = NULL;

void TargetInit(LPSTR deviceid)
{
	// Initialize XML content
	Xml = createList( "REQUEST" );
	addField( Xml, "DEVICEID", deviceid );
	addField( Xml, "QUERY", "INVENTORY" );

	// Insert inventory in Xml list
	addEntry( Xml, "CONTENT", NULL , getInventory() );
}

void TargetQuit(void)
{
	free(Xml);
}

static void WriteList(LIST *list, FILE *hFile, int indent)
{
	int i = indent;

	// Handle indentation
	while (i-->0)
		fprintf( hFile, "  ");

#ifdef DEBUG
		Debug2("Writelist: %s -> %s", list->key, list->value);
#endif

	if (list->value == NULL && list->leaf == NULL)
	{
		fprintf( hFile, "<%s/>\n", list->key );
	}
	else if ( list->leaf )
	{
		fprintf( hFile, "<%s>\n", list->key );
		WriteList( list->leaf, hFile, indent+1 );
		i = indent;
		while (i-->0)
			fprintf( hFile, "  ");
		fprintf( hFile, "</%s>\n", list->key );
	}
	else
	{
		fprintf( hFile, "<%s>%s</%s>\n", list->key, list->value, list->key );
	}
	if (list->next != NULL)
		WriteList( list->next, hFile, indent );
}

static void WriteXML(FILE *hFile)
{
	fprintf( hFile, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n" );
	WriteList( Xml, hFile, 0 );
}

void WriteLocal(LPSTR deviceid)
{
	FILE *hLocal;
	int Length = 0, count = 0;
	LPSTR LocalFile = NULL, buffer = NULL;
	LPSTR separator = NULL;
	LPSTR cursor = conf.local;

	Log("About to write inventory in local file...");

	while (cursor != NULL)
	{
		// Prepare filename
		Length = strlen(cursor);
		separator = strstr(cursor, ",");
		if (separator != NULL)
		{
			Length = separator - cursor;
		}

		// Handle "." special case
		if (strncmp(cursor, ".", Length) == 0)
		{
			Debug("Writing local inventory in current path");
			cursor = getCurrentPath();
			Length = strlen(cursor);
		}

		buffer = allocate( Length+2, NULL );
		snprintf( buffer, Length+1, "%s", cursor );

		if (Length != 0)
		{
			LocalFile = allocate( Length + strlen(deviceid) + 6, "LocalFile");
			sprintf( LocalFile, "%s\\%s.ocs", buffer, deviceid);
			Debug("Writing local inventory to %s", LocalFile);

			hLocal = fopen( LocalFile, "w" );
			if( hLocal == NULL )
			{
				Error("Can't write local inventory to %s", LocalFile);
			}
			else
			{
				WriteXML(hLocal);
				fclose(hLocal);
				count ++;
			}
			free(LocalFile);
		}

		free(buffer);

		if (separator == NULL)
			break;

		cursor = ++separator ;
	}

	if (count>1)
		Log("%d local inventory files written", count);
	else if (count)
		Log("One local inventory file written");
	else
		Log("No local inventory file written");
}

static BOOLEAN SendToServer(LPSTR deviceid, LPCSTR url)
{
	return FALSE;
}

void SendRemote(LPSTR deviceid)
{
	int Length = 0, count = 0;
	LPSTR separator, urlbuffer, cursor = conf.server;

	Debug("Parsing inventory server: %s", cursor);

	while (cursor != NULL)
	{
		// Prepare server url
		Length = strlen(cursor);
		separator = strstr(cursor, ",");
		if (separator != NULL)
		{
			Length = separator - cursor;
		}

		urlbuffer = allocate( Length+2, NULL );
		snprintf( urlbuffer, Length+1, "%s", cursor );

		if (Length != 0)
		{
			Log("About to send inventory to '%s'...", urlbuffer);
			if (SendToServer(deviceid, urlbuffer))
				count ++;
			else
				Error("Failed to send inventory to '%s'", urlbuffer);
		}

		free(urlbuffer);

		if (separator == NULL)
			break;

		cursor = ++separator ;
	}

	if (count>1)
		Log("%d remote inventory sent", count);
	else if (count)
		Log("One inventory sent");
	else
		Log("No inventory sent to server");
}
