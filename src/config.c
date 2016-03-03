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

#ifndef DEBUG
#define DEBUG 0
#endif

CONFIG conf = {
	NULL,  // .server : GLPI Server URL
	NULL,  // .local  : Local inventory folder
	DEBUG, // .debug  : Default debug mode
	FALSE
};

LPSTR configFile = NULL;

CONFIG ConfigLoad(LPSTR path)
{
	FILE *hConfig;
	LPSTR buffer, found, start;
	CONFIG config = { NULL, NULL, DEBUG, FALSE };

	// Initialize config file path
	if (path != NULL)
	{
		configFile = allocate(strlen(path)+strlen(DefaultConfigFile)+2, "config file");
		sprintf(configFile, "%s\\%s", path, DefaultConfigFile);
		Debug("Config file: %s", configFile);
	}

	// Handle case no server & no local conf found in debug build
	// By default, dump local inventory where agent is started in debug release
#ifdef DEBUG
	if (config.server == NULL && config.local == NULL)
	{
		config.local = allocate(4, NULL);
		*config.local = '\0';
		strcat(config.local, ".");
	}
#endif

	if (configFile == NULL)
	{
		Error("Can't load not defined configuration filename");
		return config;
	}

	hConfig = fopen( configFile, "r" );
	if( hConfig == NULL )
	{
		config.loaded = FALSE;
		DebugError("Can't read config from %s file", configFile);
		return config;
	}

#define MAX_LENGHT 256
	buffer = allocate(MAX_LENGHT, NULL);
	*buffer = '\0';
	while (
		(fgets(buffer, MAX_LENGHT-1, hConfig) != NULL || !feof(hConfig))
	)
	{
		// Strip buffer
		start = strchr(buffer, '\n');
		if (start != NULL)
			*start = '\0';

		// Leave while at the end of config file and buffer is empty
		if (feof(hConfig) && !strlen(buffer))
			break;

		// Skip spaces
		start = buffer;
		while (*start == ' ')
			start++;

		// Skip comments & empty lines
		if (*start == '#' || *start == '/' || *start == ';' || !strlen(start))
			continue;

		// Test if we found debug level configuration
		found = strstr(buffer, "debug");
		if (found != NULL && found == start)
		{
			if (sscanf(found, "debug = %d", &config.debug) != 1)
				Error("Failed to read debug level from config file in: %s", buffer);
			continue;
		}

		// Test if we found server configuration
		found = strstr(buffer, "server");
		if (found != NULL && found == start)
		{
			found += 7;
			while (*found == ' ' || *found == '=')
				found ++;
			free(config.server);
			if (!strlen(found))
			{
				Debug("Empty string found for server");
				config.server = NULL;
			}
			else
			{
				config.server = allocate(strlen(found)+1, "Server config");
				*config.server = '\0';
				strcat(config.server, found);
				Debug("Server in config: %s", config.server);
			}
			continue;
		}

		// Test if we found local configuration
		found = strstr(buffer, "local");
		if (found != NULL && found == start)
		{
			found += 5;
			while (*found == ' ' || *found == '=')
				found ++;
			free(config.local);
			if (!strlen(found))
			{
				Debug("Empty string found for local");
				config.local = NULL;
			}
			else
			{
				config.local = allocate(strlen(found)+1, "Local config");
				*config.local = '\0';
				strcat(config.local, found);
				Debug("Local in config: %s", config.local);
			}
			continue;
		}

		Debug("Found unsupported conf line: %s", buffer);
	}
	free(buffer);
	fclose(hConfig);

	config.loaded = TRUE;

	// Check config
	if (config.debug < 0)
		config.debug = 1;
	else if (config.debug>2)
		config.debug = 2;

	return config;
}

void ConfigSave(void)
{
	FILE *hConfig;
	CONFIG config;

	if (configFile == NULL)
	{
		Debug2("configuration file name not defined");
		return;
	}

	config = ConfigLoad(NULL);
	if (	config.loaded
			&& conf.debug == config.debug
			&& !strcmp(conf.server, config.server)
			&& !strcmp(conf.local, config.local)
		)
	{
		Debug("Configuration didn't change, no need to save it");
		Debug2("No change? debug: %d vs %d", conf.debug, config.debug);
		Debug2("No change? server: %s vs %s", conf.server, config.server);
		Debug2("No change? local: %s vs %s", conf.local, config.local);
		return;
	}

	Debug("Configuration has changed, saving it");

	hConfig = fopen( configFile, "w" );
	if( hConfig == NULL )
	{
		Error("Can't write configuration in %s file", configFile);
		return;
	}

	if (conf.server != NULL)
		fprintf(hConfig, "server = %s\n", conf.server);
	else
		fprintf(hConfig, "server = \n");

	if (conf.local != NULL)
		fprintf(hConfig, "local = %s\n", conf.local);
	else
		fprintf(hConfig, "local = \n");

	fprintf(hConfig, "debug = %d\n", conf.debug);
	fclose(hConfig);
}

void ConfigQuit(void)
{
	free(configFile);
	free(conf.server);
	free(conf.local);
}
