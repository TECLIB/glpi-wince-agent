
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
const WCHAR *AgentName;
const char  *InstallPath;
const char  *ConfigFile;

/*
 * inventory.c
 */

/*
 * logger.c
 */
void LoggerInit(void);
void LoggerQuit(void);
void Log(const char *message);
void Error(const char *message);
void Debug(const char *message);
void Debug2(const char *message);

/*
 * target.c
 */

/*
 * tools.c
 */
