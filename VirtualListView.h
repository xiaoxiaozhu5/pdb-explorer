#pragma once


#include "IListView.h"


// {A08A0F2D-0647-4443-9450-C460F4791046}
DEFINE_GUID(CLSID_CGroupedVirtualModeView, 0xa08a0f21, 0x647, 0x4443, 0x94, 0x50, 0xc4, 0x60, 0xf4, 0x79, 0x10, 0x46);

#define LVM_QUERYINTERFACE (LVM_FIRST + 189)

__inline 
BOOL IsWin7(void)
{
	OSVERSIONINFO ovi = { sizeof(OSVERSIONINFO) };
	if(GetVersionEx(&ovi)) {
		return ((ovi.dwMajorVersion == 6 && ovi.dwMinorVersion >= 1) || ovi.dwMajorVersion > 6);
	}
	return FALSE;
}


class CGroupedVirtualModeView :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CGroupedVirtualModeView, &CLSID_CGroupedVirtualModeView>,
	public CWindowImpl<CGroupedVirtualModeView, CListViewCtrl>,
	//public CCustomDraw<CGroupedVirtualModeView>, // для перехвата WM_NOTIFY, NM_CUSTOMDRAW
	public IOwnerDataCallback
{
#define ITEMCOUNT 999999
#define ITEMSPERGROUP 333333

public:
	DECLARE_WND_SUPERCLASS(NULL, CListViewCtrl::GetWndClassName())

	BOOL PreTranslateMessage(MSG* pMsg)
	{
		pMsg;
		return FALSE;
	}

	BEGIN_COM_MAP(CGroupedVirtualModeView)
		COM_INTERFACE_ENTRY_IID(IID_IOwnerDataCallback, IOwnerDataCallback)
	END_COM_MAP()

	BEGIN_MSG_MAP(CGroupedVirtualModeView)
		REFLECTED_NOTIFY_CODE_HANDLER(LVN_GETDISPINFO, OnGetDispInfo)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_NCCALCSIZE, OnNonClientCalcSize)
		//CHAIN_MSG_MAP_ALT(CCustomDraw<CGroupedVirtualModeView>, 1)
		DEFAULT_REFLECTION_HANDLER()
		REFLECT_NOTIFICATIONS()
	END_MSG_MAP()

	// Handler prototypes (uncomment arguments if needed):
	//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /* bHandle */)
	{
		CRect rect;
		GetClientRect(&rect);
		SetColumnWidth(0, rect.Width());
		return 0;
	}

	LRESULT OnNonClientCalcSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /* bHandle */)
	{
		ShowScrollBar(SB_HORZ, false);
		return DefWindowProc(uMsg, wParam, lParam); // let the vertical scroll draw
	}

	DWORD OnPrePaint(int idCtrl, LPNMCUSTOMDRAW nmdc)
	{
		// Запрашиваем уведомления NM_CUSTOMDRAW для каждого элемента списка.
		return CDRF_NOTIFYITEMDRAW;
	}

	DWORD OnItemPrePaint(int idCtrl, LPNMCUSTOMDRAW nmdc)
	{
		return CDRF_NOTIFYSUBITEMDRAW;
	}

	DWORD OnItemPostPaint(int idCtrl, LPNMCUSTOMDRAW nmcd)
	{
		NMLVCUSTOMDRAW* lvcd = reinterpret_cast<NMLVCUSTOMDRAW*>(nmcd);
		long row=nmcd->dwItemSpec;

		CRect iconRect;
		GetItemRect(row, &iconRect, LVIR_BOUNDS);
		//InvalidateRect(iconRect, true);
		FillRect(nmcd->hdc, &iconRect, (HBRUSH) (COLOR_WINDOW+1));

		return CDRF_DODEFAULT;
	}


	DWORD OnItemPreErase (int idCtrl, LPNMCUSTOMDRAW nmcd)
	{
		NMLVCUSTOMDRAW* lvcd = reinterpret_cast<NMLVCUSTOMDRAW*>(nmcd);
		long row=nmcd->dwItemSpec;

		CRect iconRect;
		GetItemRect(row, &iconRect, LVIR_BOUNDS);
		InvalidateRect(iconRect, true);

		return CDRF_DODEFAULT;
	}

	DWORD OnSubItemPrePaint (int /*idCtrl*/, LPNMCUSTOMDRAW nmcd)
	{
		NMLVCUSTOMDRAW* lvcd = reinterpret_cast<NMLVCUSTOMDRAW*>(nmcd);
		long row=nmcd->dwItemSpec;
		
		LPCWSTR ss1 = _T("Maxim Sokhatsky");
		LPCWSTR ss2 = _T("maxim@synrc.com");
		LPCWSTR ss3 = _T("+380 67 663 18 70");

		CRect rect;
		GetItemRect(row, &rect, LVIR_BOUNDS);

		CRect iconRect;
		GetItemRect(row, &iconRect, LVIR_ICON);
	
		//int mark = ListView_GetSelectionMark(m_hWnd);
		//if(mark != 0){
		//	ListView_RedrawItems(m_hWnd, row, row);	
		//}
		iconRect.InflateRect(-10,-10,-10,-10);
		//InvalidateRect(rect);		
    		//FillRect(nmcd->hdc, &iconRect,(HBRUSH)(COLOR_WINDOW));
    	//Invalidate();
		rect.OffsetRect(70,4);
		DrawText(nmcd->hdc, ss1, wcslen(ss1), rect, 
			DT_END_ELLIPSIS | DT_TOP);

		rect.OffsetRect(0,17);
		DrawText(nmcd->hdc, ss2, wcslen(ss2), rect, 
			DT_END_ELLIPSIS | DT_TOP | DT_SINGLELINE);

		rect.OffsetRect(0,17);
		DrawText(nmcd->hdc, ss3, wcslen(ss3), rect, 
			DT_END_ELLIPSIS | DT_TOP | DT_SINGLELINE);

		return CDRF_SKIPDEFAULT;
	}

	// implementation of IOwnerDataCallback
	virtual STDMETHODIMP GetItemPosition(int itemIndex, LPPOINT pPosition)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP SetItemPosition(int itemIndex, POINT position)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP GetItemInGroup(int groupIndex, int groupWideItemIndex, PINT pTotalItemIndex)
	{
		// we want group 0 to contain items 0, 3, 6...
		//         group 1            items 1, 4, 7...
		//         group 2            items 2, 5, 8...
		*pTotalItemIndex = groupIndex + groupWideItemIndex * 3;
		return S_OK;
	}

	virtual STDMETHODIMP GetItemGroup(int itemIndex, int occurenceIndex, PINT pGroupIndex)
	{
		// group 0 contains items 0, 3, 6...
		// group 1 contains items 1, 4, 7...
		// group 2 contains items 2, 5, 8...
		*pGroupIndex = itemIndex % 3;
		return S_OK;
	}

	virtual STDMETHODIMP GetItemGroupCount(int itemIndex, PINT pOccurenceCount)
	{
		// keep the one-item-in-multiple-groups stuff for another articel :-)
		*pOccurenceCount = 1;
		return S_OK;
	}

	virtual STDMETHODIMP OnCacheHint(LVITEMINDEX firstItem, LVITEMINDEX lastItem)
	{
		return E_NOTIMPL;
	}
	// implementation of IOwnerDataCallback

	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		LRESULT lr = DefWindowProc(uMsg, wParam, lParam);

		InsertColumn(0, _T("Item Name"), LVCFMT_LEFT, 300);
		ModifyStyle(LVS_REPORT |LVS_NOCOLUMNHEADER, 
			LVS_REPORT|LVS_NOCOLUMNHEADER);
		SetExtendedListViewStyle (LVS_EX_DOUBLEBUFFER, 
			LVS_EX_DOUBLEBUFFER);
		SetWindowTheme(*this, L"Explorer", NULL); // make the list view look a bit nicer

		// setup the callback interface
		IOwnerDataCallback* pCallback = NULL;
		QueryInterface(IID_IOwnerDataCallback, reinterpret_cast<LPVOID*>(&pCallback));
		if(IsWin7()) {
			IListView_Win7* pLvw = NULL;
			SendMessage(LVM_QUERYINTERFACE, reinterpret_cast<WPARAM>(&IID_IListView_Win7), reinterpret_cast<LPARAM>(&pLvw));
			if(pLvw) {
				pLvw->SetOwnerDataCallback(pCallback);
				pLvw->Release();
			}
		} else {
			IListView_WinVista* pLvw = NULL;
			SendMessage(LVM_QUERYINTERFACE, reinterpret_cast<WPARAM>(&IID_IListView_WinVista), reinterpret_cast<LPARAM>(&pLvw));
			if(pLvw) {
				pLvw->SetOwnerDataCallback(pCallback);
				pLvw->Release();
			}
		}

		ShowScrollBar(SB_VERT, true);

		//HIMAGELIST hImageList = NULL;
		//SHGetImageList(SHIL_EXTRALARGE, IID_IImageList, reinterpret_cast<LPVOID*>(&hImageList));
		//SetImageList(hImageList, LVSIL_SMALL);
		//SetItemCount(ITEMCOUNT);

		return lr;
	}

	LRESULT OnGetDispInfo(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
	{
		NMLVDISPINFO* pDetails = reinterpret_cast<NMLVDISPINFO*>(pnmh);
		if(pDetails->item.mask & LVIF_TEXT) {
			StringCchPrintf(pDetails->item.pszText, 
				pDetails->item.cchTextMax, _T("Item %i"), pDetails->item.iItem + 1);
		}
		//if(pDetails->item.mask & LVIF_IMAGE) {
		//	pDetails->item.iImage = pDetails->item.iItem % 3;
		//}
		return 0;
	}

	void InsertGroups(int cnt_udf, int cnt_enum, int cnt_typedef)
	{
		LVGROUP group = {0};
		group.cbSize = RunTimeHelper::SizeOf_LVGROUP();
		group.mask = LVGF_ALIGN | LVGF_GROUPID | LVGF_HEADER | LVGF_ITEMS | LVGF_STATE;
		group.iGroupId = 1;
		group.uAlign = LVGA_HEADER_LEFT;
		group.cItems = cnt_udf;			
		group.state = LVGS_COLLAPSIBLE;
		group.pszHeader = _T("User Defined Type");
		InsertGroup(0, &group);

		group.cItems = cnt_enum;			
		group.iGroupId = 2;
		group.pszHeader = _T("Enum");
		InsertGroup(1, &group);

		group.cItems = cnt_typedef;			
		group.iGroupId = 3;
		group.pszHeader = _T("Typedef");
		InsertGroup(2, &group);

		EnableGroupView(TRUE);
	}

};
