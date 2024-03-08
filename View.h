#pragma once

enum
{
	WM_USER_LOAD_START = WM_APP + 100,
	WM_USER_LOAD_END = WM_APP + 101,
};

#include "Collector.h"
#include "DiaInfo.h"

#include "SimpleHtml.h"
#include "DlgTabCtrl.h"
#include "CSearchView.h"
#include "RawPdb.h"
#include "VirtualListView.h"

namespace 
{
    CString ValidateName(LPCTSTR szName)
    {
        CString tmp(szName);
        tmp.Replace(TEXT("&amp;"), TEXT("&"));
        tmp.Replace(TEXT("&lt;"), TEXT("<"));
        tmp.Replace(TEXT("&gt;"), TEXT(">"));
        return tmp;
    }
}


class CBrowserView : public CSplitterWindowImpl<CBrowserView>
{
public:
	DECLARE_WND_CLASS(NULL)

	enum { TIMERID_POPULATE = 110 };

	enum
	{
		TIMER_START_INTERVAL = 200,
		TIMER_SLEEP_INTERVAL = 50,
		TIMER_WORK_INTERVAL = 200,
	};

	CString m_path;
	//CTreeViewCtrl m_ctrlTree;
	CComObject<CGroupedVirtualModeView>* m_ctrlList;
	CSimpleHtmlCtrl m_ctrlView;
	CDlgContainerCtrl m_ctrlContainer;
	CSearchView m_ctrlSearch;
	//CAnimateCtrl m_ctrlHourglass;

	CPdbCollector m_collector;
	SIZE_T m_lLastSize;
	//CRgn m_rgnWaitAnim;
	CString m_currentIndex;

	BOOL PreTranslateMessage(MSG* pMsg)
	{
		return FALSE;
	}

	void LoadFromFile(LPCTSTR pstrFilename)
	{
		m_path = pstrFilename;
		CWaitCursor cursor;

		KillTimer(TIMERID_POPULATE);

		m_collector.Abort();

		if (!m_ctrlView.IsWindow()) return;
		m_ctrlList->DeleteAllItems();
		m_ctrlView.SetWindowText(_T(""));

		m_collector.Stop();
		m_collector.Init(m_hWnd, pstrFilename);
		m_collector.Start();

		//m_ctrlSearch.Abort();
		//m_ctrlSearch.Stop();
		m_ctrlSearch.SetDataSource(&m_collector);
		m_ctrlSearch.Start();

		m_lLastSize = 0;
		SetTimer(TIMERID_POPULATE, TIMER_START_INTERVAL);
	}

	void ShowSymbol(const CString& id)
	{
		CDiaInfo info;
		PDBSYMBOL Symbol;
		Symbol.sKey = id;
		CString sRTF = info.GetSymbolInfo(m_path, Symbol);
		m_currentIndex = id;
		m_ctrlView.Load(sRTF);
	}

	void ProcessTreeUpdates()
	{
		if (m_collector.m_bDone)
		{
			CComCritSecLock<CComCriticalSection> lock(m_collector.m_lock);
			_PopulateTree();
			KillTimer(TIMERID_POPULATE);
			PostMessage(WM_USER_LOAD_END);
		}
		else
		{
			SetTimer(TIMERID_POPULATE, TIMER_SLEEP_INTERVAL);
		}
	}

	BEGIN_MSG_MAP(CBrowserView)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		MESSAGE_HANDLER(WM_USER_LOAD_START, OnAnimStart)
		MESSAGE_HANDLER(WM_USER_LOAD_END, OnAnimEnd)
		NOTIFY_CODE_HANDLER(NM_DBLCLK, OnSelChanged)
		NOTIFY_CODE_HANDLER(LVN_GETDISPINFO, OnTvnGetdispinfoTree)
		NOTIFY_CODE_HANDLER(EN_LINK, OnLink)
		CHAIN_MSG_MAP(CSplitterWindowImpl<CBrowserView>)
		ALT_MSG_MAP(1)
		COMMAND_ID_HANDLER(ID_EDIT_FIND, OnFind)
	    COMMAND_ID_HANDLER(IDOK, OnOk)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	    CHAIN_COMMANDS_MEMBER(m_ctrlView)
		//FORWARD_NOTIFICATIONS()
	    REFLECT_NOTIFICATIONS()
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		CComObject<CGroupedVirtualModeView>::CreateInstance(&m_ctrlList);
		m_ctrlList->Create(m_hWnd, rcDefault, NULL, WS_VSCROLL  |
				WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | LVS_REPORT |
				LVS_SHOWSELALWAYS | LVS_AUTOARRANGE | LVS_ALIGNTOP | LVS_OWNERDATA, 
				0);
		m_ctrlView.Create(m_hWnd, rcDefault, _T(""),
					WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | 
					      ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL |
			              ES_SAVESEL | ES_SELECTIONBAR | ES_READONLY, 
			0,IDC_TREE);
		m_ctrlView.LimitText();
		m_ctrlSearch.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE);
		m_ctrlContainer.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
		m_ctrlContainer.AddItem(m_ctrlList->m_hWnd);
		m_ctrlContainer.AddItem(m_ctrlSearch);
		m_ctrlContainer.SetCurSel(0);

		/*
		RECT rcAnim = {10, 10, 10 + 32, 10 + 32};
		m_ctrlHourglass.Create(m_ctrlView, rcAnim, _T(""), WS_CHILD | ACS_CENTER | ACS_TRANSPARENT, 0, IDC_HOURGLASS);
		m_ctrlHourglass.Open(IDR_HOURGLASS);
		RECT rcHourglass = {0};
		m_ctrlHourglass.GetClientRect(&rcHourglass);
		m_rgnWaitAnim.CreateRoundRectRgn(0, 0, rcAnim.right - rcAnim.left, rcAnim.bottom - rcAnim.top, 12, 12);
		m_ctrlHourglass.SetWindowRgn(m_rgnWaitAnim, FALSE);
		*/
		SetSplitterPosPct(20);
		SetSplitterPanes(m_ctrlContainer, m_ctrlView);
		bHandled = FALSE;

		return 0;
	}

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		m_ctrlList->EnableGroupView(FALSE);
		m_collector.Stop();
		bHandled = FALSE;
		return 0;
	}

	LRESULT OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
	{
		if (wParam == TIMERID_POPULATE) ProcessTreeUpdates();
		bHandled = FALSE;
		return 0;
	}

	LRESULT OnAnimStart(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		//m_ctrlHourglass.ShowWindow(SW_SHOWNOACTIVATE);
		//m_ctrlHourglass.Play(0, (UINT)-1, (UINT)-1);

		CString s;
		s.LoadString(IDS_START_PROCESS);
		::SendMessage(GetParent(), WM_SET_STATUS_TEXT, 0, (LPARAM)(LPCTSTR)s);
		return 0;
	}

	LRESULT OnAnimEnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		//m_ctrlHourglass.ShowWindow(SW_HIDE);
		//m_ctrlHourglass.Stop();

		CString s;
		s.LoadString(IDS_TOTAL_SYMBOL);
		CString st;
		st.Format(s, m_lLastSize);
		::SendMessage(GetParent(), WM_SET_STATUS_TEXT, 0, (LPARAM)(LPCTSTR)st);
		return 0;
	}

	LRESULT OnSelChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
	{
		NM_LISTVIEW* pDetails = reinterpret_cast<NM_LISTVIEW*>(pnmh);
		CDiaInfo info;
		CString sRTF;
		CComCritSecLock<CComCriticalSection> lock(m_collector.m_lock);
		if(pDetails->iItem < 0 || pDetails->iItem > m_collector.m_aSymbols.GetCount())
			return 0;
		PDBSYMBOL Symbol = m_collector.m_aSymbols[pDetails->iItem];
		sRTF = info.GetSymbolInfo(m_path, Symbol);
		m_currentIndex = Symbol.sKey;
		::SendMessage(GetParent(), WM_ADD_HISTORY, (WPARAM)Symbol.sKey.AllocSysString(), 0);


#if 0
		CComCritSecLock<CComCriticalSection> lock(m_collector.m_lock);
		CDiaInfo info;
		HTREEITEM hTreeSel = m_ctrlTree.GetSelectedItem();
		DWORD dwIndex = m_ctrlTree.GetItemData(hTreeSel);
		CString sRTF;
		if (dwIndex == (DWORD)-1)
		{
			if (m_ctrlTree.GetRootItem() == hTreeSel)
			{
				CString pdb_path;
				m_ctrlTree.GetItemText(hTreeSel, pdb_path);
				PDB::RawPdb raw_pdb;
				if (raw_pdb.Open(pdb_path))
				{
					CString pdb_info;
					auto pdb_hdr = raw_pdb.GetHeader();
					pdb_info.Format(_T("Version %u <br />signature %u <br />age %u <br />GUID %08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x <br />Block Size %lu <br />Number of blocks %lu <br />Number of streams %lu <br />"),
						static_cast<uint32_t>(pdb_hdr->version), pdb_hdr->signature, pdb_hdr->age,
						pdb_hdr->guid.Data1, pdb_hdr->guid.Data2, pdb_hdr->guid.Data3,
						pdb_hdr->guid.Data4[0], pdb_hdr->guid.Data4[1], pdb_hdr->guid.Data4[2], pdb_hdr->guid.Data4[3], pdb_hdr->guid.Data4[4], pdb_hdr->guid.Data4[5], pdb_hdr->guid.Data4[6], pdb_hdr->guid.Data4[7],
						raw_pdb.GetBlockSize(), raw_pdb.GetBlockCount(), raw_pdb.GetStremCount());
					size_t feature_num = 0;
					auto feature = raw_pdb.GetFeature(&feature_num);
					CString sFeature(_T(" Feature ["));
					for (size_t i = 0u; i < feature_num; ++i)
					{
						if (feature[i] == 0)
							continue;
						CString fea;
						fea.Format(_T("%08lu(0x%08X)"), feature[i], feature[i]);
						sFeature += fea;
						sFeature += _T(" ");
					}
					sFeature += _T("]");
					pdb_info += sFeature;
					sRTF = pdb_info;
				}
			}
			else
			{
				sRTF = info.GetEmptyInfo(m_ctrlTree);
			}
		}
		else
		{
			PDBSYMBOL Symbol = m_collector.m_aSymbols[dwIndex];
			sRTF = info.GetSymbolInfo(m_path, Symbol);
			m_currentIndex = Symbol.sKey;
			::SendMessage(GetParent(), WM_ADD_HISTORY, (WPARAM)Symbol.sKey.AllocSysString(), 0);
		}
#endif
		m_ctrlView.Load(sRTF);
		return 0;
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

		CComCritSecLock<CComCriticalSection> lock(m_collector.m_lock);
		const PDBSYMBOL& Symbol = m_collector.m_aSymbols[pItem->iItem];
		pItem->pszText = const_cast<LPTSTR>((LPCTSTR)Symbol.sKey);
		//StringCchPrintf(pItem->pszText,
		//pItem->cchTextMax, _T("%d %s"), pItem->iItem, (LPCTSTR)Symbol.sKey);
		bHandled = TRUE;
		return 0;
	}

	LRESULT OnLink(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
	{
		TCHAR szUrl[MAX_PATH] = {0};
		ENLINK *pEnLink = (ENLINK *)pnmh;	
		if(pEnLink->msg == WM_LBUTTONDOWN)
		{
			if (m_ctrlView.ExtractLink(pEnLink->chrg, szUrl, MAX_PATH))
			{
				PTSTR p = _tcsstr(szUrl, TEXT("sym://"));
				if (NULL != p)
				{
					PTSTR url = p+6;
					PTSTR slash = _tcschr(url, '/');
					slash[0] = '\0';
					CDiaInfo info;
					PDBSYMBOL Symbol;
					Symbol.dwSymTag = _ttoi(slash + 1);
					Symbol.sKey=ValidateName(url);
					CString sRTF = info.GetSymbolInfo(m_path, Symbol);
					m_currentIndex = Symbol.sKey;
					::SendMessage(GetParent(), WM_ADD_HISTORY, (WPARAM)Symbol.sKey.AllocSysString(), 0);
					m_ctrlView.Load(sRTF);
				}
			}
		}
		return 0;
	}

	LRESULT OnFind(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
	{
		bHandled = TRUE;
		HWND hWndCtl = ::GetFocus();
		if(hWndCtl == m_ctrlView)
		{
			bHandled = FALSE;
		}
		else if(hWndCtl == m_ctrlList->m_hWnd)
		{
			m_ctrlContainer.SetCurSel(1);
		}
		return 0;
	}

	LRESULT OnOk(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
	{
		PDBSYMBOL Symbol{};
		if(0 < m_ctrlSearch.GetWindowText(Symbol.sKey))
		{
			CDiaInfo info;
			auto sRTF = info.GetSymbolInfo(m_path, Symbol);
			m_ctrlView.Load(sRTF);
		}
		return 0;
	}

	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
	{
		m_ctrlContainer.SetCurSel(0);
		return 0;
	}

	// Implementation

	void _PopulateTree()
	{
		//DWORD dwStartTick = ::GetTickCount();
		SIZE_T iIndex;
		int cnt_udf = 0, cnt_enum = 0, cnt_typedef = 0;
		for (iIndex = 0; iIndex < m_collector.m_aSymbols.GetCount(); iIndex++)
		{
			const PDBSYMBOL& Symbol = m_collector.m_aSymbols[iIndex];
			switch (Symbol.dwSymTag)
			{
			case SymTagUDT:
				cnt_udf++;
				break;
			case SymTagEnum:
				cnt_enum++;
				break;
			case SymTagTypedef:
				cnt_typedef++;
				break;
			default:
				break;
			}

			//if (::GetTickCount() - dwStartTick > TIMER_WORK_INTERVAL)
			//{
			//	CString s, st;
			//	s.LoadString(IDS_PROCESSING_SYMBOL);
			//	st.Format(s, iIndex, m_collector.m_aSymbols.GetCount(), Symbol.sKey);
			//	::SendMessage(GetParent(), WM_SET_STATUS_TEXT, 0, (LPARAM)(LPCTSTR)st);
			//	break;
			//}
		}
		m_lLastSize = m_collector.m_aSymbols.GetCount();
		m_ctrlList->InsertGroups(cnt_udf, cnt_enum, cnt_typedef);
		m_ctrlList->SetItemCount(m_collector.m_aSymbols.GetCount());
	}
};
