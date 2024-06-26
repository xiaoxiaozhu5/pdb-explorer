#pragma once
#include <unordered_map>
#include <iterator>
#include <algorithm>
#include <map>

extern "C" {
	#include "fzf/fzf.h"
}

#include <strsafe.h>

#include "tsqueue.h"

template <typename K, typename V>
std::size_t flatten(std::multimap<K,V,std::greater<V>>& m, std::vector<V>& v)
{
    v.clear();
    v.reserve(m.size());
    std::transform(m.begin(), m.end(),
                   std::back_inserter(v),
                   std::bind(&std::multimap<K,V,std::greater<V>>::value_type::second, std::placeholders::_1));
    return v.size();
}

class CSearchView : public CWindowImpl<CSearchView>,
					public CThreadImpl<CSearchView>,
					public CCustomDraw<CSearchView>
{
public:
    DECLARE_WND_CLASS(_T("WTL_SearchView"))

	CFont m_font;
	CFont m_font_bold;
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


		lf.lfWeight = FW_BOLD;
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
		m_font_bold.CreateFontIndirect(&lf);
	}

	BEGIN_MSG_MAP(CSearchView)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		COMMAND_CODE_HANDLER(EN_CHANGE, OnEditChange)
		NOTIFY_CODE_HANDLER(LVN_GETDISPINFO, OnTvnGetdispinfoTree)
		NOTIFY_CODE_HANDLER(NM_DBLCLK, OnSelChanged)
		CHAIN_MSG_MAP( CCustomDraw<CSearchView> )
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
			    //CA2W wstr(Symbol.sKey.c_str());
				//pDetails->item.pszText = wstr;
				pDetails->item.iSubItem = Symbol.dwSymTag;
				StringCchPrintf(pDetails->item.pszText,
					pDetails->item.cchTextMax, _T("%S"), Symbol.sKey.c_str());
				//StringCchPrintf(pDetails->item.pszText,
				//	pDetails->item.cchTextMax, _T("%d %s"), pDetails->item.iItem, (LPCTSTR)Symbol.sKey);
			}
			else
			{
				auto index = m_filtered_index[pDetails->item.iItem];
				const PDBSYMBOL& Symbol = m_symbols->m_aSymbols[index];
			    //CA2W wstr(Symbol.sKey.c_str());
				//pDetails->item.pszText = wstr;
				pDetails->item.iSubItem = Symbol.dwSymTag;
				StringCchPrintf(pDetails->item.pszText,
					pDetails->item.cchTextMax, _T("%S"), Symbol.sKey.c_str());
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
		CT2CA atext(text);
		::SendMessage(GetParent(), WM_SHOW_ITEM, pDetails->iSubItem, (LPARAM)(LPCSTR)atext);
		return 0;
	}

	DWORD OnPreErase(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/)
	{
		return CDRF_SKIPDEFAULT;
	}

	DWORD OnPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/)
	{
		return CDRF_NOTIFYITEMDRAW;
	}

	DWORD OnItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW lpNMCustomDraw)
	{
		LPNMTBCUSTOMDRAW lpNMTBCD = reinterpret_cast<LPNMTBCUSTOMDRAW>(lpNMCustomDraw);
		CDCHandle dc = lpNMTBCD->nmcd.hdc;

		DWORD row = lpNMTBCD->nmcd.dwItemSpec;
		bool bIsFocus = (lpNMCustomDraw->uItemState & CDIS_FOCUS) != 0;

		// Draw Selected item Background
		CPen pen;
		CBrush brush;
		pen.CreatePen(PS_DOT, 1, RGB(0, 0, 0));
		if(bIsFocus)
			brush.CreateSolidBrush(::GetSysColor(CTLCOLOR_LISTBOX));
		else
			brush.CreateSolidBrush(RGB(255, 255, 255));

		HPEN hOldPen = dc.SelectPen(pen);
		HBRUSH hOldBrush = dc.SelectBrush(brush);
		dc.FillRect(&lpNMTBCD->nmcd.rc, brush);
		dc.SelectBrush(hOldBrush);
		dc.SelectPen(hOldPen);

		CString text;
		m_list.GetItemText(row, 0, text);

		const int max_match_index = 512;
		uint32_t match_idx[max_match_index] = {0};
		CString input;
		m_search.GetWindowTextW(input);
		if(!input.IsEmpty())
		{
			CT2CA ainput(input);
			CT2CA atext(text);
			fzf_slab_t* fzf_flab = fzf_make_default_slab();
			fzf_pattern_t* fzf_pattern = fzf_parse_pattern(CaseSmart, false, ainput, true);
			int score = fzf_get_score(atext, fzf_pattern, fzf_flab);
			if (score > 0)
			{
				fzf_position_t* pos = fzf_get_positions(atext, fzf_pattern, fzf_flab);
				if (pos)
				{
					for (size_t i = 0; i < pos->size; ++i)
					{
						match_idx[pos->data[i]] = 1;
					}
					fzf_free_positions(pos);
				}
			}
			fzf_free_pattern(fzf_pattern);
		}

		RECT rect = lpNMTBCD->nmcd.rc;
		int x = rect.left;
		SIZE sz;
		for(int i = 0; i < text.GetLength(); ++i)
		{
			TCHAR ch[2] = { text[i], '\0' };
			if(match_idx[i])
			{
				HFONT hOldFont = dc.SelectFont(m_font_bold);
				dc.DrawTextEx(ch, 1, &rect, DT_LEFT, NULL);

				dc.GetTextExtent(ch, 1, &sz);
				x += sz.cx;
				rect.left = x;
				dc.SelectFont(hOldFont);
			}
			else
			{
				dc.DrawTextEx(ch, 1, &rect, DT_LEFT, NULL);
				dc.GetTextExtent(ch, 1, &sz);
				x += sz.cx;
				rect.left = x;
			}
		}

		return CDRF_SKIPDEFAULT;
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
			    m_list.SetRedraw(FALSE);
				m_list.DeleteAllItems();
			    m_list.SetRedraw(TRUE);
				if (IsAborted()) Abort();
				m_list.SetItemCountEx(tmp_symbols.GetCount(), LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);
				continue;
			}
			
			m_list.SetRedraw(FALSE);
			m_list.DeleteAllItems();
			m_list.SetRedraw(TRUE);
			std::multimap<int, int, std::greater<int>> score_index_map;
			std::vector<int> filtered_index;
			if(m_search_filter.empty())
			{
				for (size_t i = 0; i < tmp_symbols.GetCount(); ++i)
				{
					fzf_pattern_t* fzf_pattern = fzf_parse_pattern(CaseSmart, false, const_cast<char*>(task.text.c_str()), true);
					int score = fzf_get_score(tmp_symbols[i].sKey.c_str(), fzf_pattern, fzf_flab);
					if (score > 0)
					{
						score_index_map.insert({score, i});
					}
					fzf_free_pattern(fzf_pattern);
				}
				flatten(score_index_map, filtered_index);
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
						int score = fzf_get_score(tmp_symbols[m_filtered_index[i]].sKey.c_str(), fzf_pattern, fzf_flab);
						if (score > 0)
						{
						    score_index_map.insert({score, m_filtered_index[i]});
						}
						fzf_free_pattern(fzf_pattern);
					}
				    flatten(score_index_map, filtered_index);
					m_search_filter.insert(std::make_pair(task.text, filtered_index));
				}
				else
				{
					auto& findex = find_item->second;
					for (size_t i = 0; i < findex.size(); ++i)
					{
						filtered_index.push_back(findex[i]);
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

