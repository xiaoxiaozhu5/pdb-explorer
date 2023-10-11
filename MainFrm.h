#pragma once
#include <stack>


class CMainFrame :
	public CFrameWindowImpl<CMainFrame>,
	public CUpdateUI<CMainFrame>,
	public CMessageFilter,
	public CIdleHandler
{
public:
	DECLARE_FRAME_WND_CLASS(NULL, IDR_MAINFRAME)

	CBrowserView m_view;
	CMultiPaneStatusBarCtrl m_StatusBar;
	CCommandBarCtrl m_CmdBar;

	std::stack<DWORD> m_QueBack;
	std::stack<DWORD> m_QueNext;

	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		if (CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg)) return TRUE;
		return m_view.PreTranslateMessage(pMsg);
	}

	virtual BOOL OnIdle()
	{
		UIUpdateToolBar();
		return FALSE;
	}

	BEGIN_UPDATE_UI_MAP(CMainFrame)
		UPDATE_ELEMENT(ID_VIEW_TOOLBAR, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_VIEW_STATUS_BAR, UPDUI_MENUPOPUP)
		END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CMainFrame)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_SET_STATUS_TEXT, OnSetStatusBarText)
		MESSAGE_HANDLER(WM_ADD_HISTORY, OnAddHistory)
		COMMAND_ID_HANDLER(ID_FILE_OPEN, OnFileOpen)
		COMMAND_ID_HANDLER(ID_APP_EXIT, OnFileExit)
		COMMAND_ID_HANDLER(ID_EDIT_COPY, OnCopy)
		COMMAND_ID_HANDLER(ID_FILE_SAVE, OnSave)
		COMMAND_ID_HANDLER(ID_NAV_BACK, OnBack)
		COMMAND_ID_HANDLER(ID_NVA_FOR, OnNext)
		COMMAND_ID_HANDLER(ID_VIEW_REFRESH, OnViewRefresh)
		COMMAND_ID_HANDLER(ID_VIEW_TOOLBAR, OnViewToolBar)
		COMMAND_ID_HANDLER(ID_VIEW_STATUS_BAR, OnViewStatusBar)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		CHAIN_MSG_MAP(CUpdateUI<CMainFrame>)
		CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame>)
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

		HWND hWndToolBar = CreateSimpleToolBarCtrl(m_hWnd, IDR_MAINFRAME, FALSE, ATL_SIMPLE_TOOLBAR_PANE_STYLE);
		ATLASSERT(::IsWindow(hWndToolBar));
		CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
		AddSimpleReBarBand(hWndCmdBar);
		AddSimpleReBarBand(hWndToolBar, NULL, TRUE);

		m_hWndStatusBar = m_StatusBar.Create(m_hWnd);
		int aPanes[] = {ID_DEFAULT_PANE, IDS_SB_PANE1, IDS_SB_PANE2};
		m_StatusBar.SetPanes(aPanes, 3);

		m_hWndClient = m_view.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE, WS_EX_CLIENTEDGE);

		UIAddToolBar(m_hWndToolBar);
		UISetCheck(ID_VIEW_TOOLBAR, 1);
		UISetCheck(ID_VIEW_STATUS_BAR, 1);

		// Register object for message filtering and idle updates
		CMessageLoop* pLoop = _Module.GetMessageLoop();
		ATLASSERT(pLoop != NULL);
		pLoop->AddMessageFilter(this);
		pLoop->AddIdleHandler(this);

		return 0;
	}

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		// Unregister object for message filtering and idle updates
		CMessageLoop* pLoop = _Module.GetMessageLoop();
		ATLASSERT(pLoop != NULL);
		pLoop->RemoveMessageFilter(this);
		pLoop->RemoveIdleHandler(this);

		bHandled = FALSE;
		return 0;
	}

	LRESULT OnFileOpen(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		COMDLG_FILTERSPEC spec[] = {{L"PDB Files", L"*.pdb"}};
		CShellFileOpenDialog dlg(NULL, FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST, _T("pdb"), spec,
		                         _countof(spec));
		dlg.DoModal();
		CString sFilename;
		dlg.GetFilePath(sFilename);
		if (sFilename.IsEmpty()) return 0;
		m_view.LoadFromFile(sFilename);
		return 0;
	}

	LRESULT OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		PostMessage(WM_CLOSE);
		return 0;
	}

	LRESULT OnCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		m_view.m_ctrlView.SetSel(0, -1);
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
			DWORD id = m_QueBack.top();
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
			DWORD id = m_QueNext.top();
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
		m_QueBack.push(wParam);
		return 0;
	}
};
