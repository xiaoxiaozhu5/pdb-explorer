#pragma once

enum
{
	WM_USER_LOAD_START = WM_APP + 100,
	WM_USER_LOAD_END = WM_APP + 101,
};

#include "Collector.h"
#include "DiaInfo.h"

#include "SimpleHtml.h"


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

	CTreeViewCtrl m_ctrlTree;
	CSimpleHtmlCtrl m_ctrlView;
	CAnimateCtrl m_ctrlHourglass;

	CPdbCollector m_collector;
	SIZE_T m_lLastSize;
	CRgn m_rgnWaitAnim;

	BOOL PreTranslateMessage(MSG* pMsg)
	{
		return FALSE;
	}

	void LoadFromFile(LPCTSTR pstrFilename)
	{
		CWaitCursor cursor;

		KillTimer(TIMERID_POPULATE);

		m_collector.Abort();

		if (!m_ctrlView.IsWindow()) return;
		m_ctrlTree.SetRedraw(FALSE);
		m_ctrlTree.DeleteAllItems();
		m_ctrlTree.SetRedraw(TRUE);
		m_ctrlView.SetWindowText(_T(""));

		CString sKey(L"Source Files"), sToken(L"Source Files");
		_InsertTreeNode(sKey, sToken, m_ctrlTree.GetRootItem(), 0, 0);

		m_collector.Stop();
		m_collector.Init(m_hWnd, pstrFilename);
		m_collector.Start();

		m_lLastSize = 0;
		SetTimer(TIMERID_POPULATE, TIMER_START_INTERVAL);
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
		END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		DWORD dwStyle = WS_CHILD | WS_VISIBLE | TVS_HASBUTTONS | TVS_HASLINES | TVS_SHOWSELALWAYS;
		m_ctrlTree.Create(m_hWnd, rcDefault, _T(""), dwStyle, 0, IDC_TREE);
		m_ctrlView.Create(m_hWnd, rcDefault, _T(""),
		                  WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY, 0,
		                  IDC_TREE);
		RECT rcAnim = {10, 10, 10 + 32, 10 + 32};
		m_ctrlHourglass.Create(m_ctrlView, rcAnim, _T(""), WS_CHILD | ACS_CENTER | ACS_TRANSPARENT, 0, IDC_HOURGLASS);
		m_ctrlHourglass.Open(IDR_HOURGLASS);
		RECT rcHourglass = {0};
		m_ctrlHourglass.GetClientRect(&rcHourglass);
		m_rgnWaitAnim.CreateRoundRectRgn(0, 0, rcAnim.right - rcAnim.left, rcAnim.bottom - rcAnim.top, 12, 12);
		m_ctrlHourglass.SetWindowRgn(m_rgnWaitAnim, FALSE);
		SetSplitterPosPct(20);
		SetSplitterPanes(m_ctrlTree, m_ctrlView);
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
		m_ctrlHourglass.ShowWindow(SW_SHOWNOACTIVATE);
		m_ctrlHourglass.Play(0, (UINT)-1, (UINT)-1);
		return 0;
	}

	LRESULT OnAnimEnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		m_ctrlHourglass.ShowWindow(SW_HIDE);
		m_ctrlHourglass.Stop();

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
		DWORD dwIndex = m_ctrlTree.GetItemData(m_ctrlTree.GetSelectedItem());
		CString sRTF;
		if (dwIndex == (DWORD)-1)
		{
			sRTF = info.GetEmptyInfo(m_ctrlTree);
		}
		else
		{
			PDBSYMBOL Symbol = m_collector.m_aSymbols[dwIndex];
			sRTF = info.GetSymbolInfo(m_collector.m_global_sym, Symbol);
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
				MessageBoxW(szUrl);
			}
		}
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
			CString sKey = Symbol.sKey;
			CString sToken = sKey.SpanExcluding(_T("|"));
			_InsertTreeNode(sKey, sToken, m_ctrlTree.GetRootItem(), iIndex, 0);
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

	void _InsertTreeNode(CString& sKey, CString& sToken, HTREEITEM hItem, SIZE_T iIndex, int iDataLevel)
	{
		if (sToken.IsEmpty()) return;
		bool bLastToken = (sKey.Find('|') < 0);
		HTREEITEM hChild = _FindTreeChildItem(hItem, sToken);
		if (hChild == NULL)
		{
			hChild = m_ctrlTree.InsertItem(sToken, -1, -1, hItem, TVI_SORT);
			m_ctrlTree.SetItemData(hChild, bLastToken ? iIndex : (DWORD)-1);
		}
		else if (bLastToken)
		{
			m_ctrlTree.SetItemData(hChild, iIndex);
		}
		CString sNewKey = sKey.Mid(sToken.GetLength() + 1);
		CString sNewToken = sNewKey.SpanExcluding(_T("|"));
		_InsertTreeNode(sNewKey, sNewToken, hChild, iIndex, iDataLevel);
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
