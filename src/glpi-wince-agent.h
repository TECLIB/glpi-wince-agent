
#define VERSION			"0.1"

#define IDR_MAINMENU	101
#define IDM_MENU_EXIT	201
#define IDM_MENU_RUN	202

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
const WCHAR *ClassName;
LPCSTR AgentName;
LPCSTR InstallPath;
LPCSTR ConfigFile;

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
 * target.c
 */

/*
 * tools.c
 */
void *allocate(ULONG size, LPCSTR reason );
