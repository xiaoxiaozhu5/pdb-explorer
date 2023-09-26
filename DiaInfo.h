#pragma once
#include "dia_const.h"
#include "regs.h"

DWORD g_dwMachineType = CV_CFL_80386;

class CDiaInfo
{
public:
	CString GetSymbolInfo(IDiaDataSource* spDataSource, const PDBSYMBOL& Symbol)
	{
		//CString sRTF;
		_AddHeader(m_sRTF);
		_ATLTRY
		{
			HRESULT Hr = S_OK;

			IDiaSession* spSession = NULL;
			Hr = spDataSource->openSession(&spSession);
			if (FAILED(Hr))
			{
				return CString();
			}
			DWORD dwMachineType = CV_CFL_80386;
			IDiaSymbol* spSymbol = NULL;
			IDiaSymbol* spGlobal = NULL;
			IDiaSourceFile* spSourceFile = NULL;
			spSession->get_globalScope(&spGlobal);
			if (spGlobal->get_machineType(&dwMachineType) == S_OK)
			{
				switch (dwMachineType)
				{
				case IMAGE_FILE_MACHINE_I386: g_dwMachineType = CV_CFL_80386;
					break;
				case IMAGE_FILE_MACHINE_IA64: g_dwMachineType = CV_CFL_IA64;
					break;
				case IMAGE_FILE_MACHINE_AMD64: g_dwMachineType = CV_CFL_AMD64;
					break;
				}
			}
			spSession->symbolById(Symbol.dwSymId, &spSymbol);

			if (spSymbol == NULL)
			{
				spSession->findSymbolByAddr(Symbol.dwAddrSect, Symbol.dwAddrOffset, (enum SymTagEnum)Symbol.dwSymTag,
				                            &spSymbol);
			}

			if (spSymbol == NULL)
			{
				spSession->findFileById(Symbol.dwUniqueId, &spSourceFile);
			}

			if (spSymbol == NULL)
			{
				IDiaEnumSymbols* spChildren = NULL;
				spGlobal->findChildren(SymTagNull, CT2W(Symbol.sKey.Mid(Symbol.sKey.ReverseFind('|') + 1)), 0,
				                       &spChildren);
				ULONG celt = 0;
				if (spChildren != NULL)
				{
					spChildren->Next(1, &spSymbol, &celt);
				}
			}

			if (spSymbol == NULL)
			{
				return CString();
			}

			CComBSTR bstrTitle;
			if (bstrTitle.m_str == NULL) spSymbol->get_undecoratedNameEx(0x1000, &bstrTitle);
			if (bstrTitle.m_str == NULL) spSymbol->get_undecoratedName(&bstrTitle);
			if (bstrTitle.m_str == NULL) spSymbol->get_name(&bstrTitle);
			if (bstrTitle.m_str == NULL) spSymbol->get_libraryName(&bstrTitle);

			BOOL bIsFunction = FALSE;
			BOOL bIsConstructor = FALSE;
			spSymbol->get_function(&bIsFunction);
			spSymbol->get_constructor(&bIsConstructor);

			DWORD dwSymTag = 0;
			spSymbol->get_symTag(&dwSymTag);
			LPCTSTR pstrSymTags[] = {
				_T("NULL"), _T("Executable"), _T("Compiland"), _T("Compiland Details"), _T("Compiland Env"),
				_T("Function"), _T("Block"), _T("Data"), _T("Annotation"), _T("Label"), _T("Public Symbol"),
				_T("User Defined Type"), _T("Enumeration"), _T("Function Type"), _T("Pointer Type"), _T("Array Type"),
				_T("Base Type"), _T("Typedef"), _T("Class"), _T("Friend"), _T("Argument Type"), _T("Debug Start"),
				_T("Debug End"), _T("Using Namespace"), _T("VTable Shape"), _T("VTable"), _T("Custom"), _T("Thunk"),
				_T("Custom Type"), _T("Managed Type"), _T("Dimension")
			};
			CString sType = _T("Misc");
			if (dwSymTag < _countof(pstrSymTags)) sType = pstrSymTags[dwSymTag];
			if (bIsFunction) sType = _T("Function");
			if (bIsConstructor) sType = _T("Constructor / Destructor");
			_AddTitle(m_sRTF, bstrTitle.m_str, sType);

			CComBSTR bstrUndecorated;
			spSymbol->get_undecoratedNameEx(0, &bstrUndecorated);
			if (bstrUndecorated.Length() > 0)
			{
				m_sRTF += _T("\\fs22\\b Signature:\\par\\fs24\\b0 ");
				m_sRTF += bstrUndecorated;
				m_sRTF += _T(" \\par\\par ");
			}

			DWORD dwDataKind = 0;
			Hr = spSymbol->get_dataKind(&dwDataKind);
			LPCTSTR pstrDataKinds[] = {
				_T(""), _T("Local"), _T("Static Local"), _T("Parameter"), _T("Object Pointer"), _T("File Static"),
				_T("Global"), _T("Member"), _T("Static Member"), _T("Constant")
			};
			CString sDataKind;
			if (dwDataKind < _countof(pstrDataKinds)) sDataKind = pstrDataKinds[dwDataKind];
			if (Hr == S_OK) _AddTableEntry(m_sRTF, _T("Data Kind"), sDataKind);

			GUID guid = {0};
			spSymbol->get_guid(&guid);
			if (guid != GUID_NULL)
			{
				CComBSTR bstrGUID(guid);
				_AddTableEntry(m_sRTF, _T("GUID"), bstrGUID.m_str);
			}

			DWORD dwLanguage = 0;
			Hr = spSymbol->get_language(&dwLanguage);
			LPCTSTR pstrLanguages[] = {
				_T("C"), _T("C++"), _T("Fortran") _T("MASM"), _T("Pascal"), _T("Basic"), _T("Cobol"), _T("Link"),
				_T("Cvtres"), _T("Cvtpgd"), _T("C#"), _T("VB"), _T("ILASM"), _T("Java"), _T("JavaScript"), _T("MSIL")
			};
			CString sLanguage;
			if (dwLanguage < _countof(pstrLanguages)) sLanguage = pstrLanguages[dwLanguage];
			if (Hr == S_OK) _AddTableEntry(m_sRTF, _T("Language"), sLanguage);

			CComBSTR bstrSourceFile;
			spSymbol->get_sourceFileName(&bstrSourceFile);
			if (bstrSourceFile.m_str == NULL) spSymbol->get_symbolsFileName(&bstrSourceFile);
			_AddTableEntry(m_sRTF, _T("Source File"), bstrSourceFile.m_str);

			CComBSTR bstrCompiler;
			spSymbol->get_compilerName(&bstrCompiler);
			_AddTableEntry(m_sRTF, _T("Compiler"), bstrCompiler.m_str);

			CComVariant vValue;
			Hr = spSymbol->get_value(&vValue);
			if (Hr == S_OK)
			{
				if (SUCCEEDED(vValue.ChangeType(VT_BSTR))) _AddTableEntry(m_sRTF, _T("Value"), vValue.bstrVal);
			}

			ULONG nTypes = 0;
			IDiaSymbol* ppTypeSymbols[10] = {0};
			spSymbol->get_types(_countof(ppTypeSymbols), &nTypes, ppTypeSymbols);
			CString sTypes;
			for (ULONG i = 0; i < nTypes && ppTypeSymbols[i] != NULL; i++)
			{
				CComBSTR bstrName;
				ppTypeSymbols[i]->get_name(&bstrName);
				if (!sTypes.IsEmpty()) sTypes += _T(", ");
				sTypes += bstrName;
				ppTypeSymbols[i]->Release();
			}
			_AddTableEntry(m_sRTF, _T("Type"), sTypes);

			IDiaEnumSymbols* spChildren = NULL;
			spSymbol->findChildren(SymTagNull, NULL, 0, &spChildren);
			if (spChildren != NULL)
			{
				ULONG celt = 0;
				CString sChildren;
				IDiaSymbol* spChildSymbol = NULL;
				while (SUCCEEDED(spChildren->Next(1, &spChildSymbol, &celt)) && celt == 1)
				{
					DWORD dwSymTag = 0;
					spChildSymbol->get_symTag(&dwSymTag);
					if (dwSymTag != SymTagBaseClass && dwSymTag != SymTagBaseType) continue;
					CComBSTR bstrName;
					spChildSymbol->get_name(&bstrName);
					if (!sChildren.IsEmpty()) sChildren += _T(", ");
					sChildren += bstrName;
					spChildSymbol->Release();
				}
				_AddTableEntry(m_sRTF, _T("Lex"), sChildren);
				spChildren->Release();
			}

			if (SUCCEEDED(spSymbol->findChildren(SymTagNull, NULL, 0, &spChildren)))
			{
				ULONG celt = 0;
				IDiaSymbol* spChildSymbol = NULL;
				while (SUCCEEDED(spChildren->Next(1, &spChildSymbol, &celt)) && (celt == 1))
				{
					PrintTypeInDetail(spChildSymbol, 0);
					spChildSymbol->Release();
				}
				spChildren->Release();
			}

			IDiaSymbol* spBaseTypeSymbol = NULL;
			spSymbol->get_type(&spBaseTypeSymbol);
			if (spBaseTypeSymbol != NULL)
			{
				CComBSTR bstrName;
				spBaseTypeSymbol->get_name(&bstrName);
				_AddTableEntry(m_sRTF, _T("Base Type"), bstrName.m_str);
			}

			IDiaSymbol* spObjectTypeSymbol = NULL;
			spSymbol->get_objectPointerType(&spObjectTypeSymbol);
			if (spObjectTypeSymbol != NULL)
			{
				CComBSTR bstrName;
				spObjectTypeSymbol->get_name(&bstrName);
				_AddTableEntry(m_sRTF, _T("Object Type"), bstrName.m_str);
			}

			IDiaSymbol* spContainerSymbol = NULL;
			spSymbol->get_container(&spContainerSymbol);
			if (spContainerSymbol != NULL)
			{
				CComBSTR bstrName;
				spContainerSymbol->get_name(&bstrName);
				_AddTableEntry(m_sRTF, _T("Parent Type"), bstrName.m_str);
			}

			IDiaSymbol* spBaseClassSymbol = NULL;
			spSymbol->get_virtualBaseTableType(&spBaseClassSymbol);
			if (spBaseClassSymbol != NULL)
			{
				CComBSTR bstrName;
				spBaseClassSymbol->get_name(&bstrName);
				_AddTableEntry(m_sRTF, _T("Base Class"), bstrName.m_str);
			}

			IDiaSymbol* spBaseShapeSymbol = NULL;
			spSymbol->get_virtualTableShape(&spBaseShapeSymbol);
			if (spBaseShapeSymbol != NULL)
			{
				CComBSTR bstrName;
				spBaseShapeSymbol->get_name(&bstrName);
				_AddTableEntry(m_sRTF, _T("Base Class"), bstrName.m_str);
			}

			IDiaSymbol* spClassParentSymbol = NULL;
			spSymbol->get_classParent(&spClassParentSymbol);
			if (spClassParentSymbol != NULL)
			{
				CComBSTR bstrName;
				spClassParentSymbol->get_name(&bstrName);
				_AddTableEntry(m_sRTF, _T("Class Parent"), bstrName.m_str);
			}

			IDiaSymbol* spLexParentSymbol = NULL;
			spSymbol->get_lexicalParent(&spLexParentSymbol);
			if (spLexParentSymbol != NULL)
			{
				CComBSTR bstrName;
				spLexParentSymbol->get_name(&bstrName);
				_AddTableEntry(m_sRTF, _T("Owner"), bstrName.m_str);
			}
		}
		_ATLCATCHALL()
		{
		}
		_AddFooter(m_sRTF);
		return m_sRTF;
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

	void PrintUdtKind(IDiaSymbol* pSymbol)
	{
		DWORD dwKind = 0;

		if (pSymbol->get_udtKind(&dwKind) == S_OK)
		{
			_AddContent2(m_sRTF, L"%s ", rgUdtKind[dwKind]);
		}
	}

	void PrintName(IDiaSymbol* pSymbol)
	{
		BSTR bstrName;
		BSTR bstrUndName;

		if (pSymbol->get_name(&bstrName) != S_OK)
		{
			_AddContent(m_sRTF, L"(none)");
			return;
		}

		if (pSymbol->get_undecoratedName(&bstrUndName) == S_OK)
		{
			if (wcscmp(bstrName, bstrUndName) == 0)
			{
				_AddContent2(m_sRTF, L"%s", bstrName);
			}

			else
			{
				_AddContent2(m_sRTF, L"%s(%s)", bstrUndName, bstrName);
			}

			SysFreeString(bstrUndName);
		}

		else
		{
			_AddContent2(m_sRTF, L"%s", bstrName);
		}

		SysFreeString(bstrName);
	}

	void PrintVariant(VARIANT var)
	{
		switch (var.vt)
		{
		case VT_UI1:
		case VT_I1:
			_AddContent2(m_sRTF, L" 0x%X", var.bVal);
			break;

		case VT_I2:
		case VT_UI2:
		case VT_BOOL:
			_AddContent2(m_sRTF, L" 0x%X", var.iVal);
			break;

		case VT_I4:
		case VT_UI4:
		case VT_INT:
		case VT_UINT:
		case VT_ERROR:
			_AddContent2(m_sRTF, L" 0x%X", var.lVal);
			break;

		case VT_R4:
			_AddContent2(m_sRTF, L" %g", var.fltVal);
			break;

		case VT_R8:
			_AddContent2(m_sRTF, L" %g", var.dblVal);
			break;

		case VT_BSTR:
			_AddContent2(m_sRTF, L" \"%s\"", var.bstrVal);
			break;

		default:
			_AddContent(m_sRTF, L" ??");
		}
	}

	void PrintBound(IDiaSymbol* pSymbol)
	{
		DWORD dwTag = 0;
		DWORD dwKind;

		if (pSymbol->get_symTag(&dwTag) != S_OK)
		{
			return;
		}

		if (pSymbol->get_locationType(&dwKind) != S_OK)
		{
			return;
		}

		if (dwTag == SymTagData && dwKind == LocIsConstant)
		{
			VARIANT v;

			if (pSymbol->get_value(&v) == S_OK)
			{
				PrintVariant(v);
				VariantClear((VARIANTARG*)&v);
			}
		}

		else
		{
			PrintName(pSymbol);
		}
	}

	void PrintLocation(IDiaSymbol* pSymbol)
	{
		DWORD dwLocType;
		DWORD dwRVA, dwSect, dwOff, dwReg, dwBitPos, dwSlot;
		LONG lOffset;
		ULONGLONG ulLen;
		VARIANT vt = {VT_EMPTY};

		if (pSymbol->get_locationType(&dwLocType) != S_OK)
		{
			// It must be a symbol in optimized code

			_AddContent(m_sRTF, L"symbol in optmized code");
			return;
		}

		switch (dwLocType)
		{
		case LocIsStatic:
			if ((pSymbol->get_relativeVirtualAddress(&dwRVA) == S_OK) &&
				(pSymbol->get_addressSection(&dwSect) == S_OK) &&
				(pSymbol->get_addressOffset(&dwOff) == S_OK))
			{
				_AddContent2(m_sRTF, L"%s, [%08X][%04X:%08X]", SafeDRef(rgLocationTypeString, dwLocType), dwRVA, dwSect, dwOff);
			}
			break;

		case LocIsTLS:
		case LocInMetaData:
		case LocIsIlRel:
			if ((pSymbol->get_relativeVirtualAddress(&dwRVA) == S_OK) &&
				(pSymbol->get_addressSection(&dwSect) == S_OK) &&
				(pSymbol->get_addressOffset(&dwOff) == S_OK))
			{
				_AddContent2(m_sRTF, L"%s, [%08X][%04X:%08X]", SafeDRef(rgLocationTypeString, dwLocType), dwRVA, dwSect, dwOff);
			}
			break;

		case LocIsRegRel:
			if ((pSymbol->get_registerId(&dwReg) == S_OK) &&
				(pSymbol->get_offset(&lOffset) == S_OK))
			{
				_AddContent2(m_sRTF, L"%s Relative, [%08X]", SzNameC7Reg((USHORT)dwReg), lOffset);
			}
			break;

		case LocIsThisRel:
			if (pSymbol->get_offset(&lOffset) == S_OK)
			{
				_AddContent2(m_sRTF, L"this+0x%X", lOffset);
			}
			break;

		case LocIsBitField:
			if ((pSymbol->get_offset(&lOffset) == S_OK) &&
				(pSymbol->get_bitPosition(&dwBitPos) == S_OK) &&
				(pSymbol->get_length(&ulLen) == S_OK))
			{
				_AddContent2(m_sRTF, L"this(bf)+0x%X:0x%X len(0x%X)", lOffset, dwBitPos, (ULONG)ulLen);
			}
			break;

		case LocIsEnregistered:
			if (pSymbol->get_registerId(&dwReg) == S_OK)
			{
				_AddContent2(m_sRTF, L"enregistered %s", SzNameC7Reg((USHORT)dwReg));
			}
			break;

		case LocIsSlot:
			if (pSymbol->get_slot(&dwSlot) == S_OK)
			{
				_AddContent2(m_sRTF, L"%s, [%08X]", SafeDRef(rgLocationTypeString, dwLocType), dwSlot);
			}
			break;

		case LocIsConstant:
			_AddContent(m_sRTF, L"constant");

			if (pSymbol->get_value(&vt) == S_OK)
			{
				PrintVariant(vt);
				VariantClear((VARIANTARG*)&vt);
			}
			break;

		case LocIsNull:
			break;

		default:
			break;
		}
	}

	void PrintType(IDiaSymbol* pSymbol)
	{
		IDiaSymbol* pBaseType;
		IDiaEnumSymbols* pEnumSym;
		IDiaSymbol* pSym;
		DWORD dwTag;
		BSTR bstrName;
		DWORD dwInfo;
		BOOL bSet;
		DWORD dwRank;
		LONG lCount = 0;
		ULONG celt = 1;

		if (pSymbol->get_symTag(&dwTag) != S_OK)
		{
			return;
		}

		if (pSymbol->get_name(&bstrName) != S_OK)
		{
			bstrName = NULL;
		}

		if (dwTag != SymTagPointerType)
		{
			if ((pSymbol->get_constType(&bSet) == S_OK) && bSet)
			{
				_AddContent(m_sRTF, L"const ");
			}

			if ((pSymbol->get_volatileType(&bSet) == S_OK) && bSet)
			{
				_AddContent(m_sRTF, L"volatile ");
			}

			if ((pSymbol->get_unalignedType(&bSet) == S_OK) && bSet)
			{
				_AddContent(m_sRTF, L"__unaligned ");
			}
		}

		ULONGLONG ulLen;

		pSymbol->get_length(&ulLen);

		switch (dwTag)
		{
		case SymTagUDT:
			PrintUdtKind(pSymbol);
			PrintName(pSymbol);
			break;

		case SymTagEnum:
			_AddContent(m_sRTF, L"enum ");
			PrintName(pSymbol);
			break;

		case SymTagFunctionType:
			_AddContent(m_sRTF, L"function ");
			break;

		case SymTagPointerType:
			if (pSymbol->get_type(&pBaseType) != S_OK)
			{
				if (bstrName != NULL)
				{
					SysFreeString(bstrName);
				}
				return;
			}

			PrintType(pBaseType);
			pBaseType->Release();

			if ((pSymbol->get_reference(&bSet) == S_OK) && bSet)
			{
				_AddContent(m_sRTF, L" &");
			}

			else
			{
				_AddContent(m_sRTF, L" *");
			}

			if ((pSymbol->get_constType(&bSet) == S_OK) && bSet)
			{
				_AddContent(m_sRTF, L" const");
			}

			if ((pSymbol->get_volatileType(&bSet) == S_OK) && bSet)
			{
				_AddContent(m_sRTF, L" volatile");
			}

			if ((pSymbol->get_unalignedType(&bSet) == S_OK) && bSet)
			{
				_AddContent(m_sRTF, L" __unaligned");
			}
			break;

		case SymTagArrayType:
			if (pSymbol->get_type(&pBaseType) == S_OK)
			{
				PrintType(pBaseType);

				if (pSymbol->get_rank(&dwRank) == S_OK)
				{
					if (SUCCEEDED(pSymbol->findChildren(SymTagDimension, NULL, nsNone, &pEnumSym)) && (pEnumSym !=
						NULL))
					{
						while (SUCCEEDED(pEnumSym->Next(1, &pSym, &celt)) && (celt == 1))
						{
							IDiaSymbol* pBound;

							_AddContent(m_sRTF, L"[");

							if (pSym->get_lowerBound(&pBound) == S_OK)
							{
								PrintBound(pBound);

								_AddContent(m_sRTF, L"..");

								pBound->Release();
							}

							pBound = NULL;

							if (pSym->get_upperBound(&pBound) == S_OK)
							{
								PrintBound(pBound);

								pBound->Release();
							}

							pSym->Release();
							pSym = NULL;

							_AddContent(m_sRTF, L"]");
						}

						pEnumSym->Release();
					}
				}

				else if (SUCCEEDED(pSymbol->findChildren(SymTagCustomType, NULL, nsNone, &pEnumSym)) &&
					(pEnumSym != NULL) &&
					(pEnumSym->get_Count(&lCount) == S_OK) &&
					(lCount > 0))
				{
					while (SUCCEEDED(pEnumSym->Next(1, &pSym, &celt)) && (celt == 1))
					{
						_AddContent(m_sRTF, L"[");
						PrintType(pSym);
						_AddContent(m_sRTF, L"]");

						pSym->Release();
					}

					pEnumSym->Release();
				}

				else
				{
					DWORD dwCountElems;
					ULONGLONG ulLenArray;
					ULONGLONG ulLenElem;

					if (pSymbol->get_count(&dwCountElems) == S_OK)
					{
						_AddContent2(m_sRTF, L"[0x%X]", dwCountElems);
					}

					else if ((pSymbol->get_length(&ulLenArray) == S_OK) &&
						(pBaseType->get_length(&ulLenElem) == S_OK))
					{
						if (ulLenElem == 0)
						{
							_AddContent2(m_sRTF, L"[0x%lX]", (ULONG)ulLenArray);
						}

						else
						{
							_AddContent2(m_sRTF, L"[0x%lX]", (ULONG)ulLenArray / (ULONG)ulLenElem);
						}
					}
				}

				pBaseType->Release();
			}

			else
			{
				if (bstrName != NULL)
				{
					SysFreeString(bstrName);
				}
				return;
			}
			break;

		case SymTagBaseType:
			if (pSymbol->get_baseType(&dwInfo) != S_OK)
			{
				if (bstrName != NULL)
				{
					SysFreeString(bstrName);
				}
				return;
			}

			switch (dwInfo)
			{
			case btUInt:
				_AddContent(m_sRTF, L"unsigned ");

			// Fall through

			case btInt:
				switch (ulLen)
				{
				case 1:
					if (dwInfo == btInt)
					{
						_AddContent(m_sRTF,L"signed ");
					}

					_AddContent(m_sRTF,L"char");
					break;

				case 2:
					_AddContent(m_sRTF,L"short");
					break;

				case 4:
					_AddContent(m_sRTF,L"int");
					break;

				case 8:
					_AddContent(m_sRTF,L"__int64");
					break;
				}

				dwInfo = 0xFFFFFFFF;
				break;

			case btFloat:
				switch (ulLen)
				{
				case 4:
					_AddContent(m_sRTF,L"float");
					break;

				case 8:
					_AddContent(m_sRTF,L"double");
					break;
				}

				dwInfo = 0xFFFFFFFF;
				break;
			}

			if (dwInfo == 0xFFFFFFFF)
			{
				break;
			}

			_AddContent2(m_sRTF, L"%s", rgBaseType[dwInfo]);
			break;

		case SymTagTypedef:
			PrintName(pSymbol);
			break;

		case SymTagCustomType:
			{
				DWORD idOEM, idOEMSym;
				DWORD cbData = 0;
				DWORD count;

				if (pSymbol->get_oemId(&idOEM) == S_OK)
				{
					_AddContent2(m_sRTF, L"OEMId = %X, ", idOEM);
				}

				if (pSymbol->get_oemSymbolId(&idOEMSym) == S_OK)
				{
					_AddContent2(m_sRTF, L"SymbolId = %X, ", idOEMSym);
				}

				if (pSymbol->get_types(0, &count, NULL) == S_OK)
				{
					IDiaSymbol** rgpDiaSymbols = (IDiaSymbol**)_alloca(sizeof(IDiaSymbol*) * count);

					if (pSymbol->get_types(count, &count, rgpDiaSymbols) == S_OK)
					{
						for (ULONG i = 0; i < count; i++)
						{
							PrintType(rgpDiaSymbols[i]);
							rgpDiaSymbols[i]->Release();
						}
					}
				}

				// print custom data

				if ((pSymbol->get_dataBytes(cbData, &cbData, NULL) == S_OK) && (cbData != 0))
				{
					_AddContent(m_sRTF, L", Data: ");

					BYTE* pbData = new BYTE[cbData];

					pSymbol->get_dataBytes(cbData, &cbData, pbData);

					for (ULONG i = 0; i < cbData; i++)
					{
						_AddContent2(m_sRTF, L"0x%02X ", pbData[i]);
					}

					delete[] pbData;
				}
			}
			break;

		case SymTagData: // This really is member data, just print its location
			PrintLocation(pSymbol);
			break;
		}

		if (bstrName != NULL)
		{
			SysFreeString(bstrName);
		}
	}

	void PrintSymbolType(IDiaSymbol* pSymbol)
	{
		IDiaSymbol* pType;

		if (pSymbol->get_type(&pType) == S_OK)
		{
			_AddContent(m_sRTF, L", Type: ");
			PrintType(pType);
			pType->Release();
		}
	}

	void PrintUDT(IDiaSymbol* pSymbol)
	{
		PrintName(pSymbol);
		PrintSymbolType(pSymbol);
	}

	void PrintFunctionType(IDiaSymbol* pSymbol)
	{
		DWORD dwAccess = 0;

		if (pSymbol->get_access(&dwAccess) == S_OK)
		{
			_AddContent2(m_sRTF, L"%s ", SafeDRef(rgAccess, dwAccess));
		}

		BOOL bIsStatic = FALSE;

		if ((pSymbol->get_isStatic(&bIsStatic) == S_OK) && bIsStatic)
		{
			_AddContent(m_sRTF, L"static ");
		}

		IDiaSymbol* pFuncType;

		if (pSymbol->get_type(&pFuncType) == S_OK)
		{
			IDiaSymbol* pReturnType;

			if (pFuncType->get_type(&pReturnType) == S_OK)
			{
				PrintType(pReturnType);
				_AddContent(m_sRTF, L" ");

				BSTR bstrName;

				if (pSymbol->get_name(&bstrName) == S_OK)
				{
					_AddContent2(m_sRTF, L"%s", bstrName);

					SysFreeString(bstrName);
				}

				IDiaEnumSymbols* pEnumChildren;

				if (SUCCEEDED(pFuncType->findChildren(SymTagNull, NULL, nsNone, &pEnumChildren)))
				{
					IDiaSymbol* pChild;
					ULONG celt = 0;
					ULONG nParam = 0;

					_AddContent(m_sRTF, L"(");

					while (SUCCEEDED(pEnumChildren->Next(1, &pChild, &celt)) && (celt == 1))
					{
						IDiaSymbol* pType;

						if (pChild->get_type(&pType) == S_OK)
						{
							if (nParam++)
							{
								_AddContent(m_sRTF, L", ");
							}

							PrintType(pType);
							pType->Release();
						}

						pChild->Release();
					}

					pEnumChildren->Release();

					_AddContent(m_sRTF, L")\\par");
				}

				pReturnType->Release();
			}

			pFuncType->Release();
		}
	}

	void PrintThunk(IDiaSymbol* pSymbol)
	{
		DWORD dwRVA;
		DWORD dwISect;
		DWORD dwOffset;

		if ((pSymbol->get_relativeVirtualAddress(&dwRVA) == S_OK) &&
			(pSymbol->get_addressSection(&dwISect) == S_OK) &&
			(pSymbol->get_addressOffset(&dwOffset) == S_OK))
		{
			_AddContent2(m_sRTF, L"[%08X][%04X:%08X]", dwRVA, dwISect, dwOffset);
		}

		if ((pSymbol->get_targetSection(&dwISect) == S_OK) &&
			(pSymbol->get_targetOffset(&dwOffset) == S_OK) &&
			(pSymbol->get_targetRelativeVirtualAddress(&dwRVA) == S_OK))
		{
			_AddContent2(m_sRTF, L", target [%08X][%04X:%08X] ", dwRVA, dwISect, dwOffset);
		}

		else
		{
			_AddContent(m_sRTF, L", target ");

			PrintName(pSymbol);
		}
	}

	void PrintTypeInDetail(IDiaSymbol* pSymbol, DWORD dwIndent)
	{
		IDiaEnumSymbols* pEnumChildren;
		IDiaSymbol* pType;
		IDiaSymbol* pChild;
		DWORD dwSymTag;
		DWORD dwSymTagType;
		ULONG celt = 0;
		BOOL bFlag;

		if (pSymbol->get_symTag(&dwSymTag) != S_OK)
		{
			return;
		}

		for (DWORD i = 0; i < dwIndent; i++)
		{
			_AddContent(m_sRTF, L" ");
		}

		switch (dwSymTag)
		{
		case SymTagData:
			if (pSymbol->get_type(&pType) == S_OK)
			{
				if (pType->get_symTag(&dwSymTagType) == S_OK)
				{
					if (dwSymTagType == SymTagUDT)
					{
						_AddContent(m_sRTF, L"\\par");
						PrintTypeInDetail(pType, dwIndent + 2);
					}
				}
				pType->Release();
			}
			break;

		case SymTagTypedef:
		case SymTagVTable:
			PrintSymbolType(pSymbol);
			break;
		case SymTagEnum:
		case SymTagUDT:
			PrintUDT(pSymbol);
			_AddContent(m_sRTF, L"\\par");

			if (SUCCEEDED(pSymbol->findChildren(SymTagNull, NULL, nsNone, &pEnumChildren)))
			{
				while (SUCCEEDED(pEnumChildren->Next(1, &pChild, &celt)) && (celt == 1))
				{
					PrintTypeInDetail(pChild, dwIndent + 2);

					pChild->Release();
				}

				pEnumChildren->Release();
			}
			return;
			break;

		case SymTagFunction:
			PrintFunctionType(pSymbol);
			return;
			break;

		case SymTagPointerType:
			PrintName(pSymbol);
			_AddContent(m_sRTF, L" has type ");
			PrintType(pSymbol);
			break;

		case SymTagArrayType:
		case SymTagBaseType:
		case SymTagFunctionArgType:
		case SymTagUsingNamespace:
		case SymTagCustom:
		case SymTagFriend:
			PrintName(pSymbol);
			PrintSymbolType(pSymbol);
			break;

		case SymTagVTableShape:
		case SymTagBaseClass:
			PrintName(pSymbol);

			if ((pSymbol->get_virtualBaseClass(&bFlag) == S_OK) && bFlag)
			{
				IDiaSymbol* pVBTableType;
				LONG ptrOffset;
				DWORD dispIndex;

				if ((pSymbol->get_virtualBaseDispIndex(&dispIndex) == S_OK) &&
					(pSymbol->get_virtualBasePointerOffset(&ptrOffset) == S_OK))
				{
					_AddContent2(m_sRTF, L" virtual, offset = 0x%X, pointer offset = %ld, virtual base pointer type = ", dispIndex,
					        ptrOffset);

					if (pSymbol->get_virtualBaseTableType(&pVBTableType) == S_OK)
					{
						PrintType(pVBTableType);
						pVBTableType->Release();
					}

					else
					{
						_AddContent(m_sRTF, L"(unknown)");
					}
				}
			}

			else
			{
				LONG offset;

				if (pSymbol->get_offset(&offset) == S_OK)
				{
					_AddContent2(m_sRTF, L", offset = 0x%X", offset);
				}
			}

			_AddContent(m_sRTF, L"\\par");

			if (SUCCEEDED(pSymbol->findChildren(SymTagNull, NULL, nsNone, &pEnumChildren)))
			{
				while (SUCCEEDED(pEnumChildren->Next(1, &pChild, &celt)) && (celt == 1))
				{
					PrintTypeInDetail(pChild, dwIndent + 2);
					pChild->Release();
				}

				pEnumChildren->Release();
			}
			break;

		case SymTagFunctionType:
			if (pSymbol->get_type(&pType) == S_OK)
			{
				PrintType(pType);
			}
			break;

		case SymTagThunk:
			// Happens for functions which only have S_PROCREF
			PrintThunk(pSymbol);
			break;

		default:
			break;
		}
	}

private:
	CString m_sRTF;
};
