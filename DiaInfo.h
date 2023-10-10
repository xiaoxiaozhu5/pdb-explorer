#pragma once
#include "SymWrap.h"

BOOL _FindSymbol(IDiaSymbol* sym, LPVOID param);

class CDiaInfo
{
public:
	CString GetSymbolInfo(IDiaSymbol* sym_global, const PDBSYMBOL& Symbol)
	{
		CString sRTF;
		_ATLTRY
		{
			auto find_symbol = CSym::Enum(sym_global, SymTagNull, _FindSymbol, (PVOID)Symbol.dwSymId);
			if (find_symbol)
			{
				CSym* sym = CSym::NewSym(find_symbol);
				sym->Format(&sRTF);
				CSym::Delete(sym);
			}
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
		sRTF = sTitle;
		return sRTF;
	}

private:
	CString m_sRTF;
};

BOOL _FindSymbol(IDiaSymbol* sym, LPVOID param)
{
	DWORD id;
	sym->get_symIndexId(&id);
	return id == (DWORD)param;
}

