#pragma once
#include "levenshtein.h"

class CSearchView : public CWindowImpl<CSearchView, CComboBox>
{
	typedef CWindowImpl<CSearchView, CComboBox> BaseComboBox;

public:
    DECLARE_WND_CLASS(NULL)

    CSearchView() : m_list(this, 0)
    {
        m_symbols = NULL;
    }

    BOOL Create(DWORD dwStyle, LPRECT lpRect, HWND hWndParent, UINT nID)
    {
		if (NULL == BaseComboBox::Create(hWndParent, *lpRect, NULL, dwStyle, 0, nID))
			return FALSE;
		m_list.SubclassWindow(FindWindowEx(m_hWnd, NULL, _T("ComboLBox"), NULL));
		m_edit = FindWindowEx(m_hWnd, NULL, WC_EDIT, NULL);

		return TRUE;
    }

    int FindString(PCTSTR lpszString)
    {
	    return m_list.FindString(0, lpszString);
    }

    void SetDataSource(const CAtlArray<PDBSYMBOL>* symbols)
    {
        m_symbols = symbols;
    }

    BEGIN_MSG_MAP(CSearchView)
        MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
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
            CString find;
            int shortest_distance = levenshteinDistance(m_symbols->GetAt(0).sKey, str);
            for(int i = 0; i < m_symbols->GetCount(); ++i)
            {
	            int distance = levenshteinDistance(m_symbols->GetAt(i).sKey, str);
                if(distance < shortest_distance)
                {
	                shortest_distance = distance;
                    find = m_symbols->GetAt(i).sKey;
                }
            }

            AddString(find);

			//int idx = FindString(str);
			//if (CB_ERR != idx) {
			//	// Must use CListBoxT::SetCurSel,
			//	// because CComboBoxT::SetCurSel will change edit's text.
			//	m_list.SetCurSel(idx);
			//	m_edit.SetSel(nLength, -1);
			//}
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

    const CAtlArray<PDBSYMBOL> *m_symbols;
    CContainedWindowT<CListBox> m_list;
    CEdit m_edit;
};
