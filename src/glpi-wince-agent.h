
#define EDITOR  "Teclib"
#define APPNAME "GLPI-Agent"

#define MAJOR_VERSION      1
#define MINOR_VERSION      6

#define DEFAULTVARDIR      "var"
#define DEFAULTINSTALLPATH "\\Program Files\\" APPNAME
#define JOURNALBASENAME    "service.txt"
#define INTERFACEBASENAME  "client.txt"

#ifdef STDERR
#define STDERRFILE         "\\stderr.txt"
#endif

#define STRING(s)			#s
#define XSTRING(s)			STRING(s)

#define VERSION				XSTRING(MAJOR_VERSION.MINOR_VERSION)
#define WEDITOR				L"Teclib"
#define WAPPNAME			L"GLPI-Agent"

/*
 * Resources IDs
 */
#define IDR_MAINICON			101
#define IDR_MAINMENU			102
#define IDR_MAINDIALOG			103
#define IDR_CPLDIALOG			104

#define IDM_MENU_EXIT			201
#define IDM_MENU_RUN			202
#define IDM_MENU_SAVECONFIG		203

#define IDS_APP_TITLE			501
#define IDS_DESCRIPTION			502

#ifdef DEBUG
#define IDM_MENU_DEBUGINVENTORY	901
#endif

#define IDC_EDIT_URL			1001
#define IDC_EDIT_LOCAL			1002
#define IDC_EDIT_TAG			1004
#define IDC_DEBUG0				1010
#define IDC_DEBUG1				1011
#define IDC_DEBUG2				1012

#define IDC_STATIC (-1)

// Define expiration delay to 10 minutes related to GetTickCount() API
#define EXPIRATION_DELAY  (DWORD)600000

// Define target initial and max delay as minutes
#define DEFAULT_MAX_DELAY        4*60
#define DEFAULT_INITIAL_DELAY    10

// Define minimal wait time in milli-seconds between checks if time to run
#define DEFAULT_MINIMAL_SLEEP    60000

// Define GLPI connection timeout in milliseconds
#define GLPI_CONNECT_TIMEOUT     1000

#include <iphlpapi.h>

#include "sdk-extracted.h"

// Some macros
#define lpstrcmp(x,y) \
	((x == NULL && y == NULL)||(x != NULL && y != NULL && !strcmp(x,y)))

#define OpenedKey(x,y,z) RegOpenKeyEx(x,y,0,0,z)==ERROR_SUCCESS

/*
 * agent.c
 */
DWORD maxDelay;
FILETIME nextRunDate;
void Init(void);
void Start(void);
void Stop(void);
void Refresh(void);
#ifdef GWA
void Run(BOOL force);
void Resume(void);
void Suspend(void);
#else
void RequestRun(void);
#endif
void Quit(void);
LPSTR getCurrentPath(void);
void RunDebugInventory(void);

/*
 * config.c
 */
typedef struct {
	LPSTR server;		// GLPI server URL
	LPSTR local;		// local path
	LPSTR tag;			// client tag
	int debug;			// debug level
	BOOL  loaded;		// flag telling config has been loaded
} CONFIG;

CONFIG conf;

CONFIG ConfigLoad(LPSTR path);
void ConfigSave(void);
void ConfigQuit(void);

/*
 * constants.c
 */
LPCSTR AppName;
LPCSTR AgentName;
LPCSTR InstallPath;
LPCSTR DefaultConfigFile;
LPCSTR DefaultVarDir;

/*
 * glpi-wince-agent.c
 */
#ifndef GWA
void Abort(void);
#endif

/*
 * inventory.c
 */
typedef struct {
	LPSTR key;
	LPSTR value;
	LPVOID leaf;
	LPVOID next;
} ENTRY, LIST, INVENTORY;

void RunInventory(void);
void FreeInventory(void);
void InventoryAdd(LPCSTR key, ENTRY *node);
INVENTORY *getInventory(void);

// List tools
ENTRY *createEntry(LPCSTR key, LPSTR value, LPVOID leaf, LPVOID next);
void addEntry(ENTRY *node, LPCSTR path, LPCSTR key, LPVOID what);
LIST *createList(LPCSTR key);
void addField(ENTRY *node, LPCSTR key, LPSTR what);
void FreeEntry(ENTRY *entry);

void DebugInventory(void);
void DebugEntry(ENTRY *entry, LPCSTR parent);

/*
 * inventory/board.c
 */
typedef struct _MANUFACTURER {
	DWORD id;
	LPCSTR name;
} MANUFACTURER, *LPMANUFACTURER;

LPSTR getUUID(BOOL tryDeprecated);
void getBios(void);
void getHardware(void);
void CheckDeviceId(LPMANUFACTURER manufacturer, LIST *list);

/*
 * inventory/network.c
 */
void getNetworks(void);

/*
 * logger.c
 */
void SystemDebug(LPCSTR format, ...);
void LoggerInit(void);
void LoggerQuit(void);
void Log(LPCSTR format, ...);
void Error(LPCSTR format, ...);
void Debug(LPCSTR format, ...);
void Debug2(LPCSTR format, ...);
void DebugError(LPCSTR format, ...);
void RawDebug(LPCSTR format, LPBYTE buffer, ULONG size);
#ifdef STDERR
FILE *hStdErr;
void stderrf(LPCSTR format, ...);
#endif

/*
 * storage.c
 */
LPSTR VarDir;
void StorageInit(LPCSTR path);
void StorageQuit(void);
LPSTR loadState(void);
void saveState(LPSTR deviceid);

/*
 * target.c
 */
void TargetInit(LPSTR deviceid);
void TargetQuit(void);
void WriteLocal(LPSTR deviceid);
BOOL SendRemote(LPSTR deviceid);

/*
 * tools.c
 */
void *allocate(ULONG size, LPCSTR reason );
void ToolsInit(void);
void ToolsQuit(void);
PFIXED_INFO getNetworkParams(void);
LPSTR getHostname(void);
LPSTR getTimestamp(void);
void computeNextRunDate(void);
BOOL timeToSubmit(void);
LPSYSTEMTIME getLocalTime(void);
LPSTR getRegString( HKEY hive, LPCSTR subkey, LPCSTR value );
DWORD getRegValue( HKEY hive, LPCSTR subkey, LPCSTR value );
LPSTR getRegPath( LPCSTR value );
LPSTR vsPrintf( LPCSTR fmt, ... );
LPSTR hexstring(BYTE *addr, int addrlen);
void checkMemory(void);
