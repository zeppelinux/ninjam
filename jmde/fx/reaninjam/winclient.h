/*
    NINJAM Windows Client - winclient.h
    Copyright (C) 2005 Cockos Incorporated

    NINJAM is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    NINJAM is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with NINJAM; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/



#ifndef _WINCLIENT_H_
#define _WINCLIENT_H_

#define WM_LCUSER_RESIZE WM_USER+49
#define WM_LCUSER_ADDCHILD WM_USER+50
#define WM_LCUSER_REMCHILD WM_USER+51
#define WM_LCUSER_VUUPDATE WM_USER+52
#define WM_LCUSER_REPOP_CH WM_USER+53
#define WM_RCUSER_UPDATE WM_USER+54

#include "../../../ninjam/njclient.h"

#include "../../../WDL/mutex.h"
#include "../../../WDL/wdlstring.h"
#include "../../../WDL/db2val.h"

#include "../../../WDL/win32_utf8.h"
#include "../../../WDL/win32_hidpi.h" // for mmon SetWindowPos() tweaks

#include "../../reaper_plugin.h"
#define LOCALIZE_IMPORT_PREFIX "reaninjam_"
#define LOCALIZE_IMPORT_FORCE_NOCACHE
#ifdef WANT_LOCALIZE_IMPORT_INCLUDE
#include "../../localize-import.h"
#endif
#include "../../localize.h"


extern double (*DB2SLIDER)(double x);
extern double (*SLIDER2DB)(double y);

void mkvolpanstr(char *str, double vol, double pan);
void mkvolstr(char *str, double vol);
void mkpanstr(char *str, double pan);

extern WDL_Mutex g_client_mutex;
extern WDL_FastString g_ini_file;
extern NJClient *g_client;
extern HINSTANCE g_hInst;
extern int g_done;
extern WDL_String g_topic;

#define CONFSEC "ninjam"

#define MAX_INPUTS 128
#define MAX_OUTPUTS 128
extern int g_config_num_inputs, g_config_num_outputs;

void PopulateOutputCombo(HWND hwndCombo, int sel, bool allowmono=true);

// locchn.cpp
WDL_DLGRET LocalOuterChannelListProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);


// remchn.cpp
WDL_DLGRET RemoteOuterChannelListProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

// chat.cpp
void chat_addline(const char *src, const char *text);
void chatRun(HWND hwndDlg);
void chatmsg_cb(void *userData, NJClient *inst, const char **parms, int nparms);

// license.cpp
int licensecallback(void *userData, const char *licensetext);
void licenseRun(HWND hwndDlg);


extern BOOL  (WINAPI *InitializeCoolSB)(HWND hwnd);
extern HRESULT (WINAPI *UninitializeCoolSB)(HWND hwnd);
extern int   (WINAPI *CoolSB_SetScrollInfo)(HWND hwnd, int fnBar, LPSCROLLINFO lpsi, BOOL fRedraw);
extern BOOL (WINAPI *CoolSB_GetScrollInfo)(HWND hwnd, int fnBar, LPSCROLLINFO lpsi);
extern int (WINAPI *CoolSB_SetScrollPos)(HWND hwnd, int nBar, int nPos, BOOL fRedraw);
extern int (WINAPI *CoolSB_SetScrollRange)(HWND hwnd, int nBar, int nMinPos, int nMaxPos, BOOL fRedraw);
extern BOOL (WINAPI *CoolSB_SetMinThumbSize)(HWND hwnd, UINT wBar, UINT size);

extern void (*SetWindowAccessibilityString)(HWND h, const char *, int mode);

#endif//_WINCLIENT_H_
