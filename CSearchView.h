#pragma once
extern "C" {
	#include "fzf/fzf.h"
}

#include "tsqueue.h"

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

    CSearchView() : m_list(this, 1),
					m_symbols(NULL)
    {
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
		ALT_MSG_MAP(1)
        MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnListLButtonDblClk)
		NOTIFY_CODE_HANDLER(LVN_GETDISPINFO, OnTvnGetdispinfoTree)
    END_MSG_MAP()

private:
    LRESULT OnCreate(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
    {
		//DefWindowProc();
		m_list.Create(*this, rcDefault, NULL, WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_OWNERDATA | LVS_SHOWSELALWAYS | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
		m_edit.Create(*this, rcDefault, NULL, WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_NOHIDESEL | ES_SAVESEL | ES_SELECTIONBAR);

		RECT rc{0};
		GetClientRect(&rc);
		m_edit.MoveWindow(0, 0, rc.right-rc.left, 20);
		m_list.MoveWindow(0, 23, rc.right-rc.left, rc.bottom-rc.top-23);
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
		int x = GET_X_LPARAM(lParam);
    	int y = GET_Y_LPARAM(lParam);
		RECT rc{0};
		GetClientRect(&rc);
		if(m_edit)
		{
			m_edit.MoveWindow(0, 0, rc.right-rc.left, 23);
		}
		if(m_list)
		{
			m_list.MoveWindow(0, 23, rc.right-rc.left, rc.bottom-rc.top-23);
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
    CContainedWindowT<CListViewCtrl> m_list;
    CRichEditCtrl m_edit;
};
