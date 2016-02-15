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
#include <commctrl.h>

#include "glpi-wince-agent.h"

LRESULT CALLBACK WndProcedure(HWND, UINT, WPARAM, LPARAM);
void DoMenuActions(HWND w, INT id);

HINSTANCE	hi;

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

	hWnd = CreateWindow(wClassName, wAgentName, WS_OVERLAPPEDWINDOW, 0, 0,
			CW_USEDEFAULT, CW_USEDEFAULT,
			NULL, NULL,
			hInstance,
			NULL);

	if (! hWnd)
		return FALSE;

	ShowWindow(hWnd, SW_SHOWNORMAL);
	UpdateWindow(hWnd);

	while (GetMessageW(&Msg, NULL, 0, 0)) {
		TranslateMessage(&Msg);
		DispatchMessageW(&Msg);
	}

	free(wAgentName);
	free(wClassName);

	return Msg.wParam;

	exit(0);
}

LRESULT CALLBACK WndProcedure(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	HWND		bar;

	switch (Msg) {
		case WM_CREATE:
			bar = CommandBar_Create(hi, hWnd, 1);
			CommandBar_InsertMenubar(bar, hi, IDR_MAINMENU, 0);
			CommandBar_AddAdornments(bar, 0, 0);
			break;
		case WM_SHOWWINDOW:
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

void DoMenuActions(HWND w, INT id)
{
	switch (id) {
#ifdef DEBUG
		case IDM_MENU_DEBUGINVENTORY:
			DebugInventory();
			break;
#endif
		case IDM_MENU_EXIT:
			Quit();
			PostQuitMessage(WM_QUIT);
			break;
		case IDM_MENU_RUN:
			Run();
			break;
		case IDM_MENU_DOINVENTORY:
			RunInventory();
			break;
	}
}

void Abort(void)
{
	Log("Aborting...");
	Quit();
	PostQuitMessage(WM_QUIT);
	exit(EXIT_FAILURE);
}
