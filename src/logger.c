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

#include <string.h>
#include <windows.h>
#include <libgen.h>

#include "glpi-wince-agent.h"

#define MAX_VS_SIZE 256
#define MAX_LF_SIZE (64*1024)

FILE *hLogger = NULL;

BOOL bLoggerInit = FALSE;

#ifdef STDERR
FILE *hStdErr = NULL;
#endif

// Avoid concurrent access to logger
#ifdef GWA
CRITICAL_SECTION SafeLogging;
#endif

LPSTR logFilename = NULL;
static char lpLogText[MAX_VS_SIZE];

static BOOL getSystemError(void)
{
	int error = 0 ;
	BOOL ret = FALSE;

	error = GetLastError();

	if (error != NO_ERROR)
	{
		LPTSTR lpMsgBuf = NULL;

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
			LPSTR buffer = NULL;
			int buflen = 0;

			buflen = wcslen(lpMsgBuf)+1;
			buffer = malloc(buflen);
			if (buffer != NULL)
			{
				ret = TRUE;
				wcstombs(buffer, lpMsgBuf, buflen);
				sprintf(lpLogText, "%d, %s", error, buffer);
				free(buffer);
			}
			LocalFree( lpMsgBuf );
		}
		else
		{
			ret = TRUE;
			sprintf(lpLogText, "%d, ", error);
			switch (error)
			{
				case ERROR_FILE_NOT_FOUND:
					strcat(lpLogText, "file not found");
					break;
				case ERROR_INVALID_HANDLE:
					strcat(lpLogText, "invalid handle");
					break;
				default:
					strcat(lpLogText, "unknown error");
					break;
			}
		}
	}

	return ret;
}

void SystemDebug(LPCSTR format, ...)
{
	LPSTR buffer = NULL;

#ifdef GWA
	OutputDebugString(L"glpi-agent-service: ");
#else
	OutputDebugString(L"glpi-agent-client: ");
#endif

	buffer = malloc(MAX_VS_SIZE);
	if (buffer != NULL)
	{
		LPTSTR wLogBuf = NULL;
		int buflen = 0;

		va_list args;
		va_start(args, format);
		buflen = _vsnprintf(buffer, MAX_VS_SIZE-1, format, args);
		va_end(args);

#ifdef STDERR
		stderrf(buffer);
#endif

		// enlarge buffer to include CR
		wLogBuf = malloc( 2*(++buflen)+1 );
		if (wLogBuf != NULL)
		{
			swprintf( wLogBuf, L"%hs\n", buffer );
			OutputDebugString(wLogBuf);
			free(wLogBuf);
		}
		free(buffer);
	}
}

void LoggerInit(void)
{
	if (logFilename == NULL)
	{
		int buflen = 0;
#ifdef GWA
		LPCSTR basename = JOURNALBASENAME;
#else
		LPCSTR basename = INTERFACEBASENAME;
#endif

#ifdef TEST
		buflen = strlen(basename) + 2 ;
		logFilename = malloc(buflen);
		if (logFilename != NULL)
		{
			/* Force journal under root folder */
			sprintf( logFilename, "\\%s", basename );
		}
#else
		if (VarDir == NULL)
		{
			VarDir = getRegPath( "VarDir" );
		}
		buflen = strlen(VarDir) + strlen(basename) + 2 ;
		logFilename = malloc(buflen);
		if (logFilename != NULL)
		{
			sprintf( logFilename, "%s\\%s", VarDir, basename );
		}
#endif
#ifdef DEBUG
		SystemDebug("in LoggerOpen: logFilename = %s", logFilename);
#endif
	}

	if (!bLoggerInit)
	{
		bLoggerInit = TRUE;

#ifdef GWA
		// Initialize critical section
		InitializeCriticalSection(&SafeLogging);
#ifndef TEST
		// Truncate logfile
		hLogger = fopen( logFilename, "w" );
		fclose(hLogger);
		hLogger = NULL;
#endif
#endif
	}
}

BOOL LoggerOpen(void)
{
	static DWORD expiration = 0;

	lpLogText[MAX_VS_SIZE-1] = '\0';

	if (!bLoggerInit)
	{
		LoggerInit();
	}
#ifdef GWA
	if (!TryEnterCriticalSection(&SafeLogging))
	{
		SystemDebug("in LoggerOpen: TryEnterCriticalSection failure");
		return FALSE;
	}
#endif

	if( hLogger != NULL )
	{
		return FALSE;
	}

	// Check if it's time to rotate before logging: check at first run
	// and than every 10 minutes if log file is larger than 64kB before
	// rotating it
	if (expiration < GetTickCount())
	{
		LPTSTR wLogFile = NULL;
		HANDLE hLogFile = NULL;
		DWORD dwSize = 0;
		int buflen = strlen(logFilename);

		wLogFile = malloc(2*(buflen+1));
		if (wLogFile != NULL)
		{
			swprintf(wLogFile, L"%hs", logFilename);
			hLogFile = CreateFile(wLogFile, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hLogFile != NULL)
			{
				dwSize = GetFileSize(hLogFile, NULL);
				CloseHandle(hLogFile);
			}
		}

		if (dwSize == 0xFFFFFFFF)
		{
			SystemDebug("Failed to check %s file size", logFilename);
		}
		else if (dwSize > MAX_LF_SIZE)
		{
			LPTSTR wOldLogFile = NULL;

			wOldLogFile = malloc(2*(buflen+5));
			if (wOldLogFile != NULL)
			{
#ifdef DEBUG
				SystemDebug("Rotating %s log file", logFilename);
#endif
				swprintf(wOldLogFile, L"%hs", logFilename);
				wOldLogFile[buflen-4] = '\0';
				wcscat(wOldLogFile, L".old.txt");
				DeleteFile(wOldLogFile);
				MoveFile(wLogFile, wOldLogFile);
			}
			free(wOldLogFile);
		}
#ifdef DEBUG
		else if (dwSize)
		{
			SystemDebug("%s file size = %d bytes", logFilename, dwSize);
		}
#endif
		free(wLogFile);
		expiration = GetTickCount() + EXPIRATION_DELAY;
	}

	hLogger = fopen( logFilename, "a+" );

	if( hLogger == NULL )
	{
#ifdef DEBUG
		if (getSystemError())
		{
#ifndef GWA
			LPTSTR wMessage = NULL;
			wMessage = allocate( 2*strlen(lpLogText)+1, NULL);
			swprintf( wMessage, L"%hs", lpLogText );
			MessageBox( NULL, (LPCTSTR)wMessage, L"LoggerInit", MB_OK | MB_ICONINFORMATION );
			free(wMessage);
#else
			SystemDebug(APPNAME": Error opening logfile: %s\n", lpLogText);
#endif
		}
		else
		{
			SystemDebug(APPNAME": Error opening logfile\n");
		}
#endif
		return FALSE;
	}

#ifdef DEBUG
	if (setvbuf(hLogger, NULL, _IONBF, 0) != 0)
	{
		SystemDebug("Failed to make logger unbuffered");
	}
#endif

	return TRUE;
}

static void LoggerClose(void)
{
	if( hLogger != NULL )
	{
		fclose( hLogger );
		hLogger = NULL;
	}

#ifdef GWA
	if (bLoggerInit)
	{
		LeaveCriticalSection(&SafeLogging);
	}
#endif
}

void LoggerQuit(void)
{
	if (bLoggerInit)
	{
		bLoggerInit = FALSE;

		free(logFilename);
		logFilename = NULL;

#ifdef GWA
		DeleteCriticalSection(&SafeLogging);
#endif
	}
}

void Log(LPCSTR format, ...)
{
	if (!LoggerOpen())
		return;

	va_list args;
	va_start(args, format);
	_vsnprintf(lpLogText, MAX_VS_SIZE-1, format, args);
	va_end(args);

	// Only log on system in debug mode
	if (conf.debug)
		SystemDebug(lpLogText);

	fprintf( hLogger, "%sInfo  : %s\n", getTimestamp(), lpLogText );

	SetLastError(NO_ERROR);

	LoggerClose();
}

void Error(LPCSTR format, ...)
{
	if (!LoggerOpen())
		return;

	va_list args;
	va_start(args, format);
	_vsnprintf(lpLogText, MAX_VS_SIZE-1, format, args);
	va_end(args);

	// Only log error on system in debug mode
	if (conf.debug)
		SystemDebug(lpLogText);

	fprintf(hLogger, "%sError: %s\n", getTimestamp(), lpLogText);

#ifdef DEBUG
	Debug2("Errno: %d", GetLastError());
#endif

	if (getSystemError())
	{
		fprintf( hLogger, "%sLast system error: %s\n", getTimestamp(), lpLogText);
	}

	SetLastError(NO_ERROR);

	LoggerClose();
}

void Debug(LPCSTR format, ...)
{
	if (!conf.debug || !LoggerOpen())
		return;

	va_list args;
	va_start(args, format);
	_vsnprintf(lpLogText, MAX_VS_SIZE-1, format, args);
	va_end(args);

	SystemDebug(lpLogText);

	fprintf(hLogger, "%sDebug: %s\n", getTimestamp(), lpLogText);

	LoggerClose();
}

void Debug2(LPCSTR format, ...)
{
	if (conf.debug<2 || !LoggerOpen())
		return;

	va_list args;
	va_start(args, format);
	_vsnprintf(lpLogText, MAX_VS_SIZE-1, format, args);
	va_end(args);

	SystemDebug(lpLogText);

	fprintf(hLogger, "%sDebug2: %s\n", getTimestamp(), lpLogText);

	LoggerClose();
}

void DebugError(LPCSTR format, ...)
{

	if (!conf.debug || !LoggerOpen())
		return;

	va_list args;
	va_start(args, format);
	_vsnprintf(lpLogText, MAX_VS_SIZE-1, format, args);
	va_end(args);

	SystemDebug(lpLogText);

	fprintf(hLogger, "%sDebugE: %s\n", getTimestamp(), lpLogText);

	SetLastError(NO_ERROR);

	LoggerClose();
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
