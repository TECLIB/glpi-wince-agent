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

#include <stdio.h>
#include <windows.h>
//#include <windowsx.h>
//#include <commctrl.h>
//#include <shellapi.h>

#include "glpi-wince-agent.h"

#include "cpl.h" // Control Panel support.

// Returns the number of characters in an expression.
#define lengthof(exp) ((sizeof((exp)))/sizeof((*(exp))))

#ifdef __CPL__
LRESULT CALLBACK WndProcedure(HWND, UINT, WPARAM, LPARAM);
void DoMenuActions(HWND w, INT id);

HINSTANCE	hi = NULL;
HWND		bar = NULL;
HWND		dialog = NULL;

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	WNDCLASS	WndCls;
	HWND		hWnd;
	MSG			Msg;
	LPTSTR		wAgentName;
	LPTSTR		wClassName;

	Init();

	hi = hInstance;

	wAgentName = allocate((strlen(AgentName)+1)*2, "wAgentName");
	wsprintf( wAgentName, L"%hs", AgentName );

	wClassName = allocate((strlen(AppName)+1)*2, "wClassName");
	wsprintf( wClassName, L"%hs", AppName );

	// Prevent application from starting twice
	hWnd = FindWindow(wClassName, wAgentName);
	if (hWnd != NULL)
	{
		SetForegroundWindow(hWnd);
		return FALSE;
	}

	WndCls.style			= CS_HREDRAW | CS_VREDRAW;
	WndCls.lpfnWndProc		= WndProcedure;
	WndCls.cbClsExtra		= 0;
	WndCls.cbWndExtra		= 0;
	WndCls.hIcon			= NULL;
	WndCls.hCursor			= NULL;
	WndCls.hbrBackground	= (HBRUSH)GetStockObject(WHITE_BRUSH);
	WndCls.lpszMenuName		= NULL;
	WndCls.lpszClassName	= wClassName;
	WndCls.hInstance		= hInstance;

	RegisterClass(&WndCls);

	Debug2("Windows class registered");

	hWnd = CreateWindowEx(
			0,
			wClassName, wAgentName,
			WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
			CW_USEDEFAULT, CW_USEDEFAULT,
			240, 240,
			NULL, NULL,
			hInstance,
			NULL);

	free(wAgentName);
	free(wClassName);

	if (! hWnd)
	{
		Debug2("Main window NOT created");

		Quit();

		Msg.wParam = FALSE;
	}
	else
	{
		Debug2("Main window created");

		ShowWindow(hWnd, nCmdShow);
		UpdateWindow(hWnd);

		while (GetMessageW(&Msg, NULL, 0, 0)) {
			TranslateMessage(&Msg);
			DispatchMessageW(&Msg);
		}
	}

	return Msg.wParam;
}

static BOOL DoConfigDebug(INT what, HWND dialog)
{
	LPARAM wIndex;

	switch (what) {
		case CBN_SELCHANGE:
#ifdef DEBUG
			Log("Got Debug CBN_SELCHANGE");
#endif
			wIndex = SendDlgItemMessage(dialog, IDC_DEBUG_CONFIG, CB_GETCURSEL, 0, 0 );
#ifdef DEBUG
			Log("Debug level to be set to %i", wIndex);
#endif
			conf.debug = wIndex;
			return TRUE;
	}
	return FALSE;
}

static BOOL DoDialogActions(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg) {
		case WM_COMMAND:
#ifdef DEBUG
			Log("Got Dialog WM_COMMAND: %d -> %d", LOWORD(wParam), HIWORD(wParam));
#endif
			switch (LOWORD(wParam)) {
				case IDC_DEBUG_CONFIG:
					return DoConfigDebug(HIWORD(wParam), hWnd);
					break;
			}
		break;
	}
	return FALSE;
}

static void setupMainPanel(HWND hWnd)
{
	HWND combo;
	CHAR value[16];
	LPWSTR buffer = NULL;
	int i, len;

#ifdef DEBUG
			Debug2("Setting up panel...");
#endif

	bar = CommandBar_Create(hi, hWnd, 1);
	CommandBar_AddAdornments(bar, CMDBAR_OK, 0);
	CommandBar_InsertMenubarEx(bar, hi, MAKEINTRESOURCE(IDR_MAINMENU), 0);

	dialog = CreateDialog(hi,MAKEINTRESOURCE(IDR_MAINDIALOG), hWnd, DoDialogActions);
	combo = GetDlgItem(dialog, IDC_DEBUG_CONFIG);

	// Setup Debug Level Combo
	for ( i=0 ; i <= 2 ; i++ )
	{
		sprintf( value, "%d", i );
		(void)ComboBox_AddString(combo, value);
	}

	// Initialize combo with current debug level
	SendDlgItemMessage(dialog, IDC_DEBUG_CONFIG, CB_SETCURSEL, conf.debug, 0 );

	// Initialize server editbox
	len = conf.server ? strlen(conf.server) : 0 ;
	if (len)
	{
		buffer = allocate(++len*2, "server editbox");
		wsprintf( buffer, L"%hs", conf.server );
		SendDlgItemMessage(dialog, IDC_EDIT_URL, WM_SETTEXT, 0, (LPARAM)buffer );
		free(buffer);
	}

	// Initialize local editbox
	len = conf.local ? strlen(conf.local) : 0 ;
	if (len)
	{
		buffer = allocate(++len*2, "local editbox");
		wsprintf( buffer, L"%hs", conf.local );
		SendDlgItemMessage(dialog, IDC_EDIT_LOCAL, WM_SETTEXT, 0, (LPARAM)buffer );
		free(buffer);
	}

	// Initialize tag editbox
	len = conf.tag ? strlen(conf.tag) : 0 ;
	if (len)
	{
		buffer = allocate(++len*2, "tag editbox");
		wsprintf( buffer, L"%hs", conf.tag );
		SendDlgItemMessage(dialog, IDC_EDIT_TAG, WM_SETTEXT, 0, (LPARAM)buffer );
		free(buffer);
	}
}

LRESULT CALLBACK WndProcedure(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg) {
		case WM_CREATE:
			setupMainPanel(hWnd);
			break;
		case WM_SHOWWINDOW:
			break;
		case WM_CLOSE:
			DestroyWindow(hWnd);
			break;
		case WM_DESTROY:
			Quit();
			PostQuitMessage(WM_QUIT);
			break;
		case WM_COMMAND:
			/* Handle the menu items */
			DoMenuActions(hWnd, LOWORD(wParam));
			break;
		default:
			return DefWindowProc(hWnd, Msg, wParam, lParam);
	}
	return 0;
}

static void keepConfig(void)
{
	int size = 0;
	LPWSTR buffer = NULL;

	if (dialog == NULL)
		return;

	if (SendDlgItemMessage(dialog, IDC_EDIT_URL, EM_GETMODIFY, 0, 0 ))
	{
		Debug("Updating server config");
		free(conf.server);
		size = SendDlgItemMessage(dialog, IDC_EDIT_URL, WM_GETTEXTLENGTH, 0, 0 );
		if (size>0)
		{
			conf.server = allocate( ++size, "Server config");
			buffer = allocate( size*2, "Server config buffer");
			SendDlgItemMessage(dialog, IDC_EDIT_URL, WM_GETTEXT, (WPARAM)size, (LPARAM)buffer );
			wcstombs(conf.server, buffer, size);
			free(buffer);
			Debug("Updated server config: %s", conf.server);
		}
		else
		{
			conf.server = NULL;
		}
	}

	if (SendDlgItemMessage(dialog, IDC_EDIT_LOCAL, EM_GETMODIFY, 0, 0 ))
	{
		Debug("Updating local config");
		free(conf.local);
		size = SendDlgItemMessage(dialog, IDC_EDIT_LOCAL, WM_GETTEXTLENGTH, 0, 0 );
		if (size>0)
		{
			conf.local = allocate( ++size, "Local config");
			buffer = allocate( size*2, "Local config buffer");
			SendDlgItemMessage(dialog, IDC_EDIT_LOCAL, WM_GETTEXT, (WPARAM)size, (LPARAM)buffer );
			wcstombs(conf.local, buffer, size);
			free(buffer);
			Debug("Updated local config: %s", conf.local);
		}
		else
		{
			conf.local = NULL;
		}
	}

	if (SendDlgItemMessage(dialog, IDC_EDIT_TAG, EM_GETMODIFY, 0, 0 ))
	{
		Debug("Updating tag config");
		free(conf.tag);
		size = SendDlgItemMessage(dialog, IDC_EDIT_TAG, WM_GETTEXTLENGTH, 0, 0 );
		if (size>0)
		{
			conf.tag = allocate( ++size, "Tag config");
			buffer = allocate( size*2, "Tag config buffer");
			SendDlgItemMessage(dialog, IDC_EDIT_TAG, WM_GETTEXT, (WPARAM)size, (LPARAM)buffer );
			wcstombs(conf.tag, buffer, size);
			free(buffer);
			Debug("Updated tag config: %s", conf.tag);
		}
		else
		{
			conf.tag = NULL;
		}
	}
}

void DoMenuActions(HWND w, INT id)
{
	switch (id) {
		case IDOK:
			RequestRun();
			Quit();
			PostQuitMessage(WM_QUIT);
			break;
		case IDM_MENU_EXIT:
			Quit();
			PostQuitMessage(WM_QUIT);
			break;
		case IDM_MENU_RUN:
			RequestRun();
			break;
		case IDM_MENU_SAVECONFIG:
			keepConfig();
			ConfigSave();
			Refresh(); // Ask service to refresh the con
			break;
#ifdef DEBUG
		case IDM_MENU_DEBUGINVENTORY:
			RunDebugInventory();
			break;
#endif
	}
}
#endif

HMODULE g_hModule = NULL;   // Handle to the DLL.

/**
 * Logging functions
 */
#ifdef TEST
void _debug(LPCSTR format, ...)
{
	SYSTEMTIME ts ;
	LPCSTR hdr = APPNAME "-ControlPanel (Built: " __DATE__ ", " __TIME__ ")";

	GetLocalTime(&ts);
	fprintf(stderr, "%4d-%02d-%02d %02d:%02d:%02d: %s: ",
		ts.wYear, ts.wMonth, ts.wDay, ts.wHour, ts.wMinute, ts.wSecond, hdr);

	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
	va_end(args);
}
#else
#define _debug(...)
#endif

BOOL APIENTRY
DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
			_debug("DllMain: loading");
			g_hModule = (HMODULE) hModule;
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			_debug("DllMain: other");
			break;
	}

	return TRUE;
}

LONG CALLBACK
CPlApplet(HWND hwndCPL, UINT dwReason, LPARAM lParam1, LPARAM lParam2)
{
	switch (dwReason)
	{
		case CPL_INIT:
			// Perform global initializations, memory allocations
			// Return 1 for success or 0 for failure.
			// Control Panel does not load if failure is returned.
			_debug("CPlApplet: init");
			return 1;

		case CPL_GETCOUNT:
			// The number of actions supported by this Control
			// Panel application.
			_debug("CPlApplet: getcount");
			return 1;

		case CPL_NEWINQUIRE:
			// This message is sent once for each dialog box, as
			// determined by the value returned from CPL_GETCOUNT.
			// lParam1 is the 0-based index of the dialog box.
			// lParam2 is a pointer to the NEWCPLINFO structure.
			_debug("CPlApplet: newinquire, %u", lParam1);
			ASSERT(0 == lParam1);
			ASSERT(lParam2);

			NEWCPLINFO* lpNewCplInfo = (NEWCPLINFO *) lParam2;
			if (lpNewCplInfo)
			{
				lpNewCplInfo->dwSize = sizeof(NEWCPLINFO);
				lpNewCplInfo->dwFlags = 0;
				lpNewCplInfo->dwHelpContext = 0;
				lpNewCplInfo->lData = IDR_CPLDIALOG;

				// The large icon for this application. Do not free this 
				// HICON; it is freed by the Control Panel infrastructure.
				lpNewCplInfo->hIcon = LoadIcon(g_hModule, MAKEINTRESOURCE(IDR_MAINICON));

				LoadString(g_hModule, IDS_APP_TITLE, lpNewCplInfo->szName,
				           lengthof(lpNewCplInfo->szName));
				LoadString(g_hModule, IDS_DESCRIPTION, lpNewCplInfo->szInfo,
				           lengthof(lpNewCplInfo->szInfo));
				return 0;
			}
			return 1;  // Nonzero value means CPlApplet failed.

		case CPL_DBLCLK:
			// The user has double-clicked the icon for the
			// dialog box in lParam1 (zero-based).
			_debug("CPlApplet: dblclk");

			// TODO implement

			{
				return 0;
			}
			return 1;     // CPlApplet failed.

		case CPL_STOP:
			// Called once for each dialog box. Used for cleanup.
			_debug("CPlApplet: stop");
			return 0;
		case CPL_EXIT:
			// Called only once for the application. Used for cleanup.
			_debug("CPlApplet: exit");
			return 0;
		default:
			_debug("CPlApplet: default");
			return 0;
	}
	return 1;  // CPlApplet failed.
}
