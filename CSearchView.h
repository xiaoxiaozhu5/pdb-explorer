#pragma once
#include "levenshtein.h"

class CSearchView : public CWindowImpl<CSearchView, CComboBox>,
					public CThreadImpl<CSearchView>
{
	typedef CWindowImpl<CSearchView, CComboBox> BaseComboBox;

public:
    DECLARE_WND_CLASS(NULL)

    CSearchView() : m_list(this, 0),
					m_symbols(NULL),
					m_count(0)
    {
    }

    BOOL Create(DWORD dwStyle, LPRECT lpRect, HWND hWndParent, UINT nID)
    {
        auto hCom = BaseComboBox::Create(hWndParent, *lpRect, NULL, dwStyle, 0, nID);
		if (NULL == hCom)
			return FALSE;
		m_list.SubclassWindow(hCom);
		m_edit = FindWindowEx(hCom, NULL, WC_EDIT, NULL);

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
        //while(m_list.DeleteString(0));
        m_symbols = collector;
    }

    DWORD Run()
    {
        while(!IsAborted())
        {
			if (IsAborted()) Abort();

			CAtlArray<PDBSYMBOL> tmp_symbols;
			{
				CComCritSecLock<CComCriticalSection> lock(m_symbols->m_lock);
				tmp_symbols.Copy(m_symbols->m_aSymbols);
			}
			size_t old = m_count;
			size_t update = tmp_symbols.GetCount();
			if (update > old)
			{
				for (size_t i = m_count; i < update; ++i)
				{
					if (IsAborted()) Abort();
					AddString(tmp_symbols.GetAt(i).sKey);
				}
				m_count = update;
			}

			Sleep(200);
        }

        return 0;
    }

    BEGIN_MSG_MAP(CSearchView)
        MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
	    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
        COMMAND_CODE_HANDLER(EN_UPDATE, OnEditUpdate)
        MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnListLButtonDblClk)
    END_MSG_MAP()

private:
    LRESULT OnEditUpdate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& /*bHandled*/)
    {
		ATLASSERT(m_edit.m_hWnd == hWndCtl);

		CString str;
		int nLength = m_edit.GetWindowText(str);
		if (nLength > 0) {
			int idx = FindString(str);
			if (CB_ERR != idx) {
				// Must use CListBoxT::SetCurSel,
				// because CComboBoxT::SetCurSel will change edit's text.
				m_list.SetCurSel(idx);
				m_edit.SetSel(nLength, -1);
			}
		}
        return 0;
    }

    LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
    {
		if (VK_RETURN == wParam) ::PostMessage(GetParent(), WM_COMMAND, MAKEWPARAM(IDOK, 0), reinterpret_cast<LPARAM>(m_hWnd));
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

    size_t m_count;
    CPdbCollector* m_symbols;
    CContainedWindowT<CListBox> m_list;
    CEdit m_edit;
};
