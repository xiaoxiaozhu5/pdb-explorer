
#pragma once


class CMainFrame : 
   public CFrameWindowImpl<CMainFrame>, 
   public CUpdateUI<CMainFrame>,
   public CMessageFilter, 
   public CIdleHandler
{
public:
   DECLARE_FRAME_WND_CLASS(NULL, IDR_MAINFRAME)

   CBrowserView m_view;   

   virtual BOOL PreTranslateMessage(MSG* pMsg)
   {
      if( CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg) ) return TRUE;
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
      COMMAND_ID_HANDLER(ID_FILE_OPEN, OnFileOpen)
      COMMAND_ID_HANDLER(ID_APP_EXIT, OnFileExit)
      COMMAND_ID_HANDLER(ID_VIEW_REFRESH, OnViewRefresh)
      COMMAND_ID_HANDLER(ID_VIEW_TOOLBAR, OnViewToolBar)
      COMMAND_ID_HANDLER(ID_VIEW_STATUS_BAR, OnViewStatusBar)
      COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
      CHAIN_MSG_MAP( CUpdateUI<CMainFrame> )
      CHAIN_MSG_MAP( CFrameWindowImpl<CMainFrame> )
   END_MSG_MAP()

   LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      CreateSimpleToolBar();

      CreateSimpleStatusBar();

      m_hWndClient = m_view.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE, WS_EX_CLIENTEDGE);

      UIAddToolBar(m_hWndToolBar);
      UISetCheck(ID_VIEW_TOOLBAR, 1);
      UISetCheck(ID_VIEW_STATUS_BAR, 1);

      // Register object for message filtering and idle updates
      CMessageLoop* pLoop = _Module.GetMessageLoop();
      ATLASSERT(pLoop != NULL);
      pLoop->AddMessageFilter(this);
      pLoop->AddIdleHandler(this);

      PostMessage(WM_COMMAND, ID_FILE_OPEN);

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
      COMDLG_FILTERSPEC spec[] = { {L"PDB Files", L"*.pdb"} };
      CShellFileOpenDialog dlg(NULL, FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST, _T("pdb"), spec, _countof(spec));
      dlg.DoModal();
      CString sFilename;
      dlg.GetFilePath(sFilename);
      if( sFilename.IsEmpty() ) return 0;
      m_view.LoadFromFile(sFilename);
      return 0;
   }

   LRESULT OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      PostMessage(WM_CLOSE);
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

   // Overrides

   static HWND CreateSimpleToolBarCtrl(HWND hWndParent, UINT /*nResourceID*/, BOOL /*bInitialSeparator*/ = FALSE, DWORD /*dwStyle*/ = ATL_SIMPLE_TOOLBAR_STYLE, UINT nID = ATL_IDW_TOOLBAR)
   {
      CWindow wnd = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_STATIC, _T("  PDB Explorer Tool"), WS_CHILD | WS_VISIBLE | SS_NOPREFIX | SS_CENTERIMAGE, 0, 0, 3000, 24, hWndParent, (HMENU) nID, _pModule->GetResourceInstance(), 0);
      CLogFont lf = AtlGetDefaultGuiFont();
      _tcscpy_s(lf.lfFaceName, _countof(lf.lfFaceName), _T("Georgia"));
      lf.MakeLarger(4);
      static CFont s_fontBanner;
      s_fontBanner.CreateFontIndirect(&lf);
      wnd.SetFont(s_fontBanner);
      return wnd;
   }
};

