// Register.cpp — DllRegisterServer / DllUnregisterServer (§6)
//   1) COM 서버(HKCR\CLSID\{clsid})  2) 입력 프로파일  3) 카테고리(능력)
#include "KPS9256.h"
#include <strsafe.h>

STDAPI DllUnregisterServer(void);   // 아래에서 정의. DllRegisterServer 가 실패 정리에 쓴다.

// ─── 작은 도움이 ─────────────────────────────────────────────────────────────
static BOOL CLSIDToString(REFGUID guid, WCHAR* out, size_t cch)
{
    OLECHAR* psz = nullptr;
    if (FAILED(StringFromCLSID(guid, &psz)) || !psz) return FALSE;
    HRESULT hr = StringCchCopyW(out, cch, psz);
    CoTaskMemFree(psz);
    return SUCCEEDED(hr);
}

static BOOL SetRegValue(HKEY hKey, const WCHAR* name, const WCHAR* value)
{
    DWORD cb = (DWORD)((wcslen(value) + 1) * sizeof(WCHAR));
    return RegSetValueExW(hKey, name, 0, REG_SZ, (const BYTE*)value, cb) == ERROR_SUCCESS;
}

// COM 서버 등록: HKCR\CLSID\{clsid}\InProcServer32
static BOOL RegisterComServer()
{
    WCHAR szClsid[64];
    if (!CLSIDToString(c_clsidKPS9256, szClsid, ARRAYSIZE(szClsid))) return FALSE;

    WCHAR szModule[MAX_PATH];
    if (GetModuleFileNameW(g_hInst, szModule, ARRAYSIZE(szModule)) == 0) return FALSE;

    WCHAR szKey[128];
    StringCchPrintfW(szKey, ARRAYSIZE(szKey), L"CLSID\\%s", szClsid);

    HKEY hKey = nullptr, hSub = nullptr;
    BOOL ok = FALSE;
    if (RegCreateKeyExW(HKEY_CLASSES_ROOT, szKey, 0, nullptr, REG_OPTION_NON_VOLATILE,
                        KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
        SetRegValue(hKey, nullptr, c_szDescription);
        if (RegCreateKeyExW(hKey, L"InProcServer32", 0, nullptr, REG_OPTION_NON_VOLATILE,
                            KEY_WRITE, nullptr, &hSub, nullptr) == ERROR_SUCCESS) {
            SetRegValue(hSub, nullptr, szModule);
            SetRegValue(hSub, L"ThreadingModel", L"Apartment");
            RegCloseKey(hSub);
            ok = TRUE;
        }
        RegCloseKey(hKey);
    }
    return ok;
}

static void UnregisterComServer()
{
    WCHAR szClsid[64];
    if (!CLSIDToString(c_clsidKPS9256, szClsid, ARRAYSIZE(szClsid))) return;
    WCHAR szKey[128];
    StringCchPrintfW(szKey, ARRAYSIZE(szKey), L"CLSID\\%s", szClsid);
    RegDeleteTreeW(HKEY_CLASSES_ROOT, szKey);
}

// 입력 프로파일 등록 (langid 0x0412, 표시이름 《조선 국규 (KPS 9256)》)
static BOOL RegisterProfile()
{
    ITfInputProcessorProfiles* pProfiles = nullptr;
    if (FAILED(CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                                IID_ITfInputProcessorProfiles, (void**)&pProfiles)) || !pProfiles)
        return FALSE;

    // 배지 아이콘(《조》)은 이 DLL 안의 아이콘 자원을 쓴다.
    WCHAR szModule[MAX_PATH] = L"";
    GetModuleFileNameW(g_hInst, szModule, ARRAYSIZE(szModule));

    BOOL ok = FALSE;
    if (SUCCEEDED(pProfiles->Register(c_clsidKPS9256))) {
        HRESULT hr = pProfiles->AddLanguageProfile(
            c_clsidKPS9256, KPS_LANGID, c_guidProfile,
            c_szProfileName, (ULONG)wcslen(c_szProfileName),
            szModule, (ULONG)wcslen(szModule), 0); // 아이콘: 이 모듈의 첫 아이콘(《조》)
        if (SUCCEEDED(hr)) {
            pProfiles->EnableLanguageProfile(c_clsidKPS9256, KPS_LANGID, c_guidProfile, TRUE);
            ok = TRUE;
        }
    }
    pProfiles->Release();
    return ok;
}

static void UnregisterProfile()
{
    ITfInputProcessorProfiles* pProfiles = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                                   IID_ITfInputProcessorProfiles, (void**)&pProfiles)) && pProfiles) {
        pProfiles->Unregister(c_clsidKPS9256);
        pProfiles->Release();
    }
}

// 카테고리(능력) — SampleIME 가 등록하는 그대로 + 표시속성 제공자 (§6)
static const GUID* const kCategories[] = {
    &GUID_TFCAT_TIP_KEYBOARD,                 // 자판형 TIP
    &GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER,     // 밑줄 표시속성 제공
    &GUID_TFCAT_TIPCAP_UIELEMENTENABLED,
    &GUID_TFCAT_TIPCAP_SECUREMODE,            // 보안 데스크톱
    &GUID_TFCAT_TIPCAP_COMLESS,               // comless 적재
    &GUID_TFCAT_TIPCAP_INPUTMODECOMPARTMENT,  // 입력모드 칸 사용
    &GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT,      // 몰입형(스토어) 응용
    &GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT,
};

static BOOL RegisterCategories()
{
    ITfCategoryMgr* pCat = nullptr;
    if (FAILED(CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER,
                                IID_ITfCategoryMgr, (void**)&pCat)) || !pCat)
        return FALSE;

    BOOL ok = TRUE;
    for (size_t i = 0; i < ARRAYSIZE(kCategories); ++i) {
        if (FAILED(pCat->RegisterCategory(c_clsidKPS9256, *kCategories[i], c_clsidKPS9256)))
            ok = FALSE;
    }
    pCat->Release();
    return ok;
}

static void UnregisterCategories()
{
    ITfCategoryMgr* pCat = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER,
                                   IID_ITfCategoryMgr, (void**)&pCat)) && pCat) {
        for (size_t i = 0; i < ARRAYSIZE(kCategories); ++i)
            pCat->UnregisterCategory(c_clsidKPS9256, *kCategories[i], c_clsidKPS9256);
        pCat->Release();
    }
}

// ─── 내보낸 진입점 ───────────────────────────────────────────────────────────
STDAPI DllRegisterServer(void)
{
    HRESULT hrCo = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    BOOL ok = RegisterComServer() && RegisterProfile() && RegisterCategories();
    if (SUCCEEDED(hrCo)) CoUninitialize();
    if (!ok) { DllUnregisterServer(); return E_FAIL; }
    return S_OK;
}

STDAPI DllUnregisterServer(void)
{
    HRESULT hrCo = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    UnregisterCategories();
    UnregisterProfile();
    UnregisterComServer();
    if (SUCCEEDED(hrCo)) CoUninitialize();
    return S_OK;
}
