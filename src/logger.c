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

FILE *hLogger;

LPSTR lpLogBuffer = NULL;
LPSTR lpErrorBuf = NULL;

BOOLEAN bLoggerInit = FALSE; 

LPVOID GetSystemError(void)
{
	LPTSTR lpMsgBuf = NULL;
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

		LocalFree( lpMsgBuf );
	}

	return lpErrorBuf;
}

void LoggerInit(LPCSTR path)
{
	int buflen;
	LPCSTR basename = "glpi-wince-agent.txt" ;
	LPSTR  filename = NULL;

	buflen = strlen(path) + 1 + strlen(basename) + 1;
	filename = allocate( buflen, "Logfile name");
	sprintf( filename, "%s\\%s", path, basename );

	/* Reassign "stdout" */
	hLogger = freopen( filename, "w", stdout );

	free(filename);

	if( hLogger == NULL )
	{
		Error( "Can't reopen stdout toward file" );
		if (GetSystemError() != NULL)
		{
			fprintf( stdout, "Error reopening stdout: %s\n", lpErrorBuf);
			//MessageBox( NULL, (LPCTSTR)lpMsgBuf, L"LoggerInit", MB_OK | MB_ICONINFORMATION );
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
		fprintf( stdout, "Info  : %s\n", lpLogBuffer );
	else
	{
		if (GetSystemError() != NULL)
			fprintf( stdout, "Error with '%s' format, %s\n", format, lpErrorBuf);
		else
			fprintf( stdout, "Error with '%s' format\n", format);
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
		fprintf( stdout, "Error : %s\n", lpLogBuffer );
		fprintf( stderr, "Error : %s\n", lpLogBuffer );
		if (GetSystemError() != NULL)
			fprintf( stdout, "Last system error: %s\n", lpErrorBuf);
	}
	else
	{
		if (GetSystemError() != NULL)
			fprintf( stdout, "Error with '%s' format, %s\n", format, lpErrorBuf);
		else
			fprintf( stdout, "Error with '%s' format\n", format);
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

	if (!debugMode || !bLoggerInit)
		return;

	formatsize = strlen(format);
	size = vsprintf((LPSTR)lpLogBuffer, format, args);

	if (size && size<1024)
		fprintf( stdout, "Debug : %s\n", (LPSTR)lpLogBuffer );
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

	if (debugMode<2 || !bLoggerInit)
		return;

	formatsize = strlen(format);
	size = vsprintf((LPSTR)lpLogBuffer, format, args);

	if (size && size<1024)
		fprintf( stdout, "Debug2: %s\n", (LPSTR)lpLogBuffer );
	else
	{
		fprintf( stderr, "Debug2 Buffer overflow\n" );
		exit(EXIT_FAILURE);
	}

	va_end(args);
}
