#pragma once
extern "C" {
	#include "fzf/fzf.h"
}

#include <strsafe.h>

#include "tsqueue.h"
#include "SearchBand.h"

class CSearchBar : 
	public CWindowImpl<CSearchBar, CReBarCtrl>
{

public:

	DECLARE_WND_SUPERCLASS(_T("WTL_NavigationBar"), CReBarCtrl::GetWndClassName())

	CSearchRootView m_wndSearchBand;

	BEGIN_MSG_MAP(CSearchView)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		REFLECT_NOTIFICATIONS()
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		LRESULT lRes = DefWindowProc();

		::SetWindowTheme(m_hWnd, L"NavbarComposited", NULL);

		m_wndSearchBand.Create(m_hWnd, rcDefault, NULL, 
			WS_CHILD | WS_GROUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_GROUP,
			WS_EX_CONTROLPARENT);

		enum { 
			CX_SEARCH = 200,
			CY_SEARCH = 28,
		};

		REBARBANDINFO rbi = { 0 };
		rbi.cbSize = sizeof(REBARBANDINFO);
		rbi.fMask = RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_STYLE | RBBIM_SIZE | RBBIM_IDEALSIZE;
		rbi.fStyle = RBBS_TOPALIGN | RBBS_NOGRIPPER;
		rbi.hwndChild = m_wndSearchBand;
		rbi.cx = rbi.cxIdeal = rbi.cxMinChild = CX_SEARCH;
		rbi.cyChild = rbi.cyMinChild = CY_SEARCH;
		InsertBand(0, &rbi);

		return lRes;
	}
};

class CSearchView : public CWindowImpl<CSearchView>
{
public:
    DECLARE_WND_CLASS(_T("WTL_SearchView"))

    CListViewCtrl m_list;
	CSearchBar m_reBar;

	CSearchView() {}

	BEGIN_MSG_MAP(CSearchView)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		NOTIFY_CODE_HANDLER(LVN_GETDISPINFO, OnTvnGetdispinfoTree)
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		auto s = DefWindowProc();
		m_list.Create(m_hWnd, rcDefault, NULL, WS_GROUP | WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_SHOWSELALWAYS | LVS_ALIGNTOP | LVS_OWNERDATA | LVS_REPORT);
		//m_list.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT);
		m_reBar.Create(m_hWnd, rcDefault, NULL,
			WS_CHILD | WS_VISIBLE | WS_GROUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
			RBS_VARHEIGHT | RBS_AUTOSIZE | RBS_VERTICALGRIPPER |
			CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_TOP,
			WS_EX_CONTROLPARENT);

		//m_list.InsertColumn(0, L"date", LVCFMT_LEFT, 50);
		//m_list.InsertColumn(1, L"num", LVCFMT_LEFT, 50);
		//m_list.InsertItem(0, L"dddddddddd");
		//m_list.InsertItem(1, L"dffffffefefefef");
		m_list.InsertColumn(0, _T(""), LVCFMT_LEFT, 300);
		m_list.ModifyStyle(LVS_REPORT |LVS_NOCOLUMNHEADER, 
			LVS_REPORT|LVS_NOCOLUMNHEADER);
		m_list.SetExtendedListViewStyle (LVS_EX_DOUBLEBUFFER, 
			LVS_EX_DOUBLEBUFFER);
		DWORD dwStyle = m_list.GetExtendedListViewStyle();
		dwStyle |= LVS_EX_FULLROWSELECT;
		m_list.SetExtendedListViewStyle(dwStyle);
		m_list.SetItemCount(10);
		for(int i = 0; i < 10; ++i)
		{
			LVITEM lvi = { 0 };
			lvi.mask = LVIF_TEXT;
			lvi.iItem = i;
			lvi.iSubItem = 0;
			lvi.pszText = LPSTR_TEXTCALLBACK;
			lvi.cchTextMax = MAX_PATH;
			int n = m_list.InsertItem(&lvi);
		}
		return 0;
	}

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		bHandled = FALSE;
		return 0;
	}

	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
	{
		int x = GET_X_LPARAM(lParam);//width
    	int y = GET_Y_LPARAM(lParam);//heigh
		CRect rcClient(0, 0, x, y);
		CRect rcToolBar(1, 2, rcClient.right - 2, 30);
		CRect rcList(1, rcToolBar.bottom + 2, rcClient.right - 2, rcClient.bottom - 2);
		m_reBar.SetWindowPos(NULL, &rcToolBar, SWP_NOACTIVATE | SWP_NOZORDER);
		m_list.SetWindowPos(NULL, &rcList, SWP_NOACTIVATE | SWP_NOZORDER);
		bHandled = FALSE;
		return 0;
	}

	LRESULT OnTvnGetdispinfoTree(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
	{
		NMLVDISPINFO* pDetails = reinterpret_cast<NMLVDISPINFO*>(pnmh);
		if (pDetails->item.mask & LVIF_TEXT) 
		{
			StringCchPrintf(pDetails->item.pszText,
				pDetails->item.cchTextMax, _T("Item %i"), pDetails->item.iItem + 1);
		}
		return 0;
	}
};

#if 0
class CSearchView : public CWindowImpl<CSearchView, CWindow>
					//public CThreadImpl<CSearchView>
{
	typedef CWindowImpl<CSearchView, CWindow> BaseClass;

public:
    DECLARE_WND_CLASS(NULL)

	struct task
    {
	    CString text;
    };
	tsqueue<task> tasks_;

    CSearchView() :	m_symbols(NULL)
    {
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
    }

#if 0
    BOOL Create(DWORD dwStyle, RECT lpRect, HWND hWndParent, UINT nID)
    {
		m_list.Create(*this, rcDefault, NULL, LVS_SHOWSELALWAYS | LVS_AUTOARRANGE | LVS_ALIGNTOP | LVS_OWNERDATA | LVS_REPORT);
		m_edit.Create(*this, rcDefault, NULL, ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_NOHIDESEL | ES_SAVESEL | ES_SELECTIONBAR);

		RECT rc{0};
		GetClientRect(&rc);
		m_edit.MoveWindow(0, 0, rc.right-rc.left, 20);
		m_list.MoveWindow(0, 20, rc.right-rc.left, rc.bottom-rc.top-20);
		return TRUE;
    }
#endif

    void SetDataSource(CPdbCollector* collector)
    {
        m_edit.Clear();
        m_list.DeleteAllItems();
		m_symbols = collector;
        //while(m_list.DeleteString(0));
    }

#if 0
    DWORD Run()
    {
		BOOL bFinish = FALSE;
		fzf_slab_t *fzf_flab = fzf_make_default_slab();
    	CAtlArray<PDBSYMBOL> tmp_symbols;
        while(!IsAborted())
        {
			if (IsAborted()) Abort();

			int res = tasks_.wait(100);
			if (!res)
			{
				if (IsAborted()) Abort();
				if(!bFinish && m_symbols->m_bDone)
				{
					{
						CComCritSecLock<CComCriticalSection> lock(m_symbols->m_lock);
						tmp_symbols.Copy(m_symbols->m_aSymbols);
					}

					int index = 0;
					m_list.SetRedraw(FALSE);
					for (size_t i = 0; i < tmp_symbols.GetCount(); ++i)
					{
						if (IsAborted()) Abort();
						//index = m_list.AddString(tmp_symbols.GetAt(i).sKey);
						m_list.SetItemCount(tmp_symbols.GetCount());
					}
					m_list.SetRedraw(TRUE);
					bFinish = TRUE;
				}
				continue;
			}

			if(tmp_symbols.IsEmpty())
			{
				tasks_.pop_front();
				continue;
			}

			auto task = tasks_.pop_front();
			if(task.text.IsEmpty())
			{
				m_list.DeleteAllItems();
				int index = 0;
				for (size_t i = 0; i < tmp_symbols.GetCount(); ++i)
				{
					if (IsAborted()) Abort();
					m_list.SetItemCount(tmp_symbols.GetCount());
				}
				continue;
			}

			/*
			for(int i = m_list.GetCount() - 1; i >= 0; i--)
			{
				CString text;
				m_list.GetText(i, text);
				fzf_pattern_t* fzf_pattern = fzf_parse_pattern(CaseSmart, false, const_cast<char*>(CStringA(task.text).GetString()), true);
				int score = fzf_get_score(CStringA(text).GetString(), fzf_pattern, fzf_flab);
				if (score <= 0)
				{
					DeleteString(i);
				}
				fzf_free_pattern(fzf_pattern);
			}
			*/
        }
		fzf_free_slab(fzf_flab);

        return 0;
    }
#endif

    BEGIN_MSG_MAP(CSearchView)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
	    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnBackColor)
        COMMAND_CODE_HANDLER(EN_CHANGE, OnEditChanged)
        MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnListLButtonDblClk)
		NOTIFY_CODE_HANDLER(LVN_GETDISPINFO, OnTvnGetdispinfoTree)
    END_MSG_MAP()

private:
    LRESULT OnCreate(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
    {
		//DefWindowProc();
		m_list.Create(*this, rcDefault, NULL, WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_OWNERDATA | LVS_SHOWSELALWAYS | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
		m_edit.Create(*this, rcDefault, NULL, WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_NOHIDESEL | ES_SAVESEL | ES_SELECTIONBAR);

		m_edit.SetFont(m_font);

		CClientDC dc(*this);
		HFONT hOldFont = dc.SelectFont(m_font);
		TEXTMETRIC tm;
		dc.GetTextMetrics(&tm);
		int nLogPix = dc.GetDeviceCaps(LOGPIXELSX);
		dc.SelectFont(hOldFont);
		int cxTab = ::MulDiv(tm.tmAveCharWidth * 8, 1440, nLogPix);
		if(cxTab != -1)
		{
			PARAFORMAT pf;
			pf.cbSize = sizeof(PARAFORMAT);
			pf.dwMask = PFM_TABSTOPS;
			pf.cTabCount = MAX_TAB_STOPS;
			for(int i = 0; i < MAX_TAB_STOPS; ++i)
			{
				pf.rgxTabs[i] = (i + 1) * cxTab;
				m_edit.SetParaFormat(pf);
			}
			dc.SelectFont(hOldFont);
			m_edit.SetModify(FALSE);
		}

		//RECT rc{0};
		//GetClientRect(&rc);
		//m_edit.MoveWindow(0, 0, rc.right-rc.left, tm.tmHeight + tm.tmExternalLeading + 3);
		//m_list.MoveWindow(0, tm.tmHeight + tm.tmExternalLeading + 3 + 2, rc.right-rc.left, rc.bottom-rc.top-(tm.tmHeight + tm.tmExternalLeading + 3 + 2));
		return 0;
    }

    LRESULT OnEditChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& /*bHandled*/)
    {
		ATLASSERT(m_edit.m_hWnd == hWndCtl);

		CString str;
		int nLength = m_edit.GetWindowText(str);
		tasks_.push_back({str});
        return 0;
    }

    LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
    {
		if (VK_RETURN == wParam)
		{
			::PostMessage(GetParent(), WM_COMMAND, MAKEWPARAM(IDOK, 0), reinterpret_cast<LPARAM>(m_hWnd));
		}
		if (VK_ESCAPE == wParam) ::PostMessage(GetParent(), WM_COMMAND, MAKEWPARAM(IDCANCEL, 0), reinterpret_cast<LPARAM>(m_hWnd));
		bHandled = FALSE;
		return 0;
    }

    LRESULT OnListLButtonDblClk(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
    	::PostMessage(GetParent(), WM_COMMAND, MAKEWPARAM(IDOK, 0), reinterpret_cast<LPARAM>(m_hWnd));
		return 0;
    }

    LRESULT OnSize(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
		CClientDC dc(*this);
		HFONT hOldFont = dc.SelectFont(m_font);
		TEXTMETRIC tm;
		dc.GetTextMetrics(&tm);

		int x = GET_X_LPARAM(lParam);//width
    	int y = GET_Y_LPARAM(lParam);//heigh
		RECT rc{0};
		GetClientRect(&rc);
		if(m_edit)
		{
			m_edit.MoveWindow(0, 0, x, tm.tmHeight + tm.tmExternalLeading + 3);
		}
		if(m_list)
		{
			m_list.MoveWindow(0, tm.tmHeight + tm.tmExternalLeading + 3 + 2, x, rc.bottom-rc.top-(tm.tmHeight + tm.tmExternalLeading + 3 + 2));
		}
		return 0;
    }

    LRESULT OnBackColor(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
    {
	   return reinterpret_cast<LRESULT>(::GetSysColorBrush(COLOR_WINDOW)); 
    }

   LRESULT OnEraseBkgnd(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
#if 1
	   CDCHandle dc((HDC)wParam);
	   RECT rc;
	   GetClientRect(&rc);
	   CRgn rgn1, rgn2, rgn;
	   rgn1.CreateRectRgnIndirect(&rc);
	   AdjustRect(FALSE, &rc);
	   rgn2.CreateRectRgnIndirect(&rc);
	   rgn.CreateRectRgnIndirect(&rc);
	   rgn.CombineRgn(rgn1, rgn2, RGN_DIFF);
	   dc.FillRgn(rgn, ::GetSysColorBrush(COLOR_BTNFACE));
       return TRUE;
#else
       return TRUE;
#endif
   }

	LRESULT OnTvnGetdispinfoTree(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
    {
		NMLVDISPINFO *pTVDispInfo = reinterpret_cast<NMLVDISPINFO*>(pnmh);

		// Get the tree control item
		LVITEM* pItem = &(pTVDispInfo)->item;

		// Just interested in text notifications
		if ((pItem->mask & TVIF_TEXT) != TVIF_TEXT)
		{
			bHandled = FALSE;
			return 0;
		}

		CComCritSecLock<CComCriticalSection> lock(m_symbols->m_lock);
		const PDBSYMBOL& Symbol = m_symbols->m_aSymbols[pItem->iItem];
		pItem->pszText = const_cast<LPTSTR>((LPCTSTR)Symbol.sKey);
		bHandled = TRUE;
		return 0;
    }


   	void AdjustRect(BOOL bLarger, LPRECT lpRect)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		::SendMessage(m_hWnd, TCM_ADJUSTRECT, bLarger, (LPARAM)lpRect);
	}

    CPdbCollector* m_symbols;
    CListViewCtrl m_list;
    CRichEditCtrl m_edit;
	CFont m_font;
};
#endif
