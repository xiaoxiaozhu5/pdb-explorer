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


class CBrowserView : public CSplitterWindowImpl<CBrowserView>
{
public:
	DECLARE_WND_CLASS(NULL)

	enum { TIMERID_POPULATE = 110 };

	enum
	{
		TIMER_START_INTERVAL = 200,
		TIMER_SLEEP_INTERVAL = 100,
		TIMER_WORK_INTERVAL = 150,
	};

	CString m_path;
	CTreeViewCtrl m_ctrlTree;
	CSimpleHtmlCtrl m_ctrlView;
	CDlgContainerCtrl m_ctrlContainer;
	CSearchView m_ctrlSearch;
	//CAnimateCtrl m_ctrlHourglass;

	CPdbCollector m_collector;
	SIZE_T m_lLastSize;
	//CRgn m_rgnWaitAnim;
	CString m_currentIndex;

	HTREEITEM m_root_treeitem;
	HTREEITEM m_udt_treeitem;
	HTREEITEM m_enum_treeitem;
	HTREEITEM m_typedef_treeitem;

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
		m_ctrlTree.SetRedraw(FALSE);
		m_ctrlTree.DeleteAllItems();
		m_ctrlTree.SetRedraw(TRUE);
		m_ctrlView.SetWindowText(_T(""));

		CString sKey;
		sKey.Format(TEXT("%s"), pstrFilename);
		m_root_treeitem = m_ctrlTree.InsertItem(sKey, -1, -1, m_ctrlTree.GetRootItem(), TVI_SORT);
		m_ctrlTree.SetItemData(m_root_treeitem, -1);

		sKey.LoadString(IDS_TYPE_UDT);
		m_udt_treeitem = m_ctrlTree.InsertItem(sKey, -1, -1, m_ctrlTree.GetRootItem(), TVI_SORT);
		m_ctrlTree.SetItemData(m_udt_treeitem, -1);
		sKey.LoadString(IDS_TYPE_ENUM);
		m_enum_treeitem = m_ctrlTree.InsertItem(sKey, -1, -1, m_ctrlTree.GetRootItem(), TVI_SORT);
		m_ctrlTree.SetItemData(m_enum_treeitem, -1);
		sKey.LoadString(IDS_TYPE_TYPEDEF);
		m_typedef_treeitem = m_ctrlTree.InsertItem(sKey, -1, -1, m_ctrlTree.GetRootItem(), TVI_SORT);
		m_ctrlTree.SetItemData(m_typedef_treeitem, -1);

		m_collector.Stop();
		m_collector.Init(m_hWnd, pstrFilename);
		m_collector.Start();

		m_ctrlSearch.Abort();
		m_ctrlSearch.Stop();
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
		if (!m_ctrlTree.IsWindow()) return;
		CComCritSecLock<CComCriticalSection> lock(m_collector.m_lock);
		SIZE_T lCurCount = m_collector.m_aSymbols.GetCount();
		if (m_lLastSize != lCurCount) _PopulateTree();
		if (m_collector.m_bDone && m_lLastSize == lCurCount)
		{
			KillTimer(TIMERID_POPULATE);
			PostMessage(WM_USER_LOAD_END);
		}
		else SetTimer(TIMERID_POPULATE, TIMER_SLEEP_INTERVAL);
	}

	BEGIN_MSG_MAP(CBrowserView)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		MESSAGE_HANDLER(WM_USER_LOAD_START, OnAnimStart)
		MESSAGE_HANDLER(WM_USER_LOAD_END, OnAnimEnd)
		NOTIFY_CODE_HANDLER(TVN_SELCHANGED, OnSelChanged)
		NOTIFY_CODE_HANDLER(EN_LINK, OnLink)
		CHAIN_MSG_MAP(CSplitterWindowImpl<CBrowserView>)
		ALT_MSG_MAP(1)
		COMMAND_ID_HANDLER(ID_EDIT_FIND, OnFind)
	    COMMAND_ID_HANDLER(IDOK, OnOk)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	    //CHAIN_COMMANDS_MEMBER(m_ctrlView)
		//FORWARD_NOTIFICATIONS()
	    REFLECT_NOTIFICATIONS()
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		DWORD dwStyle = WS_CHILD | WS_VISIBLE | TVS_HASBUTTONS | TVS_HASLINES | TVS_SHOWSELALWAYS;
		m_ctrlTree.Create(m_hWnd, rcDefault, _T(""), dwStyle, 0, IDC_TREE);
		m_ctrlView.Create(m_hWnd, rcDefault, _T(""),
					WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | 
					      ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL |
			              ES_SAVESEL | ES_SELECTIONBAR | ES_READONLY, 
			0,IDC_TREE);
		m_ctrlSearch.Create(WS_CHILD | WS_VISIBLE | CBS_SIMPLE | WS_VSCROLL | WS_HSCROLL | CBS_NOINTEGRALHEIGHT, rcDefault, m_hWnd, 0);
		m_ctrlContainer.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
		m_ctrlContainer.AddItem(m_ctrlTree);
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
		CComCritSecLock<CComCriticalSection> lock(m_collector.m_lock);
		CDiaInfo info;
		HTREEITEM hTreeSel = m_ctrlTree.GetSelectedItem();
		DWORD dwIndex = m_ctrlTree.GetItemData(hTreeSel);
		CString sRTF;
		if (dwIndex == (DWORD)-1)
		{
			if(m_ctrlTree.GetRootItem() == hTreeSel)
			{
				CString pdb_path;
				m_ctrlTree.GetItemText(hTreeSel, pdb_path);
				PDB::RawPdb raw_pdb;
				if(raw_pdb.Open(pdb_path))
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
					for(size_t i = 0u; i < feature_num; ++i)
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
		m_ctrlView.Load(sRTF);
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
				PCWSTR p = _tcsstr(szUrl, TEXT("sym://"));
				if (NULL != p)
				{
					CDiaInfo info;
					//DWORD id = _wtoi(p + 6);
					PDBSYMBOL Symbol;
					Symbol.dwSymId = 0;
					Symbol.sKey = p+6;
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
		else if(hWndCtl == m_ctrlTree)
		{
			m_ctrlContainer.SetCurSel(1);
		}
		return 0;
	}

	LRESULT OnOk(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
	{
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
		DWORD dwStartTick = ::GetTickCount();
		SIZE_T iIndex;
		for (iIndex = m_lLastSize; iIndex < m_collector.m_aSymbols.GetCount(); iIndex++)
		{
			const PDBSYMBOL& Symbol = m_collector.m_aSymbols[iIndex];
			switch (Symbol.dwSymTag)
			{
			case SymTagUDT:
				_InsertTreeNode(Symbol.sKey, m_udt_treeitem, iIndex, 0);
				break;
			case SymTagEnum:
				_InsertTreeNode(Symbol.sKey, m_enum_treeitem, iIndex, 0);
				break;
			case SymTagTypedef:
				_InsertTreeNode(Symbol.sKey, m_typedef_treeitem, iIndex, 0);
				break;
			default:
				break;
			}

			CString s, st;
			s.LoadString(IDS_PROCESSING_SYMBOL);
			st.Format(s, iIndex, m_collector.m_aSymbols.GetCount(), Symbol.sKey);
			::SendMessage(GetParent(), WM_SET_STATUS_TEXT, 0, (LPARAM)(LPCTSTR)st);
			if (::GetTickCount() - dwStartTick > TIMER_WORK_INTERVAL) break;
		}
		if (m_lLastSize == 0) _ExpandTreeNodes(m_ctrlTree.GetRootItem(), 0);
		m_lLastSize = iIndex;
	}

	void _ExpandTreeNodes(HTREEITEM hItem, int iLevel)
	{
		while (hItem != NULL)
		{
			m_ctrlTree.Expand(hItem);
			if (iLevel < 1) _ExpandTreeNodes(m_ctrlTree.GetChildItem(hItem), iLevel + 1);
			hItem = m_ctrlTree.GetNextItem(hItem, TVGN_NEXT);
		}
	}

	void _InsertTreeNode(const CString& sToken, HTREEITEM hItem, SIZE_T iIndex, int iDataLevel)
	{
		if (sToken.IsEmpty()) return;
		auto hChild = m_ctrlTree.InsertItem(sToken, -1, -1, hItem, TVI_LAST);
		m_ctrlTree.SetItemData(hChild, iIndex);
	}

	HTREEITEM _FindTreeChildItem(HTREEITEM hItem, LPCTSTR pstrName) const
	{
		hItem = m_ctrlTree.GetChildItem(hItem);
		TCHAR szName[300] = {0};
		TVITEMEX tvi = {0};
		tvi.mask = TVIF_TEXT;
		tvi.pszText = szName;
		tvi.cchTextMax = _countof(szName);
		while (hItem != NULL)
		{
			tvi.hItem = hItem;
			if (!m_ctrlTree.GetItem(&tvi)) return NULL;
			int iRes = _tcscmp(pstrName, szName);
			if (iRes == 0) return hItem;
			hItem = m_ctrlTree.GetNextItem(hItem, TVGN_NEXT);
		}
		return NULL;
	}
};
