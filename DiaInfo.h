#pragma once
#include "SymWrap.h"

DWORD g_dwMachineType = CV_CFL_80386;

BOOL _FindSymbol(IDiaSymbol* sym, LPVOID param);

struct SymAccess
{
	CString strPublic;
	CString strProtected;
	CString strPrivate;
};

class CDiaInfo
{
public:
	CString GetSymbolInfo(IDiaDataSource* spDataSource, const PDBSYMBOL& Symbol)
	{
		CString sRTF;
		_ATLTRY
		{
			HRESULT Hr = S_OK;

			IDiaSession* spSession = NULL;
			Hr = spDataSource->openSession(&spSession);
			if (FAILED(Hr))
			{
				return CString();
			}
			IDiaSymbol* spGlobal = NULL;
			spSession->get_globalScope(&spGlobal);
			auto find_symbol = enum_symbols(spGlobal, SymTagNull, _FindSymbol, (PVOID)Symbol.dwSymId);
			if (find_symbol)
			{
				CSym* sym = CSym::NewSym(find_symbol);
				sym->Format(&sRTF);
				CSym::Delete(sym);
			}
			spGlobal->Release();
		}
		_ATLCATCHALL()
		{
		}
		return sRTF;
	}

	CString GetEmptyInfo(CTreeViewCtrl& ctrlTree) const
	{
		CString sRTF, sTitle;
		ctrlTree.GetItemText(ctrlTree.GetSelectedItem(), sTitle);
		_AddHeader(sRTF);
		_AddTitle(sRTF, sTitle, _T("Group"));
		_AddFooter(sRTF);
		return sRTF;
	}

	// Implementation

	void _AddHeader(CString& sRTF) const
	{
		sRTF += _T(
			"{\\rtf1\\ansi\\ansicpg1252\\deff0\\deflang1030{\\fonttbl{\\f0\\fnil\\fcharset0 Arial;}} {\\viewkind4\\uc1\\pard\\sa200\\sl236\\slmult1\\li100 ");
	}

	void _AddTitle(CString& sRTF, CString sTitle, CString sGroup) const
	{
		sRTF += _T(" \\par\\f0\\fs52 ");
		sRTF += sTitle;
		sRTF += _T(" \\par\\sl1000\\fs30 ");
		sRTF += sGroup;
		sRTF += _T(" \\par\\par ");
	}

	void _AddFooter(CString& sRTF) const
	{
		sRTF += _T("\\par\\par}");
	}

	void _AddTableEntry(CString& sRTF, CString sLabel, CString sValue)
	{
		if (sValue.IsEmpty()) return;
		sValue.Replace(_T("\\"), _T("\\\\"));
		sValue.Replace(_T("{"), _T("\\{ "));
		CString sTemp;
		sTemp.Format(_T("\\fs20\\b %s: \\b0 %s \\par "), sLabel, sValue);
		sRTF += sTemp;
	}

	void _AddContent(CString& sRTF, CString sContent)
	{
		sRTF += _T(" \\fs20 ");
		sRTF += sContent;
	}

	void _AddContent2(CString& sRtf, const wchar_t* format, ...)
	{
		const int tmp_len = MAX_PATH;
		WCHAR tmp[tmp_len] = {0};
		va_list args;
		va_start(args, format);
		_vsnwprintf_s(tmp, tmp_len, tmp_len, format, args);
		_AddContent(sRtf, tmp);
		va_end(args);
	}

private:

	IDiaSymbol* enum_symbols(IDiaSymbol* symParent, enum SymTagEnum enTag, EnumProc cbEnum, PVOID param)
	{
		if (NULL == symParent)
			return NULL;

		CComPtr<IDiaEnumSymbols> pEnum = NULL;
		HRESULT hr = symParent->findChildren(enTag, NULL, nsNone, &pEnum);
		if (SUCCEEDED(hr) && pEnum)
		{
			ULONG count = 1;
			IDiaSymbol* curItem = NULL;
			pEnum->Next(1, &curItem, &count);
			do
			{
				if (NULL != curItem)
				{
					if (cbEnum(curItem, param))
						return curItem;

					curItem->Release();
				}

				pEnum->Next(1, &curItem, &count);
			}
			while (0 != count);
		}

		return NULL;
	}


	CString m_sRTF;
};

BOOL _FindSymbol(IDiaSymbol* sym, LPVOID param)
{
	DWORD id;
	sym->get_symIndexId(&id);
	return id == (DWORD)param;
}

