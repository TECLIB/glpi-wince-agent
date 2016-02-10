
#define VERSION			"0.1"

#define IDR_MAINMENU	101
#define IDM_MENU_EXIT	201
#define IDM_MENU_RUN	202

// Define expiration delay to 10 minutes related to GetTickCount() API
#define EXPIRATION_DELAY  (DWORD)600000

/*
 * agent.c
 */
void Init(void);
void Run(void);
void Quit(void);

/*
 * config.c
 */
int debugMode;

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

/*
 * logger.c
 */
void LoggerInit(LPCSTR path);
void LoggerQuit(void);
void Log(LPCSTR format, ...);
void Error(LPCSTR format, ...);
void Debug(LPCSTR format, ...);
void Debug2(LPCSTR format, ...);

/*
 * storage.c
 */
void StorageInit(LPCSTR path);
void StorageQuit(void);

/*
 * target.c
 */

/*
 * tools.c
 */
DWORD dwStartTick;
void *allocate(ULONG size, LPCSTR reason );
void ToolsQuit(void);
LPSTR getHostname(void);
LPSYSTEMTIME getLocalTime(void);
