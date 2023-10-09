#pragma once
#include <functional>
#include <unordered_set>

class CPdbCollector;
typedef BOOL (*EnumProc)(IDiaSymbol* curSym, PVOID param);


typedef struct tagPDBSYMBOL
{
	CString sKey;
	DWORD dwSymId;
	DWORD dwAddrSect;
	DWORD dwAddrOffset;
	DWORD dwSymTag;
	DWORD dwUniqueId;
} PDBSYMBOL;

BOOL _ProcessUDT(IDiaSymbol* sym, LPVOID param);
BOOL _ProcessEnum(IDiaSymbol* sym, LPVOID param);
BOOL _ProcessTypedef(IDiaSymbol* sym, LPVOID param);

class CPdbCollector : public CThreadImpl<CPdbCollector>
{
	friend BOOL _ProcessUDT(IDiaSymbol* sym, LPVOID param);
	friend BOOL _ProcessEnum(IDiaSymbol* sym, LPVOID param);
	friend BOOL _ProcessTypedef(IDiaSymbol* sym, LPVOID param);
public:
	HWND m_hWnd;
	CString m_sFilename;
	CString m_sLastError;
	CComAutoCriticalSection m_lock;
	IDiaDataSource* m_spDataSource = NULL;
	CAtlArray<PDBSYMBOL> m_aSymbols;
	volatile bool m_bDone;
	std::unordered_set<std::wstring> m_dupSet;

		CPdbCollector() : m_hWnd(NULL), m_bDone(false)
	{
	}

	void Init(HWND hWnd, LPCTSTR pstrFilename)
	{
		m_aSymbols.RemoveAll();
		m_sFilename = pstrFilename;
		m_sLastError.Empty();
		m_hWnd = hWnd;
		m_bDone = false;
	}

	DWORD Run()
	{
		::PostMessage(m_hWnd, WM_USER_LOAD_START, 0, 0L);
		m_bDone = false;
		_ATLTRY
		{
			if (m_spDataSource) m_spDataSource->Release();

			HRESULT hr = CoInitialize(NULL);
			hr = CoCreateInstance(__uuidof(DiaSource),
			                      NULL,
			                      CLSCTX_INPROC_SERVER,
			                      __uuidof(IDiaDataSource),
			                      (void**)&m_spDataSource);
			if (FAILED(hr))
			{
				return 1;
			}

			wchar_t wszExt[MAX_PATH];
			_wsplitpath_s(m_sFilename, NULL, 0, NULL, 0, NULL, 0, wszExt, MAX_PATH);
			if (!_wcsicmp(wszExt, L".pdb"))
			{
				hr = m_spDataSource->loadDataFromPdb(m_sFilename);
				if (FAILED(hr))
				{
					return 2;
				}
			}

			IDiaSession* spSession = NULL;
			hr = m_spDataSource->openSession(&spSession);
			if (FAILED(hr))
			{
				return 3;
			}
			if (IsAborted()) _Abort();
			_ProcessTables(spSession);
		}
		_ATLCATCHALL()
		{
			::PostMessage(m_hWnd, WM_USER_LOAD_END, 0, 0L);
		}
		m_spDataSource->Release();
		m_bDone = true;
		::CoUninitialize();
		return 0;
	}

	void _ProcessTables(IDiaSession* spSession) throw(...)
	{
		IDiaSymbol* global_symbol = NULL;
		if (FAILED(spSession->get_globalScope(&global_symbol)))
		{
			return;
		}
		auto enum_symbols = [&](IDiaSymbol* symParent, enum SymTagEnum enTag, EnumProc cbEnum,
		                        PVOID param) -> IDiaSymbol*
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
				}while (0 != count);
			}

			return NULL;
		};
		enum_symbols(global_symbol, SymTagUDT, _ProcessUDT, this);
		enum_symbols(global_symbol, SymTagEnum, _ProcessEnum, this);
		enum_symbols(global_symbol, SymTagTypedef, _ProcessTypedef, this);
		global_symbol->Release();
	}

	void _Fail()
	{
		CComBSTR bstrError;
		m_spDataSource->get_lastError(&bstrError);
		m_sLastError = bstrError;
		m_spDataSource->Release();
	}
};

BOOL _ProcessUDT(IDiaSymbol* sym, LPVOID param)
{
	auto This = (CPdbCollector*)param;
	UdtKind enKind;
	sym->get_udtKind((LPDWORD)&enKind);

	CComBSTR bstrName;
	sym->get_name(&bstrName);
	if(0 != lstrcmpW(bstrName, L"__unnamed") && This->m_dupSet.count(bstrName.m_str) == 0)
	{
		DWORD id;
		sym->get_symIndexId(&id);

		CString sKey;
		sKey.Format(_T("Source Files|User Defined Type|%ls"), bstrName);
		CComCritSecLock<CComCriticalSection> lock(This->m_lock);
		PDBSYMBOL Info = { sKey, id, 0, 0, SymTagUDT, 0 };
		This->m_aSymbols.Add(Info);
		This->m_dupSet.insert(bstrName.m_str);
		if (This->IsAborted()) throw 1;
	}

	return FALSE;
}


BOOL _ProcessEnum(IDiaSymbol* sym, LPVOID param)
{
	auto This = (CPdbCollector*)param;

	CComBSTR bstrName;
	sym->get_name(&bstrName);
	if(0 != lstrcmpW(bstrName, L"__unnamed") && This->m_dupSet.count(bstrName.m_str) == 0)
	{
		DWORD id;
		sym->get_symIndexId(&id);

		CString sKey;
		sKey.Format(_T("Source Files|Enum|%ls"), bstrName);
		CComCritSecLock<CComCriticalSection> lock(This->m_lock);
		PDBSYMBOL Info = { sKey, id, 0, 0, SymTagEnum, 0 };
		This->m_aSymbols.Add(Info);
		This->m_dupSet.insert(bstrName.m_str);
		if (This->IsAborted()) throw 1;
	}

	return FALSE;
}


BOOL _ProcessTypedef(IDiaSymbol* sym, LPVOID param)
{
	auto This = (CPdbCollector*)param;

	CComBSTR bstrName;
	sym->get_name(&bstrName);
	if(0 != lstrcmpW(bstrName, L"__unnamed") && This->m_dupSet.count(bstrName.m_str) == 0)
	{
		DWORD id;
		sym->get_symIndexId(&id);

		CString sKey;
		sKey.Format(_T("Source Files|Typedef|%ls"), bstrName);
		CComCritSecLock<CComCriticalSection> lock(This->m_lock);
		PDBSYMBOL Info = { sKey, id, 0, 0, SymTagTypedef, 0 };
		This->m_aSymbols.Add(Info);
		This->m_dupSet.insert(bstrName.m_str);
		if (This->IsAborted()) throw 1;
	}

	return FALSE;
}
