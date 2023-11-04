#pragma once
#include <stack>

#define FILE_MENU_POSITION	0
#define RECENT_MENU_POSITION	2

LPCTSTR lpcstrMTPadRegKey = _T("Software\\Microsoft\\PdbExplorer\\PdbExplorer");

class CMainFrame :
	public CRibbonFrameWindowImpl<CMainFrame>
{
public:
	DECLARE_FRAME_WND_CLASS(NULL, IDR_MAINFRAME)

	CBrowserView m_view;
	CMultiPaneStatusBarCtrl m_StatusBar;
	CCommandBarCtrl m_CmdBar;
	CRibbonRecentItemsCtrl<ID_RIBBON_RECENT_FILES> m_mru;

	std::stack<CString> m_QueBack;
	std::stack<CString> m_QueNext;

	BOOL PreTranslateMessage(MSG* pMsg)
	{
		if(m_view.PreTranslateMessage(pMsg))
			return TRUE;

		return CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg);
	}

	BEGIN_RIBBON_CONTROL_MAP(CMainFrame)
		RIBBON_CONTROL(m_mru)
	END_RIBBON_CONTROL_MAP()

	BEGIN_MSG_MAP(CMainFrame)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
	    MESSAGE_HANDLER(WM_CLOSE, OnClose)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_SET_STATUS_TEXT, OnSetStatusBarText)
		MESSAGE_HANDLER(WM_ADD_HISTORY, OnAddHistory)
		COMMAND_ID_HANDLER(ID_FILE_OPEN, OnFileOpen)
		COMMAND_ID_HANDLER(ID_APP_EXIT, OnFileExit)
		COMMAND_ID_HANDLER(ID_EDIT_COPY, OnCopy)
		COMMAND_ID_HANDLER(ID_FILE_SAVE, OnSave)
		COMMAND_ID_HANDLER(ID_NAV_BACK, OnBack)
		COMMAND_ID_HANDLER(ID_NVA_FOR, OnNext)
	    COMMAND_RANGE_HANDLER(ID_FILE_MRU_FIRST, ID_FILE_MRU_LAST, OnFileRecent)
		COMMAND_ID_HANDLER(ID_VIEW_REFRESH, OnViewRefresh)
		COMMAND_ID_HANDLER(ID_VIEW_TOOLBAR, OnViewToolBar)
		COMMAND_ID_HANDLER(ID_VIEW_STATUS_BAR, OnViewStatusBar)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		CHAIN_MSG_MAP(CRibbonFrameWindowImpl<CMainFrame>)
	    CHAIN_COMMANDS_MEMBER(m_view)
		END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		// Create command bar window
		HWND hWndCmdBar = m_CmdBar.Create(m_hWnd, rcDefault, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE);
		// Attach menu
		m_CmdBar.AttachMenu(GetMenu());
		// Load command bar images
		m_CmdBar.LoadImages(IDR_MAINFRAME);
		// Remove old menu
		SetMenu(NULL);

		bool bRibbonUI = RunTimeHelper::IsRibbonUIAvailable();
		if (bRibbonUI) 
		{
			// UI Setup and adjustments
			UIAddMenu(m_CmdBar.GetMenu(), true);
			UIRemoveUpdateElement(ID_FILE_MRU_FIRST);

			// Ribbon UI state and settings restoration
			CRibbonPersist(lpcstrMTPadRegKey).Restore(bRibbonUI, m_hgRibbonSettings);
		}

		HWND hWndToolBar = CreateSimpleToolBarCtrl(m_hWnd, IDR_MAINFRAME, FALSE, ATL_SIMPLE_TOOLBAR_PANE_STYLE);
		ATLASSERT(::IsWindow(hWndToolBar));
		CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
		AddSimpleReBarBand(hWndCmdBar);
		AddSimpleReBarBand(hWndToolBar, NULL, TRUE);

		m_hWndStatusBar = m_StatusBar.Create(m_hWnd);
		int aPanes[] = {ID_DEFAULT_PANE, IDS_SB_PANE1, IDS_SB_PANE2};
		m_StatusBar.SetPanes(aPanes, 3);

		m_hWndClient = m_view.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE, WS_EX_CLIENTEDGE);

		UIAddToolBar(hWndToolBar);
		UISetCheck(ID_VIEW_TOOLBAR, 1);
		UISetCheck(ID_VIEW_STATUS_BAR, 1);

		HMENU hMenu = m_CmdBar.GetMenu();
		HMENU hFileMenu = ::GetSubMenu(hMenu, FILE_MENU_POSITION);
#ifdef _DEBUG
		// absolute position, can change if menu changes
		TCHAR szMenuString[100];
		::GetMenuString(hFileMenu, RECENT_MENU_POSITION, szMenuString, sizeof(szMenuString), MF_BYPOSITION);
		ATLASSERT(lstrcmp(szMenuString, _T("Recent &Files")) == 0);
#endif //_DEBUG
		HMENU hMruMenu = ::GetSubMenu(hFileMenu, RECENT_MENU_POSITION);
		m_mru.SetMenuHandle(hMruMenu);
		m_mru.ReadFromRegistry(lpcstrMTPadRegKey);
		ShowRibbonUI(bRibbonUI); 

		return 0;
	}

	LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		if (RunTimeHelper::IsRibbonUIAvailable())
		{
			bool bRibbonUI = IsRibbonUI();
			if (bRibbonUI)
				SaveRibbonSettings();
			CRibbonPersist(lpcstrMTPadRegKey).Save(bRibbonUI, m_hgRibbonSettings);
		}
		bHandled = FALSE;
		return 0;		
	}

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		bHandled = FALSE;
		return 0;
	}

	LRESULT OnFileOpen(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		COMDLG_FILTERSPEC spec[] = {{L"PDB Files", L"*.pdb"}};
		CShellFileOpenDialog dlg(NULL, FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST, _T("pdb"), spec,
		                         _countof(spec));
		INT_PTR nRet = dlg.DoModal();
		if(nRet == IDOK)
		{
			CString sFilename;
			dlg.GetFilePath(sFilename);
			if (sFilename.IsEmpty()) return 0;
			m_view.LoadFromFile(sFilename);
			m_mru.AddToList(sFilename);
			m_mru.WriteToRegistry(lpcstrMTPadRegKey);
		}
		return 0;
	}

	LRESULT OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		PostMessage(WM_CLOSE);
		return 0;
	}

	LRESULT OnCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		m_view.m_ctrlView.Copy();
		return 0;
	}

	LRESULT OnSave(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		auto sel = m_view.m_ctrlTree.GetSelectedItem();
		CString sel_string;
		m_view.m_ctrlTree.GetItemText(sel, sel_string);
		if (sel_string.IsEmpty()) return 0;

		TCHAR tmp[4096] = {0};
		GETTEXTEX gtx = {0};
		gtx.cb = 4096;
		gtx.flags = GT_USECRLF;
		gtx.codepage = 1200;
		CString content;
		m_view.m_ctrlView.GetTextEx(&gtx, tmp);
		content = tmp;
		if (content.IsEmpty()) return 0;

		CString save_name;
		save_name.Format(TEXT("%s.txt"), sel_string);
		COMDLG_FILTERSPEC spec[] = {{L"TXT Files", L"*.txt"}};
		CShellFileSaveDialog dlg(save_name, FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST, _T("txt"),
		                         spec, _countof(spec));
		dlg.DoModal();
		CString sFilename;
		dlg.GetFilePath(sFilename);
		if (sFilename.IsEmpty()) return 0;

		LPSTR pstrData = (LPSTR)malloc(content.GetLength() + 2);
		if (pstrData)
		{
			ZeroMemory(pstrData, content.GetLength() + 2);
			AtlW2AHelper(pstrData, content, content.GetLength() + 2);
			pstrData[content.GetLength()] = '\0';
			CFile f;
			if (f.Create(sFilename))
			{
				f.Write(pstrData, content.GetLength());
				f.Close();
			}
			free(pstrData);
		}
		return 0;
	}

	LRESULT OnBack(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		if (!m_QueBack.empty())
		{
			CString id = m_QueBack.top();
			if(id == m_view.m_currentIndex)
			{
				m_QueBack.pop();
				m_QueNext.push(id);
				id = m_QueBack.top();
			}
			m_QueBack.pop();

			m_view.ShowSymbol(id);

			m_QueNext.push(id);
		}
		return 0;
	}

	LRESULT OnNext(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		if (!m_QueNext.empty())
		{
			CString id = m_QueNext.top();
			if(id == m_view.m_currentIndex)
			{
				m_QueNext.pop();
				m_QueBack.push(id);
				id = m_QueNext.top();
			}
			m_QueNext.pop();

			m_view.ShowSymbol(id);

			m_QueBack.push(id);
		}
		return 0;
	}

	LRESULT OnFileRecent(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		// get file name from the MRU list
		TCHAR szFile[MAX_PATH];
		if (m_mru.GetFromList(wID, szFile, MAX_PATH))
		{
			// find file name without the path
			LPTSTR lpstrFileName = szFile;
			for (int i = lstrlen(szFile) - 1; i >= 0; i--)
			{
				if (szFile[i] == '\\')
				{
					lpstrFileName = &szFile[i + 1];
					break;
				}
			}
			// open file
			m_view.LoadFromFile(szFile);
			m_mru.MoveToTop(wID);
			m_mru.WriteToRegistry(lpcstrMTPadRegKey);
		}
		else
		{
			::MessageBeep(MB_ICONERROR);
		}

		return 0;
	}

	LRESULT OnViewRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		return 0;
	}

	LRESULT OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		BOOL bVisible = !::IsWindowVisible(m_hWndToolBar);
		::ShowWindow(m_hWndToolBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
		UISetCheck(ID_VIEW_TOOLBAR, bVisible);
		UpdateLayout();
		return 0;
	}

	LRESULT OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		BOOL bVisible = !::IsWindowVisible(m_hWndStatusBar);
		::ShowWindow(m_hWndStatusBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
		UISetCheck(ID_VIEW_STATUS_BAR, bVisible);
		UpdateLayout();
		return 0;
	}

	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		CAboutDlg dlg;
		dlg.DoModal();
		return 0;
	}

	LRESULT OnSetStatusBarText(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		m_StatusBar.SetPaneText(wParam, (LPCTSTR)lParam);
		return 0;
	}

	LRESULT OnAddHistory(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		m_QueBack.push((BSTR)wParam);
		SysFreeString((BSTR)wParam);
		return 0;
	}

	void UpdateUIAll()
	{
		UIUpdateToolBar();
	}
};
