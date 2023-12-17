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
	//public CCustomDraw<CGroupedVirtualModeView>, // для перехват?WM_NOTIFY, NM_CUSTOMDRAW
	public IOwnerDataCallback
{

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
		// Запрашивае?уведомления NM_CUSTOMDRAW для каждог?элемента списка.
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
		if(groupIndex == 0)
		{
			*pTotalItemIndex = groupIndex + groupWideItemIndex;
		}
		else if(groupIndex == 1)
		{
			*pTotalItemIndex = udf_cnt_ + groupWideItemIndex;
		}
		else if(groupIndex == 2)
		{
			*pTotalItemIndex = udf_cnt_ + enum_cnt_ + groupWideItemIndex;
		}
		return S_OK;
	}

	virtual STDMETHODIMP GetItemGroup(int itemIndex, int occurenceIndex, PINT pGroupIndex)
	{
		if(itemIndex < udf_cnt_)
		{
			*pGroupIndex = 0;
		}
		else if(itemIndex < enum_cnt_)
		{
			*pGroupIndex = 1;
		}
		else if(itemIndex < typedef_cnt_)
		{
			*pGroupIndex = 2;
		}
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
		udf_cnt_ = cnt_udf;
		enum_cnt_ = cnt_enum;
		typedef_cnt_ = cnt_typedef;

	}

private:
	int udf_cnt_ = 0;
	int enum_cnt_ = 0;
	int typedef_cnt_ = 0;
};
