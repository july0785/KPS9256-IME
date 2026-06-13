// Globals.cpp — GUID/상수 정의
#include "Globals.h"

HINSTANCE g_hInst   = nullptr;
LONG      g_cRefDll = 0;

// {C9666EAC-0997-4EE8-8AF8-6CA7C71A81E7}
const CLSID c_clsidKPS9256 =
    { 0xC9666EAC, 0x0997, 0x4EE8, { 0x8A, 0xF8, 0x6C, 0xA7, 0xC7, 0x1A, 0x81, 0xE7 } };

// {85F522FA-155C-447F-8F53-571CBA7E754F}
const GUID c_guidProfile =
    { 0x85F522FA, 0x155C, 0x447F, { 0x8F, 0x53, 0x57, 0x1C, 0xBA, 0x7E, 0x75, 0x4F } };

// {C69E5D9C-7A08-4419-BE4C-AD4A4D667AC5}
const GUID c_guidDisplayAttribute =
    { 0xC69E5D9C, 0x7A08, 0x4419, { 0xBE, 0x4C, 0xAD, 0x4A, 0x4D, 0x66, 0x7A, 0xC5 } };

// {CFD232A4-5AE0-4B6F-B866-5627B01D0FDD}
const GUID c_guidPreservedHangul =
    { 0xCFD232A4, 0x5AE0, 0x4B6F, { 0xB8, 0x66, 0x56, 0x27, 0xB0, 0x1D, 0x0F, 0xDD } };

// {939D2526-D16F-4BFA-ACC7-F79FB320D506}
const GUID c_guidPreservedShiftSpc =
    { 0x939D2526, 0xD16F, 0x4BFA, { 0xAC, 0xC7, 0xF7, 0x9F, 0xB3, 0x20, 0xD5, 0x06 } };

// 《조선 국규 (KPS 9256)》  — 코드점으로 박아 코드페이지 영향 배제
//   조 U+C870  선 U+C120  국 U+AD6D  규 U+ADDC
const WCHAR c_szProfileName[] = L"조선 표준건반 배렬 (국규 9256)";
const WCHAR c_szDescription[] = L"조선 표준건반 배렬 (국규 9256) IME";
