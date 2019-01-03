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
#include <libgen.h>

#include "glpi-wince-agent.h"

#define LOGBUFFERSIZE 1024
#define ERRORBUFFERSIZE 64

#ifdef GWA
#define KEEPBUFFER
#define FREEBUFFER
#else
#define KEEPBUFFER TRUE
#define FREEBUFFER FALSE
#endif

FILE *hLogger;

LPSTR lpLogBuffer = NULL;
LPSTR lpErrorBuf = NULL;
#ifndef GWA
LPTSTR lpMsgBuf = NULL;
#endif

BOOL bLoggerInit = FALSE;

#ifdef STDERR
FILE *hStdErr = NULL;
#endif

// Avoid concurrent access to logger
#ifdef GWA
CRITICAL_SECTION SafeLogging;
#endif

#ifndef GWA
static LPVOID getSystemError(BOOL freeBuffer)
#else
static LPSTR getSystemError(void)
#endif
{
	int buflen = 0;
	int error = 0 ;
#ifdef GWA
	LPTSTR lpMsgBuf = NULL;
#endif

	error = GetLastError();

	if (lpErrorBuf != NULL)
		free(lpErrorBuf);
	lpErrorBuf = NULL;

	if (error != NO_ERROR)
	{
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			error,
			0, // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL
		);

		if (lpMsgBuf != NULL)
		{
			buflen = wcslen(lpMsgBuf)+1;
			lpErrorBuf = allocate( buflen, "Error buffer");
			wcstombs(lpErrorBuf, lpMsgBuf, buflen);

#ifndef GWA
			if (freeBuffer)
#endif
				LocalFree( lpMsgBuf );
		}
		else
		{
			lpErrorBuf = allocate( ERRORBUFFERSIZE, "Error buffer");
			sprintf(lpErrorBuf, "%d, ", error);
			switch (error)
			{
				case ERROR_FILE_NOT_FOUND:
					strcat(lpErrorBuf, "file not found");
					break;
				case ERROR_INVALID_HANDLE:
					strcat(lpErrorBuf, "invalid handle");
					break;
				default:
					strcat(lpErrorBuf, "unknown error");
					break;
			}
		}
	}

	return lpErrorBuf;
}

void LoggerInit(void)
{
	if (!bLoggerInit)
	{
		int buflen = 0;
#ifdef GWA
		LPCSTR basename = JOURNALBASENAME;
#else
		LPCSTR basename = INTERFACEBASENAME;
#endif
		LPSTR filename = NULL;
#ifdef TEST
		buflen = strlen(basename) + 2 ;
		filename = allocate( buflen, "Logfile name");
		sprintf( filename, "\\%s", basename ); /* Force journal under \ */
#else
		if (VarDir == NULL)
		{
			VarDir = getRegPath( "VarDir" );
		}
		buflen = strlen(VarDir) + strlen(basename) + 2 ;
		filename = allocate( buflen, "Logfile name");
		sprintf( filename, "%s\\%s", VarDir, basename );
#endif

		/* Reassign "stdout" */
		if( hLogger != NULL )
			fclose( hLogger );

#ifdef TEST
		hLogger = freopen( filename, "a+", stdout );
		fprintf(stderr, "GLPI-Agent journal: %s\n", filename);
#else
		hLogger = freopen( filename, "w", stdout );
#endif
		free(filename);

		if( hLogger == NULL )
		{
			Error( "Can't reopen stdout toward file" );
			if (getSystemError(KEEPBUFFER) != NULL)
			{
				fprintf( stdout, APPNAME": Error reopening stdout: %s\n", lpErrorBuf);
#ifndef GWA
				if (lpMsgBuf != NULL)
				{
					MessageBox( NULL, (LPCTSTR)lpMsgBuf, L"LoggerInit", MB_OK | MB_ICONINFORMATION );
					LocalFree( lpMsgBuf );
				}
#endif
			}
		}

		/* Allocate log buffer */
		lpLogBuffer = allocate(LOGBUFFERSIZE, "Log buffer");

		bLoggerInit = TRUE;

#ifdef GWA
		// Initialize critical section
		InitializeCriticalSection(&SafeLogging);
#else
		Log( "Logger initialized" );
		Debug( "Debug level enabled" );
		Debug2("Debug2 level enabled" );
#endif
	}
}

void LoggerQuit(void)
{
	if (bLoggerInit)
	{
		bLoggerInit = FALSE;

#ifdef GWA
		DeleteCriticalSection(&SafeLogging);
#else
		Debug2( "Stopping logger..." );
#endif

		if( hLogger != NULL )
			fclose( hLogger );

		// Free buffers
		free(lpErrorBuf);
		free(lpLogBuffer);

		lpErrorBuf  = NULL;
		lpLogBuffer = NULL;
		hLogger     = NULL;
	}
}

void Log(LPCSTR format, ...)
{
	int size;

#ifdef GWA
	if (!bLoggerInit)
	{
		LoggerInit();
	}
	EnterCriticalSection(&SafeLogging);
#endif

	va_list args;
	va_start(args, format);

	if (!bLoggerInit)
	{
#ifndef GWA
		vfprintf(stderr, format, args);
#endif
	}
	else
	{
		size = vsprintf((LPSTR)lpLogBuffer, format, args);

		if (size && size<1024)
			fprintf( stdout, "%sInfo  : %s\n", getTimestamp(), lpLogBuffer );
		else
		{
			if (getSystemError(FREEBUFFER) != NULL)
				fprintf( stdout, "%sError with '%s' format, %s\n", getTimestamp(), format, lpErrorBuf);
			else
				fprintf( stdout, "%sError with '%s' format\n", getTimestamp(), format);
#ifndef GWA
			fprintf( stderr, "Log Buffer overflow\n" );
			exit(EXIT_FAILURE);
#endif
		}
	}

	SetLastError(NO_ERROR);

	va_end(args);
#ifdef GWA
	LeaveCriticalSection(&SafeLogging);
#endif
}

void Error(LPCSTR format, ...)
{
	int size;

#ifdef GWA
	if (!bLoggerInit)
	{
		LoggerInit();
	}
	EnterCriticalSection(&SafeLogging);
#endif

	va_list args;
	va_start(args, format);

	if (!bLoggerInit)
	{
#ifndef GWA
		vfprintf(stderr, format, args);
#endif
	}
	else
	{
		size = vsprintf((LPSTR)lpLogBuffer, format, args);

		if (size && size<1024)
		{
			fprintf( stdout, "%sError : %s\n", getTimestamp(), lpLogBuffer );
#ifndef GWA
			fprintf( stderr, "Error : %s\n", lpLogBuffer );
#endif
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
#ifndef GWA
			fprintf( stderr, "Error Buffer overflow\n" );
			exit(EXIT_FAILURE);
#endif
		}
	}

	SetLastError(NO_ERROR);

	va_end(args);
#ifdef GWA
	LeaveCriticalSection(&SafeLogging);
#endif
}

void Debug(LPCSTR format, ...)
{
	int size;

#ifdef GWA
	if (!bLoggerInit)
	{
		LoggerInit();
	}
	EnterCriticalSection(&SafeLogging);
#endif

	va_list args;
	va_start(args, format);

	if (!conf.debug || !bLoggerInit)
		return;

	size = vsprintf((LPSTR)lpLogBuffer, format, args);

	if (size && size<1024)
		fprintf( stdout, "%sDebug : %s\n", getTimestamp(), (LPSTR)lpLogBuffer );
#ifndef GWA
	else
	{
		fprintf( stderr, "Debug Buffer overflow\n" );
		exit(EXIT_FAILURE);
	}
#endif

	va_end(args);
#ifdef GWA
	LeaveCriticalSection(&SafeLogging);
#endif
}

void Debug2(LPCSTR format, ...)
{
	int size;

#ifdef GWA
	if (!bLoggerInit)
	{
		LoggerInit();
	}
	EnterCriticalSection(&SafeLogging);
#endif

	va_list args;
	va_start(args, format);

	if (conf.debug<2 || !bLoggerInit)
		return;

	size = vsprintf((LPSTR)lpLogBuffer, format, args);

	if (size && size<1024)
		fprintf( stdout, "%sDebug2: %s\n", getTimestamp(), (LPSTR)lpLogBuffer );
#ifndef GWA
	else
	{
		fprintf( stderr, "Debug2 Buffer overflow\n" );
		exit(EXIT_FAILURE);
	}
#endif

	va_end(args);
#ifdef GWA
	LeaveCriticalSection(&SafeLogging);
#endif
}

void DebugError(LPCSTR format, ...)
{
	int size;

#ifdef GWA
	if (!bLoggerInit)
	{
		LoggerInit();
	}
	EnterCriticalSection(&SafeLogging);
#endif

	va_list args;
	va_start(args, format);

	if (!conf.debug || !bLoggerInit)
		return;

	size = vsprintf((LPSTR)lpLogBuffer, format, args);

	if (size && size<1024)
	{
		fprintf( stdout, "%sDebugE: %s\n", getTimestamp(), (LPSTR)lpLogBuffer );
		if (getSystemError(FREEBUFFER) != NULL)
			fprintf( stdout, "%sDebugE: System error: %s\n", getTimestamp(), lpErrorBuf);
	}
#ifndef GWA
	else
	{
		fprintf( stderr, "DebugE Buffer overflow\n" );
		exit(EXIT_FAILURE);
	}
#endif

	SetLastError(NO_ERROR);

	va_end(args);
#ifdef GWA
	LeaveCriticalSection(&SafeLogging);
#endif
}

void RawDebug(LPCSTR format, LPBYTE buffer, ULONG size)
{
	LPCSTR HexLookup = "0123456789abcdef";
	LPSTR dumpbuffer = NULL, index = NULL;
	LPBYTE max = buffer + size ;
	BYTE value;

#ifdef GWA
	if (!bLoggerInit)
	{
		LoggerInit();
	}
#endif

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

#ifdef STDERR
void stderrf(LPCSTR format, ...)
{
	if (hStdErr != NULL)
		fclose(hStdErr);

	hStdErr = freopen( STDERRFILE, "a+", stderr );

	if (hStdErr != NULL)
	{
		fprintf( stderr, getTimestamp() );

		va_list args;
		va_start(args, format);
		vfprintf( stderr, format, args);
		va_end(args);

		fprintf( stderr, "\n");

		fclose(hStdErr);
	}

	hStdErr = NULL;
}
#endif
