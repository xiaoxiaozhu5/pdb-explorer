#pragma once

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

		/*
		HTREEITEM hItem = GetFirstVisibleItem();
		while (hItem != NULL)
		{
			//do here what you want
			//CRect rItemRect;
			//GetItemRect(hItem, &rItemRect, FALSE);
			//if (dc.RectVisible(rItemRect))
			//{
			//	//do here what you want
			//}
			//else
			//{
			//	break; //from while
			//}

			hItem = GetNextVisibleItem(hItem);
		}
		*/

		//paintDc.BitBlt(0, 0, clientRect.right, clientRect.bottom, dc.m_hDC, 0, 0, SRCCOPY);
		mdc.BitBlt(0, 0, clientRect.right, clientRect.bottom, dc.m_hDC, 0, 0, SRCCOPY);

		dc.SelectBitmap(pOldBitmap);
		bitmap.DeleteObject();
		dc.DeleteDC();
		bHandled = FALSE;
		return 0;
    }

private:
};
