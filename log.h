#pragma once

#define OUTPUT_DEBUG_STRING

#ifdef OUTPUT_DEBUG_STRING

#define LOG_MAXBUF_SIZE 4096
#define MODULE_NAMEW _CRT_WIDE(_CRT_STRINGIZE(PROJECTNAME))
#define MODULE_NAMEA PROJECTNAME
#ifdef UNICODE
#define MODULE_NAME MODULE_NAMEW
#else
#define MODULE_NAME MODULE_NAMEA
#endif

#define LogOutW(fmt, ...) do {																								\
																															\
		const WCHAR *pFileStr = nullptr;																					\
		WCHAR szLogBuff[LOG_MAXBUF_SIZE] = { 0 };																			\
		pFileStr = wcsrchr(__FILEW__, L'\\');																				\
		pFileStr = (pFileStr == NULL) ? __FILEW__ : pFileStr + 1;															\
		swprintf_s(szLogBuff, LOG_MAXBUF_SIZE -1, L"%s[%s:%d] " fmt L"\n", MODULE_NAMEW, pFileStr, __LINE__, ##__VA_ARGS__);	\
		OutputDebugStringW(szLogBuff);																						\
																															\
	} while (FALSE)


#define LogOutA(fmt, ...) do {																							\
																														\
	const CHAR *pFileStr = nullptr;																						\
    char szLogBuff[LOG_MAXBUF_SIZE] = { 0 };																			\
    pFileStr = strrchr(__FILE__, '\\');																					\
	pFileStr = (pFileStr == NULL) ? __FILE__ : pFileStr + 1;															\
    sprintf_s(szLogBuff, LOG_MAXBUF_SIZE -1, "%s[%s:%d] " fmt "\n", MODULE_NAMEA, pFileStr, __LINE__, ##__VA_ARGS__);	\
	OutputDebugStringA(szLogBuff);																						\
																														\
	} while (FALSE)

#define LogOut(fmt, ...) do {																							\
																														\
	const TCHAR *pFileStr = nullptr;																					\
    TCHAR szLogBuff[LOG_MAXBUF_SIZE] = { 0 };																			\
    pFileStr = _tcsrchr(_T(__FILE__), '\\');																					\
	pFileStr = (pFileStr == NULL) ? _T(__FILE__) : pFileStr + 1;															\
    _stprintf_s(szLogBuff, LOG_MAXBUF_SIZE -1, _T("%s[%s:%d] ") fmt _T("\n"), MODULE_NAME, pFileStr, __LINE__, ##__VA_ARGS__);	\
	OutputDebugString(szLogBuff);																						\
																														\
	} while (FALSE)


#else

#define LogOutA(fmt, ...) do {} while (0)
#define LogOutW(fmt, ...) do {} while (0)
#define LogOut(fmt, ...) do {} while (0)

#endif 
