
#define VERSION			"0.3"

#define IDR_MAINMENU			101
#define IDM_MENU_EXIT			201
#define IDM_MENU_RUN			202
#define IDM_MENU_DOINVENTORY	203

#ifdef DEBUG
#define IDM_MENU_DEBUGINVENTORY		901
#endif

// Define expiration delay to 10 minutes related to GetTickCount() API
#define EXPIRATION_DELAY  (DWORD)600000

#include <iphlpapi.h>

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
	int debug;		// debug level
	LPSTR local;	// local paths
} CONFIG;

CONFIG conf;

/*
 * constants.c
 */
LPCSTR AppName;
LPCSTR AgentName;
LPCSTR InstallPath;
LPCSTR ConfigFile;
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
void getBios(void);
void getHardware(void);

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

/*
 * storage.c
 */
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

/*
 * tools.c
 */
void *allocate(ULONG size, LPCSTR reason );
void ToolsInit(void);
void ToolsQuit(void);
PFIXED_INFO getNetworkParams(void);
LPSTR getHostname(void);
LPSYSTEMTIME getLocalTime(void);
