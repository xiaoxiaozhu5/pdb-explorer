#pragma once

class CSearchView : public CWindowImpl<CSearchView, CComboBox>
{
	typedef CWindowImpl<CSearchView, CComboBox> BaseComboBox;

public:
    DECLARE_WND_CLASS(NULL)

    CSearchView() : m_list(this, 0)
    {
	    
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
		GetParent().SendMessage(WM_COMMAND, IDOK, (LPARAM)m_hWnd);
		return 0;
    }

    CContainedWindowT<CListBox> m_list;
    CEdit m_edit;
};
