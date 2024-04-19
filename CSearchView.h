#pragma once
#include <unordered_map>

extern "C" {
	#include "fzf/fzf.h"
}

#include <strsafe.h>

#include "tsqueue.h"

class CSearchView : public CWindowImpl<CSearchView>,
					public CThreadImpl<CSearchView>
{
public:
    DECLARE_WND_CLASS(_T("WTL_SearchView"))

	CFont m_font;
    CContainedWindowT<CListViewCtrl> m_list;
	CContainedWindowT<CEdit> m_search;
    CPdbCollector* m_symbols;
	std::vector<int> m_filtered_index;
	std::unordered_map<std::string, std::vector<int>> m_search_filter;

	struct task
    {
		std::string text;
    };
	tsqueue<task> tasks_;

	CSearchView() : m_search(this, 1), m_list(this, 1)
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

	BEGIN_MSG_MAP(CSearchView)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		COMMAND_CODE_HANDLER(EN_CHANGE, OnEditChange)
		NOTIFY_CODE_HANDLER(LVN_GETDISPINFO, OnTvnGetdispinfoTree)
		NOTIFY_CODE_HANDLER(NM_DBLCLK, OnSelChanged)
		ALT_MSG_MAP(1)
        MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
        //MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnListLButtonDblClk)
		//COMMAND_CODE_HANDLER(EN_SETFOCUS, OnEditChange)
		//COMMAND_CODE_HANDLER(EN_KILLFOCUS, OnEditChange)
		REFLECT_NOTIFICATIONS()
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		auto s = DefWindowProc();
		m_list.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_SHOWSELALWAYS | LVS_ALIGNTOP | LVS_OWNERDATA | LVS_REPORT | LVS_NOCOLUMNHEADER | LVS_AUTOARRANGE);
		//m_list.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT);
		m_search.Create(m_hWnd, rcDefault, NULL, 
			WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
			ES_LEFT | ES_AUTOHSCROLL | WS_BORDER, 0);
		m_search.SetFont(m_font);

		//m_list.InsertColumn(0, L"date", LVCFMT_LEFT, 50);
		//m_list.InsertColumn(1, L"num", LVCFMT_LEFT, 50);
		//m_list.InsertItem(0, L"dddddddddd");
		//m_list.InsertItem(1, L"dffffffefefefef");
		m_list.InsertColumn(0, _T("Item Name"), LVCFMT_LEFT, 500);
		DWORD dwStyle = m_list.GetExtendedListViewStyle();
		dwStyle |= LVS_EX_DOUBLEBUFFER;
		dwStyle |= LVS_EX_FULLROWSELECT;
		m_list.SetExtendedListViewStyle(dwStyle);
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
		CRect rcToolBar(1, 2, rcClient.right - 2, 24);
		CRect rcList(1, rcToolBar.bottom + 2, rcClient.right - 2, rcClient.bottom - 2);
		m_search.SetWindowPos(NULL, &rcToolBar, SWP_NOACTIVATE | SWP_NOZORDER);
		m_list.SetWindowPos(NULL, &rcList, SWP_NOACTIVATE | SWP_NOZORDER);
		m_list.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
		//m_list.ShowScrollBar(SB_HORZ, FALSE);
		bHandled = FALSE;
		return 0;
	}

	LRESULT OnEditChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
	{
		m_search.Invalidate(TRUE);
		CString str;
		int nLength = m_search.GetWindowText(str);
		CT2CA pszConvertedAnsiString (str);
		tasks_.push_back({pszConvertedAnsiString});

		bHandled = FALSE;
		return 0;
	}

	LRESULT OnTvnGetdispinfoTree(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
	{
		NMLVDISPINFO* pDetails = reinterpret_cast<NMLVDISPINFO*>(pnmh);
		if (pDetails->item.mask & LVIF_TEXT) 
		{
			CComCritSecLock<CComCriticalSection> lock(m_symbols->m_lock);
			if(m_filtered_index.empty())
			{
				const PDBSYMBOL& Symbol = m_symbols->m_aSymbols[pDetails->item.iItem];
				pDetails->item.pszText = const_cast<LPTSTR>((LPCTSTR)Symbol.sKey);
				pDetails->item.iSubItem = Symbol.dwSymTag;
				//StringCchPrintf(pDetails->item.pszText,
				//	pDetails->item.cchTextMax, _T("%d %s"), pDetails->item.iItem, (LPCTSTR)Symbol.sKey);
			}
			else
			{
				auto index = m_filtered_index[pDetails->item.iItem];
				const PDBSYMBOL& Symbol = m_symbols->m_aSymbols[index];
				pDetails->item.pszText = const_cast<LPTSTR>((LPCTSTR)Symbol.sKey);
				pDetails->item.iSubItem = Symbol.dwSymTag;
				//StringCchPrintf(pDetails->item.pszText,
				//	pDetails->item.cchTextMax, _T("%d %s"), index, (LPCTSTR)Symbol.sKey);
			}
		}
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

	LRESULT OnSelChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
	{
		CString text;
		NM_LISTVIEW* pDetails = reinterpret_cast<NM_LISTVIEW*>(pnmh);
		m_list.GetItemText(pDetails->iItem, 0, text);
		::SendMessage(GetParent(), WM_SHOW_ITEM, pDetails->iSubItem, (LPARAM)(LPCTSTR)text);
		return 0;
	}

    void SetDataSource(CPdbCollector* collector)
    {
        m_search.Clear();
		m_list.DeleteAllItems();
		m_search_filter.clear();
		m_filtered_index.clear();
		m_symbols = collector;
        //while(m_list.DeleteString(0));
    }

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

					if (IsAborted()) Abort();
				    m_search_filter.clear();
					m_filtered_index.clear();
					m_list.SetItemCountEx(tmp_symbols.GetCount(), LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);
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
			if(task.text.empty())
			{
				m_search_filter.clear();
				m_filtered_index.clear();
				m_list.DeleteAllItems();
				if (IsAborted()) Abort();
				m_list.SetItemCountEx(tmp_symbols.GetCount(), LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);
				continue;
			}
																				
			m_list.DeleteAllItems();
			std::vector<int> filtered_index;
			if(m_search_filter.empty())
			{
				for (size_t i = 0; i < tmp_symbols.GetCount(); ++i)
				{
					fzf_pattern_t* fzf_pattern = fzf_parse_pattern(CaseSmart, false, const_cast<char*>(task.text.c_str()), true);
					int score = fzf_get_score(CStringA(tmp_symbols[i].sKey).GetString(), fzf_pattern, fzf_flab);
					if (score > 0)
					{
						filtered_index.push_back(i);
					}
					fzf_free_pattern(fzf_pattern);
				}
				m_search_filter.insert(std::make_pair(task.text, filtered_index));
			}
			else
			{
				auto& find_item = m_search_filter.find(task.text);
				if(find_item == m_search_filter.end())
				{
					for (size_t i = 0; i < m_filtered_index.size(); ++i)
					{
						fzf_pattern_t* fzf_pattern = fzf_parse_pattern(CaseSmart, false, const_cast<char*>(task.text.c_str()), true);
						int score = fzf_get_score(CStringA(tmp_symbols[m_filtered_index[i]].sKey).GetString(), fzf_pattern, fzf_flab);
						if (score > 0)
						{
							filtered_index.push_back(m_filtered_index[i]);
						}
						fzf_free_pattern(fzf_pattern);
					}
					m_search_filter.insert(std::make_pair(task.text, filtered_index));
				}
				else
				{
					auto& findex = find_item->second;
					for (size_t i = 0; i < findex.size(); ++i)
					{
						filtered_index.push_back(i);
					}
				}
			}
			m_list.SetItemCountEx(filtered_index.size(), LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);
			m_filtered_index.swap(filtered_index);
        }
		fzf_free_slab(fzf_flab);

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
	    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnBackColor)
        COMMAND_CODE_HANDLER(EN_CHANGE, OnEditChanged)
        MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
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
