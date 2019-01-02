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

#ifdef DEBUG
#define DEFAULTDEBUG 2
#else
#ifdef TEST
#ifdef GWA
#define DEFAULTDEBUG 2
#else
#define DEFAULTDEBUG 1
#endif
#else
#define DEFAULTDEBUG 0
#endif
#endif

CONFIG conf = {
	NULL,         // .server : GLPI Server URL
	NULL,         // .local  : Local inventory folder
	NULL,         // .tag    : Inventory tag
	DEFAULTDEBUG, // .debug  : Default debug mode
	FALSE         // .loaded : not loaded at start
};

LPSTR configFile = NULL;

static inline void strip(LPSTR string)
{
	LPSTR from = strchr(string, '\n');
	while (from != NULL && *from <= ' ')
		*from-- = '\0';
}

static LPSTR readvalue(LPSTR start, LPCSTR key)
{
	int len = 0;
	LPSTR found = NULL, value = NULL;

	// Test if we found server configuration
	found = strstr(start, key);
	if (found != NULL && found == start)
	{
		BOOL equal = FALSE;

		found += strlen(key);

		// Align & check we found equal sign, equal will leave false if found is empty
		while (*found != '\0')
		{
			switch (*found)
			{
				case '=':
					equal = TRUE;
				case '\t':
				case ' ':
					found ++;
					continue;
			}
			break;
		}

		if (equal)
		{
			len = strlen(found);
			value = allocate(len+1, "config value");
			strncpy(value, found, len+1);
		}
	}
	return value;
}

CONFIG ConfigLoad(LPSTR path)
{
	FILE *hConfig;
	LPSTR buffer = NULL, found, start;
	CONFIG config = { NULL, NULL, NULL, DEFAULTDEBUG, FALSE };

	// Initialize config file path
	if (configFile == NULL && path != NULL)
	{
#ifdef STDERR
		stderrf( "path='%s'", path );
#endif

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

#ifdef STDERR
	stderrf( "configFile='%s'", configFile );
#endif

	hConfig = fopen( configFile, "r" );
	if( hConfig == NULL )
	{
		// Anyway try to check if server is to be pre-loaded as a
		// default conf should be written now
		config.server = getRegString( HKEY_LOCAL_MACHINE,
			"\\Software\\"EDITOR"\\"APPNAME,
			"ServerAgentSetup");

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
		// Strip line buffer
		strip(buffer);

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
		found = strstr(start, "debug");
		if (found != NULL && found == start)
		{
			if (strlen(found) == 5)
			{
				// Set debug to 1 if we only found "debug" string on a line
				config.debug = 1;
			}
			else if (sscanf(found, "debug = %d", &config.debug) != 1)
			{
				Error("Failed to read debug level from config file in line: %s", buffer);
			}
			Debug("Debug level in config: %d", config.debug);
			continue;
		}

		// Test if we found server configuration
		found = readvalue(start, "server");
		if (found != NULL)
		{
			free(config.server);
			config.server = found;
			Debug("Server in config: %s", config.server);
			continue;
		}

		// Test if we found local configuration
		found = readvalue(start, "local");
		if (found != NULL)
		{
			free(config.local);
			config.local = found;
			Debug("Local in config: %s", config.local);
			continue;
		}

		// Test if we found tag configuration
		found = readvalue(buffer, "tag");
		if (found != NULL)
		{
			free(config.tag);
			config.tag = found;
			Debug("Tag in config: %s", config.tag);
			continue;
		}

		Debug("Found unsupported conf line: %s", buffer);
	}
	free(buffer);
	fclose(hConfig);

	Debug2("Configuration loaded");
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

	Debug2("Loading current saved config");
	config = ConfigLoad(NULL);
	if (	config.loaded
			&& conf.debug == config.debug
			&& lpstrcmp(conf.server,config.server)
			&& lpstrcmp(conf.local,config.local)
			&& lpstrcmp(conf.tag,config.tag)
		)
	{
		Debug("Configuration didn't change, no need to save it");
		Debug2("No change? debug: %d vs %d", conf.debug, config.debug);
		Debug2("No change? server: %s vs %s", conf.server, config.server);
		Debug2("No change? local: %s vs %s", conf.local, config.local);
		Debug2("No change? tag: %s vs %s", conf.tag, config.tag);
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

	if (conf.tag != NULL)
		fprintf(hConfig, "tag = %s\n", conf.tag);
	else
		fprintf(hConfig, "tag = \n");

	fprintf(hConfig, "debug = %d\n", conf.debug);
	fclose(hConfig);

	// Loaded conf is synchronized with saved one, flag it loaded
	conf.loaded = TRUE;
}

void ConfigQuit(void)
{
	Debug2("Freeing Config");
	free(configFile);
	free(conf.server);
	free(conf.local);
	free(conf.tag);

	configFile  = NULL;
	conf.server = NULL;
	conf.local  = NULL;
	conf.tag    = NULL;
}
