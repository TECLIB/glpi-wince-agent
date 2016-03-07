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

#include <string.h>
#include <windows.h>

#include "glpi-wince-agent.h"

#define LOGBUFFERSIZE 1024

#define KEEPBUFFER TRUE
#define FREEBUFFER FALSE

FILE *hLogger;

LPSTR lpLogBuffer = NULL;
LPSTR lpErrorBuf = NULL;
LPTSTR lpMsgBuf = NULL;

BOOLEAN bLoggerInit = FALSE; 

static LPVOID getSystemError(BOOLEAN freeBuffer)
{
	int buflen = 0;

	free(lpErrorBuf);
	lpErrorBuf = NULL;

	if (GetLastError() != NO_ERROR)
	{
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			0, // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL
		);

		buflen = wcslen(lpMsgBuf)+1;
		lpErrorBuf = allocate( buflen, "Error buffer");
		wcstombs(lpErrorBuf, lpMsgBuf, buflen);

		if (freeBuffer)
			LocalFree( lpMsgBuf );
	}

	return lpErrorBuf;
}

void LoggerInit(LPCSTR path)
{
	int buflen = 2;
	LPCSTR basename = "glpi-wince-agent.txt" ;
	LPSTR  filename = NULL;

	buflen = strlen(path) + strlen(basename) ;
	filename = allocate( buflen, "Logfile name");
	sprintf( filename, "%s\\%s", path, basename );

	/* Reassign "stdout" */
	hLogger = freopen( filename, "w", stdout );

	free(filename);

	if( hLogger == NULL )
	{
		Error( "Can't reopen stdout toward file" );
		if (getSystemError(KEEPBUFFER) != NULL)
		{
			fprintf( stdout, "Error reopening stdout: %s\n", lpErrorBuf);
			MessageBox( NULL, (LPCTSTR)lpMsgBuf, L"LoggerInit", MB_OK | MB_ICONINFORMATION );
			LocalFree( lpMsgBuf );
		}
	}

	/* Allocate log buffer */
	lpLogBuffer = allocate(LOGBUFFERSIZE, "Log buffer");

	bLoggerInit = TRUE;

	Log( "Logger initialized" );
	Debug( "Debug level enabled" );
#ifdef DEBUG
	Debug2("Debug2 level enabled" );
#endif
}

void LoggerQuit(void)
{
	Log( "Stopping logger..." );
	if( hLogger != NULL )
		fclose( hLogger );
	Log( "Logger stopped" );

	// Free buffers
	free(lpErrorBuf);
	free(lpLogBuffer);
	free(lpMsgBuf);
}

void Log(LPCSTR format, ...)
{
	int size, formatsize;

	va_list args;
	va_start(args, format);

	if (!bLoggerInit)
		return;

	formatsize = strlen(format);
	size = vsprintf((LPSTR)lpLogBuffer, format, args);

	if (size && size<1024)
		fprintf( stdout, "%sInfo  : %s\n", getTimestamp(), lpLogBuffer );
	else
	{
		if (getSystemError(FREEBUFFER) != NULL)
			fprintf( stdout, "%sError with '%s' format, %s\n", getTimestamp(), format, lpErrorBuf);
		else
			fprintf( stdout, "%sError with '%s' format\n", getTimestamp(), format);
		fprintf( stderr, "Log Buffer overflow\n" );
		exit(EXIT_FAILURE);
	}

	SetLastError(NO_ERROR);

	va_end(args);
}

void Error(LPCSTR format, ...)
{
	int size, formatsize;

	va_list args;
	va_start(args, format);

	if (!bLoggerInit)
		return;

	formatsize = strlen(format);
	size = vsprintf((LPSTR)lpLogBuffer, format, args);

	if (size && size<1024)
	{
		fprintf( stdout, "%sError : %s\n", getTimestamp(), lpLogBuffer );
		fprintf( stderr, "Error : %s\n", lpLogBuffer );
		if (getSystemError(FREEBUFFER) != NULL)
			fprintf( stdout, "Last system error: %s\n", lpErrorBuf);
#ifdef DEBUG
		Debug2("Errno: %d", GetLastError());
#endif
	}
	else
	{
		if (getSystemError(FREEBUFFER) != NULL)
			fprintf( stdout, "%sError with '%s' format, %s\n", getTimestamp(), format, lpErrorBuf);
		else
			fprintf( stdout, "%sError with '%s' format\n", getTimestamp(), format);
		fprintf( stderr, "Error Buffer overflow\n" );
		exit(EXIT_FAILURE);
	}

	SetLastError(NO_ERROR);

	va_end(args);
}

void Debug(LPCSTR format, ...)
{
	int size, formatsize;

	va_list args;
	va_start(args, format);

	if (!conf.debug || !bLoggerInit)
		return;

	formatsize = strlen(format);
	size = vsprintf((LPSTR)lpLogBuffer, format, args);

	if (size && size<1024)
		fprintf( stdout, "%sDebug : %s\n", getTimestamp(), (LPSTR)lpLogBuffer );
	else
	{
		fprintf( stderr, "Debug Buffer overflow\n" );
		exit(EXIT_FAILURE);
	}

	va_end(args);
}

void Debug2(LPCSTR format, ...)
{
	int size, formatsize;

	va_list args;
	va_start(args, format);

	if (conf.debug<2 || !bLoggerInit)
		return;

	formatsize = strlen(format);
	size = vsprintf((LPSTR)lpLogBuffer, format, args);

	if (size && size<1024)
		fprintf( stdout, "%sDebug2: %s\n", getTimestamp(), (LPSTR)lpLogBuffer );
	else
	{
		fprintf( stderr, "Debug2 Buffer overflow\n" );
		exit(EXIT_FAILURE);
	}

	va_end(args);
}

void DebugError(LPCSTR format, ...)
{
	int size, formatsize;

	va_list args;
	va_start(args, format);

	if (!conf.debug || !bLoggerInit)
		return;

	formatsize = strlen(format);
	size = vsprintf((LPSTR)lpLogBuffer, format, args);

	if (size && size<1024)
	{
		fprintf( stdout, "%sDebugE: %s\n", getTimestamp(), (LPSTR)lpLogBuffer );
		if (getSystemError(FREEBUFFER) != NULL)
			fprintf( stdout, "%sDebugE: System error: %s\n", getTimestamp(), lpErrorBuf);
	}
	else
	{
		fprintf( stderr, "DebugE Buffer overflow\n" );
		exit(EXIT_FAILURE);
	}

	SetLastError(NO_ERROR);

	va_end(args);
}

void RawDebug(LPCSTR format, LPBYTE buffer, ULONG size)
{
	LPCSTR HexLookup = "0123456789abcdef";
	LPSTR dumpbuffer = NULL, index = NULL;
	LPBYTE max = buffer + size ;
	BYTE value;

	if (!conf.debug || !bLoggerInit)
		return;

	index = dumpbuffer = allocate( 2*size+1, NULL );

	while (buffer<max)
	{
		value = *buffer++;
		*index++ = HexLookup[(value >> 4) & 0x0F];
		*index++ = HexLookup[value & 0x0F];
	}
	*index ='\0';

	Debug(format, dumpbuffer);

	free(dumpbuffer);
}
