
#define VERSION			"0.1"

#define IDR_MAINMENU	101
#define IDM_MENU_EXIT	201
#define IDM_MENU_RUN	202

// Define expiration delay to 10 minutes related to GetTickCount() API
#define EXPIRATION_DELAY  (DWORD)600000

#include <iphlpapi.h>

/*
 * agent.c
 */
LPSTR DeviceID;
void Init(void);
void Run(void);
void Quit(void);

/*
 * config.c
 */
typedef struct {
	int debug;
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
	LPSTR name;
	LPSTR serialnumber;
	LPSTR ipaddress;
	LPSTR macaddress;
} INVENTORY;

INVENTORY *Inventory;

void RunInventory(void);
void FreeInventory(void);

/*
 * inventory/board.c
 */
LPSTR getSerialNumber(void);

/*
 * inventory/network.c
 */
LPSTR getIPAddress(void);
LPSTR getMACAddress(void);

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
void loadState(void);
void saveState(void);

/*
 * target.c
 */

/*
 * tools.c
 */
DWORD dwStartTick;
void *allocate(ULONG size, LPCSTR reason );
void ToolsQuit(void);
PFIXED_INFO getNetworkParams(void);
LPSTR getHostname(void);
LPSYSTEMTIME getLocalTime(void);
