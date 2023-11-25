#pragma once
#include "levenshtein.h"

#include "tsqueue.h"

class CSearchView : public CWindowImpl<CSearchView, CComboBox>,
					public CThreadImpl<CSearchView>
{
	typedef CWindowImpl<CSearchView, CComboBox> BaseComboBox;

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

    BOOL Create(DWORD dwStyle, RECT lpRect, HWND hWndParent, UINT nID)
    {
        if(NULL == BaseComboBox::Create(hWndParent, lpRect, NULL, dwStyle, 0, nID))
			return FALSE;
		m_list.SubclassWindow(FindWindowEx(m_hWnd, NULL, _T("ComboLBox"), NULL));
		m_edit = FindWindowEx(m_hWnd, NULL, WC_EDIT, NULL);

		return TRUE;
    }

    int FindString(PCTSTR lpszString)
    {
	    return m_list.FindString(0, lpszString);
    }

    void SetDataSource(CPdbCollector* collector)
    {
        m_edit.Clear();
        m_list.ResetContent();
		m_symbols = collector;
        //while(m_list.DeleteString(0));
    }

    DWORD Run()
    {
        while(!IsAborted())
        {
			if (IsAborted()) Abort();

			int res = tasks_.wait(100);
			if (!res)
			{
				if (IsAborted()) Abort();
				continue;
			}

			m_list.ResetContent();

			auto task = tasks_.pop_front();

			CAtlArray<PDBSYMBOL> tmp_symbols;
			{
				CComCritSecLock<CComCriticalSection> lock(m_symbols->m_lock);
				tmp_symbols.Copy(m_symbols->m_aSymbols);
			}

			const int threshold = 100;
			int shortest = 88;
			int index = 0;
			int shortest_index = 0;
			for (size_t i = 0; i < tmp_symbols.GetCount(); ++i)
			{
				if (IsAborted()) Abort();
				int distance = levenshteinDistance(task.text, tmp_symbols.GetAt(i).sKey);
				if(distance < threshold)
				{
					index = AddString(tmp_symbols.GetAt(i).sKey);
				}
				if(distance < shortest)
				{
					shortest = distance;
					if(index != CB_ERR)
					{
						shortest_index = index;
					}
				}
			}
        	m_list.SetCurSel(shortest_index);
			LogOut(TEXT("total:%d"), m_list.GetCount());
        }

        return 0;
    }

    BEGIN_MSG_MAP(CSearchView)
        MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
	    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
        COMMAND_CODE_HANDLER(EN_UPDATE, OnEditUpdate)
		ALT_MSG_MAP(1)
        MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnListLButtonDblClk)
    END_MSG_MAP()

private:
    LRESULT OnEditUpdate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& /*bHandled*/)
    {
		//ATLASSERT(m_edit.m_hWnd == hWndCtl);

		//CString str;
		//int nLength = m_edit.GetWindowText(str);
		//tasks_.push_back({str});
        return 0;
    }

    LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
    {
		if (VK_RETURN == wParam)
		{
			CString text;
			if(0 < this->GetWindowText(text))
			{
				AddString(text);
			}
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

   LRESULT OnEraseBkgnd(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
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
   }

   	void AdjustRect(BOOL bLarger, LPRECT lpRect)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		::SendMessage(m_hWnd, TCM_ADJUSTRECT, bLarger, (LPARAM)lpRect);
	}

    CPdbCollector* m_symbols;
    CContainedWindowT<CListBox> m_list;
    CRichEditCtrl m_edit;
};
