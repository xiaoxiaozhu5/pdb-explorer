#pragma once
#include "SymWrap.h"

BOOL _FindSymbol(IDiaSymbol* sym, LPVOID param);

class CDiaInfo
{
public:
	~CDiaInfo()
	{
		if(m_global) m_global->Release();
		if(m_session) m_session->Release();
		if(m_spDataSource) m_spDataSource->Release();
	}

	CString GetSymbolInfo(LPCTSTR path, const PDBSYMBOL& Symbol)
	{
		CString sRTF;
		HRESULT hr = CoInitialize(NULL);
		hr = CoCreateInstance(__uuidof(DiaSource),
			NULL,
			CLSCTX_INPROC_SERVER,
			__uuidof(IDiaDataSource),
			(void**)&m_spDataSource);
		if (FAILED(hr))
		{
			return sRTF;
		}

		wchar_t wszExt[MAX_PATH];
		_wsplitpath_s(path, NULL, 0, NULL, 0, NULL, 0, wszExt, MAX_PATH);
		if (!_wcsicmp(wszExt, L".pdb"))
		{
			hr = m_spDataSource->loadDataFromPdb(path);
			if (FAILED(hr))
			{
				return sRTF;
			}
		}

		hr = m_spDataSource->openSession(&m_session);
		if (FAILED(hr))
		{
			return sRTF;
		}
		IDiaSymbol* symbol = NULL;
		m_session->get_globalScope(&m_global);

		ULONG celt = 1;
		bool bFind = false;
		if(Symbol.dwSymTag != 0)
		{
			CComPtr<IDiaEnumSymbols> enum_symbols = NULL;
			m_global->findChildren((enum SymTagEnum)Symbol.dwSymTag, NULL, nsNone, &enum_symbols);
			while (SUCCEEDED(enum_symbols->Next(1, &symbol, &celt)) && (celt == 1))
			{
				CComBSTR bstrName;
				symbol->get_name(&bstrName);
				if (CString(bstrName.m_str) == Symbol.sKey)
				{
					bFind = true;
					break;
				}
				symbol->Release();
			}
			if (bFind)
			{
				CSym* sym = CSym::NewSym(symbol);
				sym->Format(&sRTF);
				CSym::Delete(sym);
				symbol->Release();
			}
		}
		else
		{
			enum SymTagEnum ste[] = { SymTagUDT, SymTagEnum, SymTagTypedef };
			for (int i = 0; i < _countof(ste); ++i)
			{
				CComPtr<IDiaEnumSymbols> enum_symbols = NULL;
				m_global->findChildren(ste[i], NULL, nsNone, &enum_symbols);
				while (SUCCEEDED(enum_symbols->Next(1, &symbol, &celt)) && (celt == 1))
				{
					CComBSTR bstrName;
					symbol->get_name(&bstrName);
					if (CString(bstrName.m_str) == Symbol.sKey)
					{
						bFind = true;
						break;
					}
					symbol->Release();
				}
				if (bFind)
				{
					CSym* sym = CSym::NewSym(symbol);
					sym->Format(&sRTF);
					CSym::Delete(sym);
					symbol->Release();
					break;
				}
			}
		}
		return sRTF;
	}

	CString GetSymbolInfo(IDiaSymbol* sym_global, const PDBSYMBOL& Symbol)
	{
		CString sRTF;
		auto find_symbol = CSym::Enum(sym_global, SymTagNull, _FindSymbol, (PVOID)Symbol.dwSymId);
		if (find_symbol)
		{
			CSym* sym = CSym::NewSym(find_symbol);
			sym->Format(&sRTF);
			CSym::Delete(sym);
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
	IDiaDataSource* m_spDataSource = NULL;
	IDiaSession* m_session = NULL;
	IDiaSymbol *m_global = NULL;
};

BOOL _FindSymbol(IDiaSymbol* sym, LPVOID param)
{
	DWORD id;
	sym->get_symIndexId(&id);
	return id == (DWORD)param;
}

