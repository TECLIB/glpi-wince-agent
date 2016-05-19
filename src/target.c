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
#include <wininet.h>

#include "glpi-wince-agent.h"

LIST *Xml = NULL;

typedef struct {
	LPSTR url;
	INTERNET_SCHEME scheme;
	LPSTR server;
	INTERNET_PORT port;
	LPSTR username;
	LPSTR password;
	LPSTR urlpath;
} GLPISERVER, *LPGLPISERVER;

void TargetInit(LPSTR deviceid)
{
	// Initialize XML content
	free(Xml);
	Xml = createList( "REQUEST" );
	addField( Xml, "DEVICEID", deviceid );
	addField( Xml, "QUERY", "INVENTORY" );

	// Insert inventory in Xml list
	addEntry( Xml, "CONTENT", NULL , getInventory() );
}

void TargetQuit(void)
{
	Debug2("Freeing Target");
	free(Xml);

	Xml = NULL;
}

static LPSTR getListContent(LIST *list, LPSTR buffer, int indent, LPLONG size)
{
	int i = indent;
	LPSTR current = buffer;

	// Handle indentation
	while (i-->0)
	{
		current = realloc(buffer, *size += 2);
		if (current != NULL)
		{
			strcat(buffer=current, "  ");
		}
	}

#ifdef DEBUG
		Debug2("getListContent: %s -> %s", list->key, list->value);
#endif

	if (list->value == NULL && list->leaf == NULL)
	{
		current = realloc(buffer, *size += strlen(list->key)+5);
		if (current != NULL)
		{
			strcat(buffer=current, vsPrintf( "<%s/>\n", list->key ));
		}
	}
	else if ( list->leaf )
	{
		current = realloc(buffer, *size += strlen(list->key)+4);
		if (current != NULL)
		{
			strcat(buffer=current, vsPrintf( "<%s>\n", list->key ));
		}
		buffer = getListContent( list->leaf, buffer, indent+1, size );
		i = indent;
		while (i-->0)
		{
			current = realloc(buffer, *size += 2);
			if (current != NULL)
			{
				strcat(buffer=current, "  ");
			}
		}
		current = realloc(buffer, *size += strlen(list->key)+5);
		if (current != NULL)
		{
			strcat(buffer=current, vsPrintf( "</%s>\n", list->key ));
		}
	}
	else
	{
		current = realloc(buffer, *size += 2*strlen(list->key)+strlen(list->value)+7);
		if (current != NULL)
		{
			strcat(buffer=current, vsPrintf( "<%s>%s</%s>\n", list->key, list->value, list->key ));
		}
	}
	if (list->next != NULL)
		buffer = getListContent( list->next, buffer, indent, size );

	if (current == NULL && !indent)
	{
		// Buffer overflow case
		free(buffer);
		buffer = NULL;
	}

	return buffer;
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

static LPSTR getContent(void)
{
	LONG size = 0;
	LPCSTR header = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
	LPSTR content = NULL ;

	size = strlen(header) + 1;
	content = allocate( size, "XML Header" );
	strcpy( content, header );
	content = getListContent( Xml, content, 0, &size );
	if (content == NULL)
	{
		Error("getContent: Buffer overflow (%d bytes needed)", size);
	}
	return content;
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

	while (cursor != NULL && *cursor != '\0')
	{
		// Prepare filename
		separator = strstr(cursor, ",");
		Length = separator != NULL ? separator - cursor : strlen(cursor);

		if (Length != 0)
		{
			// Handle "." special case
			if (strncmp(cursor, ".", Length) == 0)
			{
				Debug("Writing local inventory in current path");
				cursor = getCurrentPath();
				Length = strlen(cursor);
			}

			buffer = allocate( Length+1, NULL );
			memset(buffer, 0, Length+1);
			_snprintf( buffer, Length, "%s", cursor );
			cursor = buffer + strlen(buffer);
			// Strip final folder separator
			while ( cursor>buffer && *cursor == '\\' )
				*cursor -- = '\0';
			LocalFile = vsPrintf("%s\\%s.ocs", buffer, deviceid);
			free(buffer);
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
		}

		cursor = separator ;
		if (cursor != NULL)
			cursor ++;

	}

	if (count>1)
		Log("%d local inventory files written", count);
	else if (count)
		Log("One local inventory file written");
	else
		Log("No local inventory file written");
}

static LPGLPISERVER DecodeGlpiUrl(LPCSTR url)
{
	LPGLPISERVER lpGlpi = NULL;
	URL_COMPONENTSA Url = {
		sizeof(URL_COMPONENTS),
		NULL, 1, INTERNET_SCHEME_UNKNOWN,		// lpszScheme + nScheme
		NULL, 1, INTERNET_DEFAULT_HTTP_PORT,	// lpszHostName + nPort
		NULL, 1,								// lpszUserName
		NULL, 1,								// lpszPassword
		NULL, 1,								// lpszUrlPath
		NULL, 1									// lpszExtraInfo
	};

	if (!InternetCrackUrlA(url, strlen(url), 0, &Url))
	{
		DebugError("Can't decode URL");
		return NULL;
	}

	if (!Url.dwHostNameLength)
	{
		Error("URL not decoded");
		return NULL;
	}

	lpGlpi = allocate(sizeof(GLPISERVER), NULL);

	// Copy URL
	Debug2("Decoding: %s", url);
	lpGlpi->url = allocate(strlen(url)+1, NULL);
	strcpy( lpGlpi->url, url);

	// Copy server name
	lpGlpi->server = allocate(Url.dwHostNameLength+1, NULL);
	memset(lpGlpi->server, 0, Url.dwHostNameLength+1);
	_snprintf( lpGlpi->server, Url.dwHostNameLength, "%s", Url.lpszHostName);
	Debug2("Extracted server name: %s", lpGlpi->server);

	// Copy username
	if (Url.dwUserNameLength)
	{
		lpGlpi->username = allocate(Url.dwUserNameLength+1, NULL);
		memset(lpGlpi->username, 0, Url.dwUserNameLength+1);
		_snprintf( lpGlpi->username, Url.dwUserNameLength, "%s", Url.lpszUserName);
		Debug2("Extracted username: %s", lpGlpi->username);
	}
	else
		lpGlpi->username = NULL;

	// Copy password
	if (Url.dwPasswordLength)
	{
		lpGlpi->password = allocate(Url.dwPasswordLength+1, NULL);
		memset(lpGlpi->password, 0, Url.dwPasswordLength+1);
		_snprintf( lpGlpi->password, Url.dwPasswordLength, "%s", Url.lpszPassword);
		Debug2("Extracted password: %s", lpGlpi->password);
	}
	else
		lpGlpi->password = NULL;

	// Copy urlpath
	if (Url.dwUrlPathLength)
	{
		lpGlpi->urlpath = allocate(strlen(Url.lpszUrlPath)+1, NULL);
		strcpy( lpGlpi->urlpath, Url.lpszUrlPath);
		Debug2("Extracted urlpath: %s", lpGlpi->urlpath);
	}
	else
		lpGlpi->urlpath = NULL;

	// Copy params
	Debug2("Extracted scheme & port: %d - %d", Url.nScheme, Url.nPort);
	lpGlpi->scheme = Url.nScheme;
	lpGlpi->port   = Url.nPort;

	return lpGlpi;
}

static void FreeGlpiUrl(LPGLPISERVER glpi)
{
	if (glpi != NULL)
	{
		// Free previously allocated memory
		free(glpi->url);
		free(glpi->server);
		free(glpi->username);
		free(glpi->password);
		free(glpi->urlpath);
		free(glpi);
	}
}

static void InternetError(LPCSTR error)
{
	DWORD dwError = 0, dwBufferLength = 0;
	LPSTR lpszBuffer = NULL;

	if (!InternetGetLastResponseInfoA(&dwError, lpszBuffer, &dwBufferLength))
	{
		if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
		{
			Error("%s, and can't get InternetError Info", error);
		}
		else
		{
			lpszBuffer = allocate( dwBufferLength+1, "InternetError" );
			if (!InternetGetLastResponseInfoA(&dwError, lpszBuffer, &dwBufferLength))
			{
				Error("%s, and can't get InternetError Info in allocated buffer", error);
			}
			else
			{
				Error("%s, error=%d: %s", error, dwError, lpszBuffer);
			}
			free(lpszBuffer);
		}
	}
}

static BOOL SendToServer(LPSTR deviceid, LPGLPISERVER glpi)
{
	HINTERNET hOpen, hInet, hRequest;
	BOOL useSsl = FALSE, result = TRUE;
	DWORD dwState = 0, dwFlags = 0, dwOptions = 0 ;
	LPSTR content = NULL;
	ULONG ulOption = 0;

	// Default flag for submission
	dwFlags |= INTERNET_FLAG_NO_CACHE_WRITE ;
	dwFlags |= INTERNET_FLAG_PRAGMA_NOCACHE ;

	if (glpi == NULL)
	{
		Error("GLPI Server url not initialized");
		return FALSE;
	}

	// Check internet connection availability
	if (InternetGetConnectedState(&dwState, 0))
	{
		if ( dwState & INTERNET_CONNECTION_OFFLINE )
		{
			Error("Internet connection is OFF-LINE");
			return FALSE;
		}
		if ( dwState & INTERNET_CONNECTION_CONFIGURED )
			Log("Internet connection seems configured");
		if ( dwState & INTERNET_CONNECTION_LAN )
			Log("Lan connection available");
		if ( dwState & INTERNET_CONNECTION_PROXY )
			Log("Internet connection available via proxy");
		if ( dwState & INTERNET_RAS_INSTALLED )
			Log("RAS internet installed");
		if ( dwState & INTERNET_CONNECTION_MODEM )
			Log("Internet connection available via modem");
	}
	else
	{
		Error("No internet connection seems available on device");
		//return FALSE;
	}

	// Check internet connection connectivity
	if (conf.debug && !InternetCheckConnectionA(NULL,0,0))
	{
		Debug("Internet connection connectivity test failed");
	}

	content = getContent();
	if (content == NULL)
	{
		Error("No content to send");
		return FALSE;
	}
#ifdef DEBUG
	else
	{
		Debug2("About to send:\n%s", content);
	}
#endif

	// Check URL scheme support
	switch (glpi->scheme)
	{
		case INTERNET_SCHEME_HTTPS:
			Debug("SSL support required for this server");
			useSsl = TRUE;
			// Ignore invalid SSL certificates
			dwFlags |= INTERNET_FLAG_IGNORE_CERT_CN_INVALID;
			dwFlags |= INTERNET_FLAG_IGNORE_CERT_DATE_INVALID;
			dwFlags |= INTERNET_FLAG_SECURE;
		case INTERNET_SCHEME_HTTP:
			Debug2("Standard HTTP scheme will be used for the request");
			break;
		default:
			free(content);
			Error("Not supported scheme for this server");
			return FALSE;
	}

	// Prepare Internet access if still not prepared
	Debug2("Opening internet access");
	LPSTR UserAgent = vsPrintf("%s (wince)", AgentName);
	hOpen = InternetOpenA( UserAgent, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (hOpen == NULL)
	{
		free(content);
		Error("Can't get internet access");
		return FALSE;
	}
	else
	{
		Debug("Internet opened");
		Debug2("User-Agent set to '%s'", UserAgent);
	}

	// Set Internet options
	/*
	 * Set connection time-out to 1 second
	 */
	dwOptions = INTERNET_OPTION_CONNECT_TIMEOUT ;
	ulOption  = GLPI_CONNECT_TIMEOUT ;
	if (InternetSetOption(hOpen, dwOptions, (LPVOID)&ulOption, sizeof(ulOption)))
	{
		Debug("Internet connection time-out set to %ulms", ulOption);
	} else {
		Error("Can't set internet connection time-out set to %ulms", ulOption);
	}

	// Get internet session
	Debug2("Opening internet session");
	hInet = InternetConnectA( hOpen, (LPCSTR)glpi->server, glpi->port,
		(LPCSTR)glpi->username, (LPCSTR)glpi->password, INTERNET_SERVICE_HTTP, 0, 0);
	if (hInet == NULL)
	{
		free(content);
		Error("Can't get internet session to access %s", glpi->server);
		return FALSE;
	}

	// Prepare POST request
	Debug2("Preparing HTTP request");
	hRequest = HttpOpenRequestA( hInet, "POST",
		(LPCSTR)glpi->urlpath, NULL, NULL, NULL, dwFlags, 0);
	if (hRequest == NULL)
	{
		Error("Failed to prepare %s request on server", glpi->urlpath);
		result = FALSE;
	}
	else
	{
		DWORD length, size = 0;
		LPSTR header = NULL;

		if (content != NULL)
			length = strlen(content);

		// Add content-type header
		header = vsPrintf("Content-type: %s\n", "application/xml");
		size = strlen(header);
		if ( !HttpAddRequestHeadersA(hRequest, header, size,
				HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE ))
		{
			Error("Failed to set request Content-type");
		}

		// Do the request
		Debug2("Sending the HTTP request");
		if (!HttpSendRequestA(hRequest, NULL, 0, content, length))
		{
			InternetError("Failed to post request to server");
			result = FALSE;
		}
		else
		{
			// Check result
			Debug2("Checking HTTP response status code");
			dwFlags = HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER ;
			size = sizeof(dwState);
			if (!HttpQueryInfoA(hRequest, dwFlags, &dwState, &size, NULL))
			{
				Error("Failed to check HTTP status code");
				result = FALSE;
			}
			else
			{
				switch (dwState)
				{
					case 200:
						Log("Inventory submitted");
						break;
					case 403:
						Error("Authentication required");
						result = FALSE;
						break;
					case 404:
						Error("GLPI not found behind URL");
						result = FALSE;
						break;
					case 405:
						Error("Request not allowed");
						result = FALSE;
						break;
					default:
						Error("Server status code: %d", dwState);
						result = FALSE;
						dwFlags = HTTP_QUERY_STATUS_TEXT ;
						size = 0;
						if (!HttpQueryInfoA(hRequest, dwFlags, NULL, &size, NULL))
						{
							free(content);
							content = allocate(size, NULL);
							if (!HttpQueryInfoA(hRequest, dwFlags, content, &size, NULL))
							{
								Error("Failed to check HTTP status text");
							}
							else
							{
								Error("Server status text: %s", content);
							}
						}
						break;
				}
			}

			// Debug content returned
			if (conf.debug>1)
			{
				if (InternetQueryDataAvailable( hRequest, &size, 0, 0 ) && size)
				{
					content = allocate( size+1, NULL );
					if (InternetReadFile(hRequest, content, size, &size))
					{
						content[size] = '\0';
						Debug2("Server response:\n%s", content);
					}
					else
						InternetError("Failed to check returned content");
					free(content);
				}
			}
		}
	}

	free(content);

	// Close internet session
	if ( hRequest != NULL && !InternetCloseHandle(hRequest) )
		DebugError("Got failure while closing request handle");

	// Close internet session
	if ( hInet != NULL && !InternetCloseHandle(hInet) )
		DebugError("Got failure while closing Internet connection");

	// Close connection handle
	if ( hOpen != NULL && !InternetCloseHandle(hOpen) )
		DebugError("Got failure while closing connection handle");

	return result;
}

BOOL SendRemote(LPSTR deviceid)
{
	int Length = 0, count = 0;
	LPSTR separator, cursor = conf.server;

	Debug("Parsing inventory server: %s", cursor);

	while (cursor != NULL && *cursor != '\0')
	{
		// Prepare one server url from server list
		separator = strstr(cursor, ",");
		Length = separator != NULL ? separator - cursor : strlen(cursor);

		if (Length != 0)
		{
			LPGLPISERVER lpGlpi;
			LPSTR urlbuffer = allocate( Length+1, NULL );
			memset(urlbuffer, 0, Length+1);
			_snprintf( urlbuffer, Length, "%s", cursor );

			lpGlpi = DecodeGlpiUrl(urlbuffer);
			if (lpGlpi == NULL)
			{
				Error("Can't decode '%s' URL", urlbuffer);
			}
			else
			{
				Log("About to send inventory to '%s'...", urlbuffer);
				if (SendToServer(deviceid, lpGlpi))
				{
					Debug2("Inventory sent to %s", lpGlpi->server);
					count ++;
				}
				else
				{
					Error("Failed to send inventory to '%s'", lpGlpi->server);
				}
				FreeGlpiUrl(lpGlpi);
			}
			free(urlbuffer);
		}

		cursor = separator ;
		if (cursor != NULL)
			cursor ++ ;
	}

	if (count>1)
		Log("%d remote inventory sent", count);
	else if (count)
		Log("One inventory sent");
	else
		Log("No inventory sent to server");

	return (count>0);
}
