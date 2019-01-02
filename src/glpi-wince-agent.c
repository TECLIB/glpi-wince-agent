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
#include <windowsx.h>
#include <commctrl.h>
#include <shellapi.h>

#include "glpi-wince-agent.h"

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
	swprintf( wAgentName, L"%hs", AgentName );

	wClassName = allocate((strlen(AppName)+1)*2, "wClassName");
	swprintf( wClassName, L"%hs", AppName );

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
			CW_USEDEFAULT, CW_USEDEFAULT,
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

static void setupMainPanel(HWND hWnd)
{
	LPWSTR buffer = NULL;
	int len;

#ifdef DEBUG
			Debug2("Setting up panel...");
#endif

	bar = CommandBar_Create(hi, hWnd, 1);
	CommandBar_InsertMenubarEx(bar, hi, MAKEINTRESOURCE(IDR_MAINMENU), 0);
	CommandBar_AddAdornments(bar, CMDBAR_OK, 0);

	dialog = CreateDialog(hi,MAKEINTRESOURCE(IDR_MAINDIALOG), hWnd, NULL);

	// Initialize rabiobox with current debug level
	if (conf.debug == 2) {
		CheckRadioButton(dialog, IDC_DEBUG0, IDC_DEBUG2, IDC_DEBUG2);
	} else if (conf.debug == 1) {
		CheckRadioButton(dialog, IDC_DEBUG0, IDC_DEBUG2, IDC_DEBUG1);
	} else {
		CheckRadioButton(dialog, IDC_DEBUG0, IDC_DEBUG2, IDC_DEBUG0);
	}

	// Initialize server editbox
	len = conf.server ? strlen(conf.server) : 0 ;
	if (len)
	{
		buffer = allocate(++len*2, "server editbox");
		swprintf( buffer, L"%hs", conf.server );
		SendDlgItemMessage(dialog, IDC_EDIT_URL, WM_SETTEXT, 0, (LPARAM)buffer );
		free(buffer);
	}

	// Initialize local editbox
	len = conf.local ? strlen(conf.local) : 0 ;
	if (len)
	{
		buffer = allocate(++len*2, "local editbox");
		swprintf( buffer, L"%hs", conf.local );
		SendDlgItemMessage(dialog, IDC_EDIT_LOCAL, WM_SETTEXT, 0, (LPARAM)buffer );
		free(buffer);
	}

	// Initialize tag editbox
	len = conf.tag ? strlen(conf.tag) : 0 ;
	if (len)
	{
		buffer = allocate(++len*2, "tag editbox");
		swprintf( buffer, L"%hs", conf.tag );
		SendDlgItemMessage(dialog, IDC_EDIT_TAG, WM_SETTEXT, 0, (LPARAM)buffer );
		free(buffer);
	}
}

static void adjustEditControl(int res, LONG width)
{
	HWND edittext;
	RECT size;

	if (dialog == NULL)
		return;

	edittext = GetDlgItem(dialog, res);
	if (edittext)
	{
		if (GetWindowRect(edittext, &size))
		{
#ifdef DEBUG
			Debug2("EditText windows size(%d): %dx%d@%d+%d", res, size.right, size.bottom, size.left, size.top);
#endif
			// Resize control
			SetWindowPos(edittext, dialog, 0, 0, width - 2*size.left, size.bottom - size.top, SWP_NOMOVE|SWP_NOZORDER|SWP_SHOWWINDOW);
		}
	}

}

static void adjustDialogControls(HWND hWnd)
{
	RECT size;

	if (dialog == NULL)
		return;

#ifdef DEBUG
	Debug2("Adjusting control sizes...");
#endif

	if (GetClientRect(hWnd, &size) && size.right > 10)
	{
#ifdef DEBUG
		Debug2("Dialog size: %dx%d", size.right, size.bottom);
#endif
		adjustEditControl(IDC_EDIT_URL  , size.right);
		adjustEditControl(IDC_EDIT_LOCAL, size.right);
		adjustEditControl(IDC_EDIT_TAG  , size.right);
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
		case WM_SIZE:
			adjustDialogControls(hWnd);
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

	if (SendDlgItemMessage(dialog, IDC_DEBUG2, BM_GETCHECK, 0, 0 ) == BST_CHECKED) {
		Debug("Updating debug config to debug2");
		conf.debug = 2;
	} else if (SendDlgItemMessage(dialog, IDC_DEBUG1, BM_GETCHECK, 0, 0 ) == BST_CHECKED) {
		Debug("Updating debug config to debug1");
		conf.debug = 1;
	} else {
		Debug("Updating debug config to no debug");
		conf.debug = 0;
	}
}

void DoMenuActions(HWND w, INT id)
{
	switch (id) {
		case IDOK:
			keepConfig();
			ConfigSave();
			Refresh(); // Ask service to refresh its configuration
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
			Refresh(); // Ask service to refresh its configuration
			break;
#ifdef DEBUG
		case IDM_MENU_DEBUGINVENTORY:
			RunDebugInventory();
			break;
#endif
	}
}

void Abort(void)
{
	Log("Aborting...");
	Quit();
	PostQuitMessage(WM_QUIT);
	exit(EXIT_FAILURE);
}
