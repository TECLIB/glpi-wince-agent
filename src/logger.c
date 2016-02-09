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

FILE *hLogger;

void LoggerInit(void)
{
	/* Reassign "stdout" */
	hLogger = freopen( "glpi-wince-agent.txt", "w", stdout );

	if( hLogger == NULL )
		Error( "Can't reopen stdout toward file" );
}

void LoggerQuit(void)
{
	Log( "Stopping logger..." );
	if( hLogger != NULL )
		fclose( hLogger );
	Log( "Logger stopped" );
}

void Log(const char *message)
{
	fprintf( stdout, "Info  : %s\n", message );
}

void Error(const char *message)
{
	fprintf( stdout, "Error : %s\n", message );
	fprintf( stderr, "Error : %s\n", message );
}

void Debug(const char *message)
{
	if (debugMode)
		fprintf( stdout, "Debug : %s\n", message );
}

void Debug2(const char *message)
{
	if (debugMode>1)
		fprintf( stdout, "Debug2: %s\n", message );
}
