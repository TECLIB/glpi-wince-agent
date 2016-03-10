
#define MAJOR_VERSION	0
#define MINOR_VERSION	7

#define STRING(s)			#s
#define XSTRING(s)			STRING(s)

#define VERSION				XSTRING(MAJOR_VERSION.MINOR_VERSION)

#define IDR_MAINICON			101
#define IDR_MAINMENU			102
#define IDR_MAINDIALOG			103

#define IDM_MENU_EXIT			201
#define IDM_MENU_RUN			202
#define IDM_MENU_DOINVENTORY	203
#define IDM_MENU_SAVECONFIG		204

#ifdef DEBUG
#define IDM_MENU_DEBUGINVENTORY	901
#endif

#define IDC_EDIT_URL			1001
#define IDC_EDIT_LOCAL			1002
#define IDC_DEBUG_CONFIG		1003
#define IDC_EDIT_TAG			1004

#define IDC_STATIC (-1)

// Define expiration delay to 10 minutes related to GetTickCount() API
#define EXPIRATION_DELAY  (DWORD)600000

#define GLPI_CONNECT_TIMEOUT	5000

#include <iphlpapi.h>

#include "sdk-extracted.h"

// Some macros
#define lpstrcmp(x,y) \
	((x == NULL && y == NULL)||(x != NULL && y != NULL && !strcmp(x,y)))

/*
 * agent.c
 */
void Init(void);
void Run(void);
void Quit(void);
LPSTR getCurrentPath(void);
LPSTR getDeviceID(void);

/*
 * config.c
 */
typedef struct {
	LPSTR server;		// GLPI server URL
	LPSTR local;		// local path
	LPSTR tag;			// client tag
	int debug;			// debug level
	BOOLEAN loaded;		// flag telling config has been loaded
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
void Abort(void);

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

#ifdef DEBUG
void DebugInventory(void);
void DebugEntry(ENTRY *entry, LPCSTR parent);
#endif

/*
 * inventory/board.c
 */
typedef struct _MANUFACTURER {
	DWORD id;
	LPCSTR name;
} MANUFACTURER, *LPMANUFACTURER;

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
void LoggerInit(LPCSTR path);
void LoggerQuit(void);
void Log(LPCSTR format, ...);
void Error(LPCSTR format, ...);
void Debug(LPCSTR format, ...);
void Debug2(LPCSTR format, ...);
void DebugError(LPCSTR format, ...);
void RawDebug(LPCSTR format, LPBYTE buffer, ULONG size);

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
void SendRemote(LPSTR deviceid);

/*
 * tools.c
 */
void *allocate(ULONG size, LPCSTR reason );
void ToolsInit(void);
void ToolsQuit(void);
PFIXED_INFO getNetworkParams(void);
LPSTR getHostname(void);
LPSTR getTimestamp(void);
LPSYSTEMTIME getLocalTime(void);
LPSTR vsPrintf( LPCSTR fmt, ... );
LPSTR hexstring(BYTE *addr, int addrlen);
