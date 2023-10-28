#include "stdafx.h"
#include "SymWrap.h"

namespace 
{
    CString ValidateName(TCHAR* szName)
    {
        CString tmp(szName);
        tmp.Replace(TEXT("&"), TEXT("&amp;"));
        tmp.Replace(TEXT("<"), TEXT("&lt;"));
        tmp.Replace(TEXT(">"), TEXT("&gt;"));
        tmp.Replace(TEXT("\""), TEXT("&quot;"));
        tmp.Replace(TEXT("\\~"), TEXT("&nbsp;"));
        return tmp;
    }
}

CSym::CSym(__in IDiaSymbol* sym)
{
    m_sym = sym;
    m_sym->AddRef();
}

CSym::~CSym(void)
{
    m_sym->Release();
}

void CSym::Declare(__out CString* str, __in PCWSTR lpName)
{
    CString strType;
    GetType(&strType);
    str->Format(L"%s %s", (PCWSTR)strType, lpName);
}

void CSym::Delete(__in CSym* sym)
{
    delete sym;
}

IDiaSymbol* CSym::Enum(
    __in IDiaSymbol* symParent,
    __in enum SymTagEnum enTag,
    __in EnumProc cbEnum,
    __in_opt PVOID param)
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
        } while (0 != count);
    }

    return NULL;
}
    
BOOL CSym::Format(__out CString* str)
{
    enum SymTagEnum tag;
    CComBSTR bsName;
    m_sym->get_name(&bsName);
    m_sym->get_symTag((PDWORD)&tag);
    return FALSE;
}

BOOL CSym::GetHeader(__out CString* str)
{
    return FALSE;
}

BOOL CSym::GetType(__out CString* str)
{
    enum SymTagEnum tag;
    CComBSTR bsName;
    m_sym->get_name(&bsName);
    m_sym->get_symTag((PDWORD)&tag);
    return FALSE;
}

CSym* CSym::NewSym(__in IDiaSymbol* sym)
{
    enum SymTagEnum tag = SymTagNull;
    sym->get_symTag((PDWORD)&tag);

    CSym* ret = NULL;
    switch (tag)
    {
    case SymTagFunction:
        ret = new CSymFunction(sym);
        break;
    case SymTagData:
        ret = new CSymData(sym);
        break;
    case SymTagUDT:
        ret = new CSymUDT(sym);
        break;
    case SymTagEnum:
        ret = new CSymEnum(sym);
        break;
    case SymTagFunctionType:
        ret = new CSymFunctionType(sym);
        break;
    case SymTagPointerType:
        ret = new CSymPointerType(sym);
        break;
    case SymTagArrayType:
        ret = new CSymArrayType(sym);
        break;
    case SymTagBaseType:
        ret = new CSymBaseType(sym);
        break;
    case SymTagTypedef:
        ret = new CSymTypedef(sym);
        break;
    case SymTagFunctionArgType:
        ret = new CSymFunctionArgType(sym);
        break;
    default:
        ret = new CSym(sym);
    }
    return ret;
}

void CSym::TypeDefine(__out CString* str, __in PCWSTR lpType)
{
    CString strType;
    GetType(&strType);
    str->Format(L"<font color=blue>typedef</font> %s %s;",
        (PCWSTR)strType, lpType);
}

BOOL CSymFunction::Format(__out CString* str)
{
    CComBSTR bsName;
    m_sym->get_name(&bsName);
    CString validate_name = ValidateName(bsName);

    CComPtr<IDiaSymbol> type;
    m_sym->get_type(&type);

    CString strDeclare;
    CString strName;
    PCWSTR call = CSymFunctionType::GetCallType(type);
    if (NULL == call || L'\0' == *call)
        strName.Format(L"%s", validate_name);
    else
        strName.Format(L"%s %s", call, validate_name);

    CSym* sym = CSym::NewSym(type);
    sym->Declare(&strDeclare, strName);
    CSym::Delete(sym);

    // Virtual
    BOOL bVirtual = FALSE;
    m_sym->get_virtual(&bVirtual);
    if (bVirtual)
    {
        str->Format(L"&nbsp;&nbsp;&nbsp;&nbsp;"         \
            L"<font color=blue>virtual</font> %s",   \
            (PCWSTR)strDeclare);
    }
    else
    {
        str->Format(L"&nbsp;&nbsp;&nbsp;&nbsp;%s", (PCWSTR)strDeclare);
    }

    // Pure
    BOOL bPure = FALSE;
    m_sym->get_pure(&bPure);
    if (bPure)
        *str += L" = 0";
    *str += L";<br />";
    return TRUE;
}

BOOL CSymData::Format(__out CString* str)
{
    CComPtr<IDiaSymbol> symType;
    HRESULT hr = m_sym->get_type(&symType);
    if (FAILED(hr))
        return FALSE;

    CComBSTR bsName;
    m_sym->get_name(&bsName);

    // declare
    CString strDeclare;
    CSym* type = CSym::NewSym(symType);
    type->Declare(&strDeclare, (PCWSTR)bsName);
    CSym::Delete(type);

    // size & offset
    ULONGLONG size = 0;
    LONG offset = 0;
    symType->get_length(&size);
    m_sym->get_offset(&offset);

    // "    <type> <var>; // +<offset>(<size>)"
    str->Format(L"&nbsp;&nbsp;&nbsp;&nbsp;%s; "  \
        L"<font color=green>// +0x%x(0x%x)</font><br />\r\n",
        (PCWSTR)strDeclare, offset, (ULONG)size);
    return TRUE;
}

BOOL CSymUDT::EnumBase(IDiaSymbol* sym, PVOID param)
{
    CString* str = (CString*)param;

    CComBSTR bsName;
    sym->get_name(&bsName);

    CV_access_e enAccess = CV_private;
    sym->get_access((PDWORD)&enAccess);
    PCWSTR access = NULL;
    switch (enAccess)
    {
    case CV_private:
        access = L"<font color=blue>private</font>";
        break;
    case CV_protected:
        access = L"<font color=blue>protected</font>";
        break;
    case CV_public:
        access = L"<font color=blue>public</font>";
        break;
    }

    CString strAccess;
    if (0 == str->GetLength())
        strAccess.Format(L" : %s %s", access, (PCWSTR)bsName);
    else
        strAccess.Format(L", %s %s", access, (PCWSTR)bsName);
    *str += strAccess;
    return FALSE;
}

BOOL CSymUDT::EnumMember(IDiaSymbol* sym, PVOID param)
{
    SymAccess* p = (SymAccess*)param;

    CString strMember;
    CSym* symMember = CSym::NewSym(sym);
    if (NULL != symMember)
    {
        symMember->Format(&strMember);
        CSym::Delete(symMember);

        CV_access_e access;
        sym->get_access((PDWORD)&access);
        switch (access)
        {
        case CV_private:
            p->strPrivate += strMember;
            break;
        case CV_protected:
            p->strProtected += strMember;
            break;
        case CV_public:
            p->strPublic += strMember;
            break;
        }
    }
    return FALSE;
}

BOOL CSymUDT::Format(__out CString* str)
{
    UdtKind enKind = (UdtKind)-1;
    HRESULT hr = m_sym->get_udtKind((PDWORD)&enKind);
    if (FAILED(hr))
        return FALSE;

    // Header
    GetHeader(str);

    SymAccess member;
    int lenPublic;
    int lenProtected;
    int lenPrivate;

    // function
    CSym::Enum(m_sym, SymTagFunction, EnumMember, &member);
    lenPublic = member.strPublic.GetLength();
    lenProtected = member.strProtected.GetLength();
    lenPrivate = member.strPrivate.GetLength();
    if (0 != lenPublic)
    {
        // struct 默认不带 public
        if (UdtClass == enKind || 0 != lenProtected || 0 != lenPrivate)
            *str += L"<font color=blue>public</font>:<br />\r\n";
        *str += member.strPublic;
    }
    if (0 != lenProtected)
    {
        *str += L"<font color=blue>protected</font>:<br />\r\n";
        *str += member.strProtected;
    }
    if (0 != lenPrivate)
    {
        *str += L"<font color=blue>private</font>:<br />\r\n";
        *str += member.strPrivate;
    }

    // data
    member.strPublic = L"";
    member.strProtected = L"";
    member.strPrivate = L"";
    CSym::Enum(m_sym, SymTagData, EnumMember, &member);
    lenPublic = member.strPublic.GetLength();
    lenProtected = member.strProtected.GetLength();
    lenPrivate = member.strPrivate.GetLength();
    if (0 != lenPublic)
    {
        // struct 默认不带 public
        if (UdtClass == enKind || 0 != lenProtected || 0 != lenPrivate)
            *str += L"<font color=blue>public</font>:<br />\r\n";
        *str += member.strPublic;
    }
    if (0 != lenProtected)
    {
        *str += L"<font color=blue>protected</font>:<br />\r\n";
        *str += member.strProtected;
    }
    if (0 != lenPrivate)
    {
        *str += L"<font color=blue>private</font>:<br />\r\n";
        *str += member.strPrivate;
    }

    // tail
    *str += L"};";
    return TRUE;
}

BOOL CSymUDT::GetHeader(__out CString* str)
{
    UdtKind enKind = (UdtKind)-1;
    HRESULT hr = m_sym->get_udtKind((PDWORD)&enKind);
    if (FAILED(hr))
        return FALSE;

    PCWSTR tmp = NULL;
    switch (enKind)
    {
    case UdtStruct:
        tmp = L"struct";
        break;
    case UdtClass:
        tmp = L"class";
        break;
    case UdtUnion:
        tmp = L"union";
        break;
    default:
        break;
    }

    CComBSTR bsName;
    ULONGLONG size;
    m_sym->get_name(&bsName);
    CString validate_name = ValidateName(bsName);
    m_sym->get_length(&size);

    // base
    CString strBase;
    CSym::Enum(m_sym, SymTagBaseClass, EnumBase, &strBase);

    // header
    str->Format(L"<font class=\"key\">%s</font> %s%s "
        L"<font color=green>// 0x%x</font><br />{<br />\r\n",
        tmp, validate_name, (PCWSTR)strBase, (DWORD)size);
    return TRUE;
}

BOOL CSymUDT::GetType(__out CString* str)
{
    UdtKind enKind = (UdtKind)-1;
    HRESULT hr = m_sym->get_udtKind((PDWORD)&enKind);
    if (FAILED(hr))
        return FALSE;

    PCWSTR kind = NULL;
    switch (enKind)
    {
    case UdtStruct:
        kind = L"struct";
        break;
    case UdtClass:
        kind = L"class";
        break;
    case UdtUnion:
        kind = L"union";
        break;
    default:
        break;
    }

    DWORD id;
    CComBSTR bsName;
    m_sym->get_symIndexId(&id);
    m_sym->get_name(&bsName);
    CString validate_name = ValidateName(bsName.m_str);
    str->Format(L"<font color=blue>%s</font> <a href=\"sym://%s\">%s</a>",
        kind, bsName, validate_name);
    return TRUE;
}

BOOL CSymEnum::Format(__out CString* str)
{
    // header
    GetHeader(str);

    // members
    CSym::Enum(m_sym, SymTagData, OnEnum, str);

    // tail
    *str += L"};";
    return TRUE;
}

BOOL CSymEnum::GetHeader(__out CString* str)
{
    CComBSTR bsName;
    m_sym->get_name(&bsName);
    str->Format(L"<font color=blue>enum</font> %s<br />{ <br />\r\n",
        (PCWSTR)bsName);
    return TRUE;
}

BOOL CSymEnum::GetType(__out CString* str)
{
    CComBSTR bsName;
    m_sym->get_name(&bsName);

    DWORD id;
    m_sym->get_symIndexId(&id);
    CString validate_name = ValidateName(bsName.m_str);
    str->Format(L"<font color=blue>enum</font> <a href=\"sym://%s\">%s</a>",
        bsName, validate_name);
    return TRUE;
}

BOOL CSymEnum::OnEnum(IDiaSymbol* sym, PVOID param)
{
    CString* str = (CString*)param;

    CString strMember;
    VARIANT v;
    CComBSTR bsName;
    sym->get_name(&bsName);
    sym->get_value(&v);
    strMember.Format(L"&nbsp;&nbsp;&nbsp;&nbsp;%s = 0x%x;<br />\r\n",
        (PCWSTR)bsName, v.intVal);
    *str += strMember;
    return FALSE;
}

void CSymFunctionType::Declare(__out CString* str, __in PCWSTR lpName)
{
    CString strRet;
    BOOL bCon = FALSE;
    m_sym->get_constructor(&bCon);
    if (!bCon && L'~' != *lpName)
    {
        // 非构造函数，需判断返回值
        CComPtr<IDiaSymbol> type;
        m_sym->get_type(&type);
        CSym* sym = CSym::NewSym(type);
        sym->GetType(&strRet);
        CSym::Delete(sym);
        strRet += L" ";
    }

    CString strArgs;
    CSym::Enum(m_sym, SymTagFunctionArgType, EnumArg, &strArgs);
    if (0 == strArgs.GetLength())
        strArgs = L"<font color=blue>void</font>";

    str->Format(L"%s%s(%s)", (PCWSTR)strRet, lpName, (PCWSTR)strArgs);
}

PCWSTR CSymFunctionType::GetCallType(IDiaSymbol* sym)
{
    CV_call_e enCall;
    sym->get_callingConvention((PDWORD)&enCall);

    PCWSTR ret = L"";
    switch (enCall)
    {
    case CV_CALL_NEAR_STD:
        ret = L"<font color=blue>__stdcall</font>";
        break;
    }
    return ret;
}

BOOL CSymFunctionType::EnumArg(IDiaSymbol* sym, PVOID param)
{
    CString* str = (CString*)param;

    CString strArg;
    CSym* symArg = CSym::NewSym(sym);
    symArg->Format(&strArg);
    CSym::Delete(symArg);

    if (0 != str->GetLength())
        *str += L", ";
    *str += strArg;
    return FALSE;
}

void CSymPointerType::Declare(__out CString* str, __in PCWSTR lpName)
{
    CComPtr<IDiaSymbol> type;
    HRESULT hr = m_sym->get_type(&type);
    if (FAILED(hr))
        return;

    enum SymTagEnum tag;
    type->get_symTag((PDWORD)&tag);
    if (SymTagFunctionType != tag)
        return CSym::Declare(str, lpName);

    PCWSTR call = CSymFunctionType::GetCallType(type);
    CString strName;
    if (L'\0' == *call)
        strName.Format(L"(*%s)", lpName);
    else
        strName.Format(L"(%s * %s)", call, lpName);
    CSym* sym = CSym::NewSym(type);
    sym->Declare(str, strName);
    CSym::Delete(sym);
}

BOOL CSymPointerType::GetType(__out CString* str)
{
    CComPtr<IDiaSymbol> pointee;
    HRESULT hr = m_sym->get_type(&pointee);
    if (FAILED(hr))
        return FALSE;

    CSym* type = CSym::NewSym(pointee);
    CString strType;
    type->GetType(&strType);
    CSym::Delete(type);

    BOOL bRef = FALSE;
    m_sym->get_reference(&bRef);
    if (bRef)
        strType += L"&amp;";
    else
        strType += L"*";
    *str = strType;
    return TRUE;
}

void CSymPointerType::TypeDefine(__out CString* str, __in PCWSTR lpType)
{
    CComPtr<IDiaSymbol> type;
    HRESULT hr = m_sym->get_type(&type);
    if (FAILED(hr))
        return;

    enum SymTagEnum tag;
    type->get_symTag((PDWORD)&tag);
    if (SymTagFunctionType != tag)
        return CSym::TypeDefine(str, lpType);

    PCWSTR call = CSymFunctionType::GetCallType(type);
    CString strName;
    if (L'\0' == *call)
        strName.Format(L"(*%s)", lpType);
    else
        strName.Format(L"(%s * %s)", call, lpType);

    CString strType;
    CSym* sym = CSym::NewSym(type);
    sym->Declare(&strType, strName);
    CSym::Delete(sym);

    str->Format(L"<font color=blue>typedef</font> %s;", (PCWSTR)strType);
}

void CSymArrayType::Declare(__out CString* str, __in PCWSTR lpName)
{
    CComPtr<IDiaSymbol> type;
    m_sym->get_type(&type);

    ULONGLONG arrsize;
    ULONGLONG elemsize;
    m_sym->get_length(&arrsize);
    type->get_length(&elemsize);

    CString strArray;
    strArray.Format(L"%s[0x%x]", lpName, (ULONG)(arrsize / elemsize));

    CSym* sym = CSym::NewSym(type);
    sym->Declare(str, strArray);
    CSym::Delete(sym);
}

BOOL CSymBaseType::GetType(__out CString* str)
{
    BasicType enType = btNoType;
    HRESULT hr = m_sym->get_baseType((LPDWORD)&enType);
    if (FAILED(hr))
        return FALSE;

    ULONGLONG len = 0;
    hr = m_sym->get_length(&len);
    PCWSTR type = L"BaseType";
    switch (enType)
    {
    case btVoid:
        type = L"<font color=blue>void</font>";
        break;
    case btChar:
        type = L"<font color=blue>char</font>";
        break;
    case btWChar:
        type = L"WCHAR";
        break;
    case btInt:
        {
            switch (len)
            {
            case 1:
                type = L"<font color=blue>char</font>";
                break;
            case 2:
                type = L"<font color=blue>short</font>";
                break;
            case 4:
                type = L"<font color=blue>int</font>";
                break;
            case 8:
                type = L"<font color=blue>__int64</font>";
                break;
            }
        }
        break;
    case btUInt:
        {
            switch (len)
            {
            case 1:
                type = L"BYTE";
                break;
            case 2:
                type = L"WORD";
                break;
            case 4:
                type = L"DWORD";
                break;
            case 8:
                type = L"ULONGLONG";
                break;
            }
        }
        break;
    case btFloat:
        {
            switch (len)
            {
            case 4:
                type = L"<font color=blue>float</font>";
                break;
            case 8:
                type = L"<font color=blue>double</font>";
                break;
            }
        }
        break;
    case btBCD:
        type = L"BCD";
        break;
    case btBool: 
        type = L"<font color=blue>bool</font>";
        break;
    case btLong: 
        type = L"<font color=blue>long</font>";
        break;
    case btULong: 
        type = L"ULONG";
        break;
    case btCurrency: 
        type = L"CURRENCY";
        break;
    case btDate: 
        type = L"DATE";
        break;
    case btVariant: 
        type = L"VARIANT";
        break;
    case btComplex:
        type = L"COMPLEX";
        break;
    case btBit:
        type = L"BIT";
        break;
    case btBSTR:
        type = L"BSTR";
        break;
    case btHresult:
        type = L"HRESULT";
        break;
    }

    *str = type;
    return TRUE;
}

BOOL CSymTypedef::Format(__out CString* str)
{
    CComBSTR bsName;
    m_sym->get_name(&bsName);

    CComPtr<IDiaSymbol> type;
    m_sym->get_type(&type);

    CString strType;
    CSym* sym = CSym::NewSym(type);
    sym->TypeDefine(str, bsName);
    CSym::Delete(sym);
    return TRUE;
}

BOOL CSymFunctionArgType::Format(__out CString* str)
{
    CComPtr<IDiaSymbol> type;
    m_sym->get_type(&type);

    CSym* sym = CSym::NewSym(type);
    sym->GetType(str);
    CSym::Delete(sym);
    return TRUE;
}

