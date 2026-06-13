// Globals.h — 전역 상수, GUID, 모듈 참조계수 (§6)
#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <ole2.h>
#include <msctf.h>
#include <ctffunc.h>

// 모듈
extern HINSTANCE g_hInst;
extern LONG      g_cRefDll;

inline void DllAddRef()  { InterlockedIncrement(&g_cRefDll); }
inline void DllRelease() { InterlockedDecrement(&g_cRefDll); }

// 우리 GUID — §6 에 따라 새로 발급. SampleIME 의 것을 재사용하지 않는다.
extern const CLSID c_clsidKPS9256;          // COM 서버 CLSID
extern const GUID  c_guidProfile;           // 입력 프로파일 GUID
extern const GUID  c_guidDisplayAttribute;  // 조합 밑줄 표시속성 GUID
extern const GUID  c_guidPreservedHangul;   // 토글건: VK_HANGUL
extern const GUID  c_guidPreservedShiftSpc; // 토글건: Shift+Space

// langid 0x0412 (ko-KR). 정체성은 표시이름으로 준다.
#define KPS_LANGID  ((LANGID)MAKELANGID(LANG_KOREAN, SUBLANG_KOREAN))

extern const WCHAR c_szProfileName[];       // 《조선 국규 (KPS 9256)》
extern const WCHAR c_szDescription[];       // CLSID 친숙이름
