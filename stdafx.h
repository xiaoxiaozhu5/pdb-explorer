// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define WINVER         0x0600
#define _WIN32_WINNT   0x0600
#define _WIN32_IE      0x0700
#define _RICHEDIT_VER  0x0300

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS

#include <atlbase.h>
#include <atlapp.h>

extern CAppModule _Module;

#include <atlwin.h>
#include <atlcom.h>
#include <atlmisc.h>
#include <atlcoll.h>
#include <atldlgs.h>
#include <atlsplit.h>
#include <atlctrls.h>
#include <atlctrlx.h>
#include <atltheme.h>
#include <atlctrlw.h>
#include <atlwfile.h>

//#pragma warning( disable : 4192 )
//#import "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\DIA SDK\\bin\\msdia140.dll" \
//   no_namespace named_guids no_implementation raw_interfaces_only auto_rename

#include "Thread.h"

#include "dia2.h"

#include <tchar.h>
#include "log.h"

#define WM_SET_STATUS_TEXT (WM_USER + 1)
#define WM_ADD_HISTORY     (WM_USER + 2)

