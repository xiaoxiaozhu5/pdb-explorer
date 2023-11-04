// ShellTest.cpp : main source file for ShellTest.exe
//

#include "stdafx.h"

#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>
#include <atlribbon.h>

#include "resource.h"

#include "View.h"
#include "AboutDlg.h"
#include "MainFrm.h"

#include "PdbExplorer.h"

CAppModule _Module;


int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
	_Module.Init(NULL, hInstance);
	HINSTANCE hInstRich = ::LoadLibrary(CRichEditCtrl::GetLibraryName());

	CThreadManager mgr;
	int nRet = mgr.Run(lpstrCmdLine, nCmdShow);

	::FreeLibrary(hInstRich);
	_Module.Term();
}

