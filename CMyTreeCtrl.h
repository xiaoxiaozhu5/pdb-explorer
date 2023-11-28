#pragma once
#include <gdiplus.h>

#pragma comment (lib,"Gdiplus.lib")

class CMyTreeCtrl : public CWindowImpl<CMyTreeCtrl, CTreeViewCtrl>
{
	typedef CWindowImpl<CMyTreeCtrl, CTreeViewCtrl> BaseTreeCtrl;

public:
    DECLARE_WND_CLASS(NULL)

    BEGIN_MSG_MAP(CMyTreeCtrl)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
    END_MSG_MAP()

    LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
    {
		CPaintDC paintDc(m_hWnd);
		CMemoryDC mdc(paintDc, paintDc.m_ps.rcPaint);

		CDC dc;
		//dc.CreateCompatibleDC(paintDc.m_hDC);
		dc.CreateCompatibleDC(mdc.m_hDC);

		CRect clientRect;
		GetClientRect(&clientRect);

		CBitmap bitmap;
		//bitmap.CreateCompatibleBitmap(paintDc.m_hDC, clientRect.right, clientRect.bottom);
		bitmap.CreateCompatibleBitmap(mdc.m_hDC, clientRect.right, clientRect.bottom);

		auto pOldBitmap = dc.SelectBitmap(bitmap);

		DefWindowProc(WM_PAINT, (WPARAM)dc.m_hDC, 0);

		dc.SetBkMode(TRANSPARENT);

#if 0
		CRect itemRect;
		int   itemState;
		HTREEITEM hItem = GetFirstVisibleItem();
		while (hItem != NULL)
		{
			if (GetItemRect(hItem,itemRect,TRUE))
			{
				itemRect.left = itemRect.left - 19;
				CRect fillRect(0, itemRect.top, clientRect.right, itemRect.bottom);
				itemState = GetItemState(hItem, TVIF_STATE);
				if (itemRect.top > clientRect.bottom)
				{
					break;
				}

				//if (itemState & TVIS_SELECTED)
				//{
				//	Gdiplus::Graphics graphics(mdc);
				//	Gdiplus::SolidBrush brush(Gdiplus::Color(0xFF00BB));
				//	graphics.FillRectangle(&brush, fillRect.left, fillRect.top, fillRect.Width(), fillRect.Height());
				//}

				if (ItemHasChildren(hItem))
				{
					CPoint point;
					point.x = itemRect.left;
					point.y = itemRect.top + itemRect.Height() / 2;

					//if (itemState & TVIS_EXPANDED)
					//{
					//    image_list_.Draw(mdc, 1, point, ILD_TRANSPARENT);
					//}
					//else
					//{
					//    image_list_.Draw(mdc, 0, point, ILD_TRANSPARENT);
					//}
				}
				CString text;
				if (GetItemText(hItem, text))
				{
					mdc.DrawText(text, text.GetLength(), itemRect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
				}
			}

			hItem = GetNextVisibleItem(hItem);
		}
#endif

		//paintDc.BitBlt(0, 0, clientRect.right, clientRect.bottom, dc.m_hDC, 0, 0, SRCCOPY);
		mdc.BitBlt(0, 0, clientRect.right, clientRect.bottom, dc.m_hDC, 0, 0, SRCCOPY);

		dc.SelectBitmap(pOldBitmap);
		bitmap.DeleteObject();
		dc.DeleteDC();
		bHandled = FALSE;
		return 0;
    }

private:
	CImageList image_list_;
};
