#pragma once

#include <atldwm.h>

class CSearchRootView : 
	public CWindowImpl<CSearchRootView>,
	public CThemeImpl<CSearchRootView>,
	public CCustomDraw<CSearchRootView>,
	public CDwm
{
public:

	DECLARE_WND_CLASS(_T("WTL_SearchBand"))

	// SBBACKGROUND stuff that Microsoft won't document properly
	enum {
		SBP_SBBACKGROUND = 1,
		SBB_NORMAL = 0x1, 
		SBB_HOT = 0x2, 
		SBB_DISABLED = 0x3, 
		SBB_FOCUSED = 0x4
	};

	CRect m_rcButton;
	CContainedWindowT<CEdit> m_ctrlEdit;
	HWND m_hWndContained;
	BOOL m_bTracked;
	CFont m_font;

	CSearchRootView() : m_ctrlEdit(this, 1), m_bTracked(FALSE)
	{
	}

	BEGIN_MSG_MAP(CSearchRootView)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_PRINTCLIENT, OnPaint)
		COMMAND_CODE_HANDLER(EN_CHANGE, OnEditChange)
		COMMAND_CODE_HANDLER(EN_SETFOCUS, OnEditChange)
		COMMAND_CODE_HANDLER(EN_KILLFOCUS, OnEditChange)
		CHAIN_MSG_MAP( CThemeImpl<CSearchRootView> )
		CHAIN_MSG_MAP( CCustomDraw<CSearchRootView> )
		ALT_MSG_MAP( 1 )
		m_hWndContained = hWnd; // надо для ховера
		MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
		MESSAGE_HANDLER(WM_MOUSELEAVE, OnMouseLeave)
		REFLECT_NOTIFICATIONS()
	END_MSG_MAP()

	DWORD OnPreErase(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/)
	{
		//Invalidate();
		return CDRF_NOTIFYITEMDRAW;
	}

	DWORD OnPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/)
	{
		//Invalidate();
		return CDRF_NOTIFYITEMDRAW;
	}

	DWORD OnItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/)
	{
		Invalidate();
		return CDRF_DODEFAULT;
	}


	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{     
		SetThemeClassList(L"SearchBoxComposited::SearchBox");
		SetThemeExtendedStyle(THEME_EX_THEMECLIENTEDGE, THEME_EX_THEMECLIENTEDGE);

		m_ctrlEdit.Create(m_hWnd, rcDefault, NULL, 
			WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
			ES_LEFT | ES_AUTOHSCROLL, 0);

		CLogFont lf;
		lf.lfWeight = FW_NORMAL;
		if(RunTimeHelper::IsVista())
		{
			lf.SetHeight(9);
			SecureHelper::strcpy_x(lf.lfFaceName, LF_FACESIZE, _T("Consolas"));
		}
		else
		{
			lf.SetHeight(8);
			SecureHelper::strcpy_x(lf.lfFaceName, LF_FACESIZE, _T("Tahoma"));
		}
		m_font.CreateFontIndirect(&lf);

		m_ctrlEdit.SetFont(m_font);

		DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_TABSTOP 
			| TBSTYLE_TRANSPARENT | TBSTYLE_CUSTOMERASE 
			| TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS 
			| CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE | CCS_TOP
			;

		::SetWindowTheme(m_ctrlEdit, L"SearchBoxEditComposited", NULL);

		bHandled = FALSE;
		return 0;
	}

	LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		CRect rcClient(0, 0, LOWORD(lParam), HIWORD(lParam));
		CRect rcEdit(1, 4, rcClient.Width() - m_rcButton.Width() - 2, rcClient.bottom + 1);
		CRect rcToolBar(rcClient.Width() - m_rcButton.Width() - 2, 1, rcClient.right - 2, rcClient.bottom + 2);
		rcClient.bottom += 6;
		//CRect rcEdit(0, 0, rcClient.Width() - m_rcButton.Width(), rcClient.bottom);
		//CRect rcToolBar(rcClient.Width() - m_rcButton.Width(), 0, rcClient.right, rcClient.bottom);
		m_ctrlEdit.SetWindowPos(NULL, &rcEdit, SWP_NOACTIVATE | SWP_NOZORDER);
		return 0;
	}

	LRESULT OnEraseBkgnd(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		CDCHandle dc = (HDC) wParam;
		if( !DwmIsCompositionEnabled() ) 
			DoPaint(dc);
		
		return 1;
	}

	LRESULT OnPaint(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		if( wParam != NULL )
		{
			DoPaint((HDC) wParam);
		}
		else
		{
			CPaintDC dc(m_hWnd);
			if( DwmIsCompositionEnabled() ) 
				DoPaint(dc.m_hDC);
		}
		return 0;
	}

	LRESULT OnClearEdit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		m_ctrlEdit.SetWindowText(_T(""));
		return 0;
	}

	LRESULT OnEditChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
	{
		_Invalidate();
		// TODO: Notify someone about SEARCH filter change!
		bHandled = FALSE;
		return 0;
	}

	// Edit & SearchButton handlers

	LRESULT OnMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		if( !m_bTracked ) {
			m_bTracked = _StartTrackMouseLeave(m_hWndContained);
			_Invalidate();
		}
		bHandled = FALSE;
		return 0;
	}

	LRESULT OnMouseLeave(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		m_bTracked = FALSE;
		_Invalidate();
		bHandled = FALSE;
		return 0;
	}

	// Implementation

	void DoPaint(CDCHandle dc)
	{
		//ATLASSERT(IsThemeActive());
		CRect rcClient;
		GetClientRect(&rcClient);
		int nStateId = SBB_NORMAL;
		if( m_bTracked ) nStateId = SBB_FOCUSED;
		if( ::GetFocus() == m_ctrlEdit ) nStateId = SBB_FOCUSED;
		if( !IsWindowEnabled() ) nStateId = SBB_DISABLED;
		DrawThemeParentBackground(dc, &rcClient);
		DrawThemeBackground(dc, SBP_SBBACKGROUND, nStateId, &rcClient, NULL);
	}

	void _Invalidate()
	{
		m_ctrlEdit.Invalidate(TRUE);
	}

	BOOL _StartTrackMouseLeave(HWND hWnd)
	{
		TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hWnd };
		return _TrackMouseEvent(&tme);
	}
};

