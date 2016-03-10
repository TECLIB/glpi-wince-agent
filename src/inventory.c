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

INVENTORY *Inventory = NULL;

static void getTag(void);

void RunInventory(void)
{
	// Free any previously done inventory and allocate fresh memory
	FreeInventory();
	Inventory = allocate( sizeof(INVENTORY), "Inventory" );
	Inventory = createList( "CONTENT" );

	// Get board details
	getHardware();
	getBios();

	// Add client version
	addEntry( Inventory, "CONTENT", "VERSIONCLIENT", (LPVOID)AgentName );

	// Add TAG information
	getTag();

	// Add network adapters
	getNetworks();
}

INVENTORY *getInventory(void)
{
	return Inventory;
}

LIST *createList( LPCSTR key )
{
	return createEntry( key , NULL, NULL, NULL );
}

void addField( ENTRY *node, LPCSTR key, LPSTR what )
{
	addEntry( node, "", key, (LPVOID)what );
}

ENTRY *createEntry( LPCSTR key, LPSTR value, LPVOID leaf, LPVOID next )
{
	ENTRY *entry = NULL;

	entry = allocate( sizeof(INVENTORY), key );
	entry->key = allocate( strlen(key)+1, "createEntry key" );
	strcpy( entry->key, key );
	entry->value = value;
	entry->leaf = leaf;
	entry->next = next;
	// Anyway allocate new string for value if set
	if (value != NULL)
	{
		entry->value = allocate( strlen(value)+1, "Entry value" );
		strcpy( entry->value, value );
	}
	return entry;
}

void addEntry( ENTRY *node, LPCSTR path, LPCSTR key, LPVOID what )
{
	ENTRY *entry = NULL, *prev = NULL, *cursor = NULL;
	LPSTR slash = NULL;
	int cmp, len;

	if (node == NULL)
	{
		Error("Can't insert %s inventory node under %s, inventory not initialiazed");
		return;
	}

	cursor = (ENTRY *)node->leaf;
	if (path != NULL)
		slash = strstr(path, "/");
	if (slash == NULL)
	{
		if ( key == NULL )
		{
			// While called with addEntry( node, entry->key, NULL, entry ),
			// connect an entry as a new "path" leaf while key is empty
			entry = (ENTRY *)what;
			if (strcmp( entry->key, path ) != 0)
			{
				Error("Don't know how to insert %s entry", path);
				return;
			}
		}
		else
		{
			entry = createEntry((LPCSTR)key, what, NULL, NULL);
		}

		// Find where to insert entry in the list
		if (cursor == NULL)
		{
			// Just insert as leaf if list is empty
			node->leaf = (LPVOID)entry;
		}
		else
		{
			// Compare key to find insertion pos
			while ((cmp = strcmp(cursor->key, entry->key))<0 && cursor->next != NULL)
			{
				prev   = cursor;
				cursor = (ENTRY *)prev->next;
			}
			if (cmp>=0)
			{
				// Insert before cursor
				if (prev == NULL)
					node->leaf = (LPVOID)entry;
				else
					prev->next = (LPVOID)entry;
				entry->next = (LPVOID)cursor;
			}
			else
			{
				// Insert after cursor
				entry->next = cursor->next;
				cursor->next = (LPVOID)entry;
			}
		}
	}
	else
	{
		LPSTR leafkey = NULL;

		// Find next entry point in the tree
		len = strlen(path) - strlen(slash);

		while ( cursor != NULL && strncmp(path, cursor->key, len) != 0 )
			cursor = (ENTRY *)cursor->next;

		if (cursor == NULL)
		{
			// Create new leaf
			leafkey = allocate( len+1, NULL);
			strncpy( leafkey, path, len);
			leafkey[len] = '\0';
			cursor = createEntry((LPCSTR)leafkey, NULL, NULL, NULL);
			free(leafkey);
		}

		// Recursive call
		addEntry( cursor, ++slash, key, what );
	}
}

void InventoryAdd(LPCSTR key, ENTRY *node)
{
	addEntry( Inventory, key, NULL , node );
}

static void getTag(void)
{
	LIST *AccountInfo = NULL;

	if (!strlen(conf.tag))
		return;

	// Initialize ACCOUNTINFO list
	AccountInfo = createList("ACCOUNTINFO");

	// Add TAG information
	addField( AccountInfo, "KEYNAME", "TAG" );
	addField( AccountInfo, "KEYVALUE", conf.tag );

	// Insert AccountInfo in inventory
	InventoryAdd( "ACCOUNTINFO", AccountInfo );
}

#ifdef DEBUG
void DebugEntry(ENTRY *entry, LPCSTR parent)
{
	int buflen = 16; // Be large
	LPSTR path = NULL;

	if (entry->key == NULL)
		Error("Found illegal entry with NULL key");
	else
		buflen += strlen(entry->key);
	if (parent != NULL)
		buflen += strlen(parent) + 1;
	path = allocate(buflen, NULL);
	if (parent == NULL)
		strcpy( path, entry->key);
	else
		sprintf( path, "%s/%s", parent, entry->key );
	if (entry->value != NULL)
		Debug("Entry value: %s='%s'", path, entry->value);
	else
		Debug("Entry path : %s", path);
	if (entry->leaf != NULL)
		DebugEntry(entry->leaf, path);
	if (entry->next != NULL)
		DebugEntry(entry->next, parent);
	free(path);
}

void DebugInventory(void)
{
	DebugEntry((ENTRY *)Inventory, NULL);
}
#endif

void FreeEntry(ENTRY *entry)
{
	if (entry == NULL)
		return;
	FreeEntry(entry->next);
	FreeEntry(entry->leaf);
	free(entry->key);
	free(entry->value);
	free(entry);
}

void FreeInventory(void)
{
	FreeEntry(Inventory);
}
