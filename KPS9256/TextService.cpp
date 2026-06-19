// TextService.cpp — CKPS9256TextService 생애주기·IUnknown·싱크 설치·한영 칸
#include "KPS9256.h"

CKPS9256TextService::CKPS9256TextService()
    : _cRef(1), _pThreadMgr(nullptr), _tfClientId(TF_CLIENTID_NULL), _dwActivateFlags(0),
      _dwThreadMgrCookie(TF_INVALID_COOKIE), _pComposition(nullptr), _pCompositionContext(nullptr),
      _cuasMode(false), _pendingBack(0),
      _pConvMode(nullptr), _dwConvModeCookie(TF_INVALID_COOKIE), _gaDisplayAttribute(TF_INVALID_GUIDATOM),
      _pLangBarButton(nullptr)
{
    DllAddRef();
}

CKPS9256TextService::~CKPS9256TextService()
{
    DllRelease();
}

// ─── IUnknown ────────────────────────────────────────────────────────────────
STDMETHODIMP CKPS9256TextService::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv) return E_INVALIDARG;
    *ppv = nullptr;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfTextInputProcessor) ||
        IsEqualIID(riid, IID_ITfTextInputProcessorEx))
        *ppv = static_cast<ITfTextInputProcessorEx*>(this);
    else if (IsEqualIID(riid, IID_ITfThreadMgrEventSink))
        *ppv = static_cast<ITfThreadMgrEventSink*>(this);
    else if (IsEqualIID(riid, IID_ITfKeyEventSink))
        *ppv = static_cast<ITfKeyEventSink*>(this);
    else if (IsEqualIID(riid, IID_ITfCompositionSink))
        *ppv = static_cast<ITfCompositionSink*>(this);
    else if (IsEqualIID(riid, IID_ITfCompartmentEventSink))
        *ppv = static_cast<ITfCompartmentEventSink*>(this);
    else if (IsEqualIID(riid, IID_ITfDisplayAttributeProvider))
        *ppv = static_cast<ITfDisplayAttributeProvider*>(this);

    if (*ppv) { AddRef(); return S_OK; }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CKPS9256TextService::AddRef()  { return ++_cRef; }
STDMETHODIMP_(ULONG) CKPS9256TextService::Release()
{
    LONG c = --_cRef;
    if (c == 0) delete this;
    return c;
}

// ─── 활성/비활성 ─────────────────────────────────────────────────────────────
STDMETHODIMP CKPS9256TextService::Activate(ITfThreadMgr* ptim, TfClientId tid)
{
    return ActivateEx(ptim, tid, 0);
}

STDMETHODIMP CKPS9256TextService::ActivateEx(ITfThreadMgr* ptim, TfClientId tid, DWORD dwFlags)
{
    _pThreadMgr = ptim;
    _pThreadMgr->AddRef();
    _tfClientId      = tid;
    _dwActivateFlags = dwFlags;

    _InitThreadMgrEventSink();
    _InitKeyEventSink();
    _InitPreservedKeys();
    _InitDisplayAttributeGuid();
    _InitConversionCompartment();
    _InitLangBarButton();              // 한/영 토글 단추(작업표시줄)
    return S_OK;
}

STDMETHODIMP CKPS9256TextService::Deactivate()
{
    // 비활성화 직전 조합중 음절 확정 (§5)
    if (_pComposition && _pCompositionContext)
        _FinalizeComposition(_pCompositionContext);

    _UninitLangBarButton();
    _UninitConversionCompartment();
    _UninitPreservedKeys();
    _UninitKeyEventSink();
    _UninitThreadMgrEventSink();

    if (_pComposition)          { _pComposition->Release();        _pComposition = nullptr; }
    if (_pCompositionContext)   { _pCompositionContext->Release(); _pCompositionContext = nullptr; }
    if (_pConvMode)             { _pConvMode->Release();           _pConvMode = nullptr; }
    if (_pThreadMgr)            { _pThreadMgr->Release();          _pThreadMgr = nullptr; }

    _tfClientId = TF_CLIENTID_NULL;
    _gaDisplayAttribute = TF_INVALID_GUIDATOM;
    _composer.Reset();
    return S_OK;
}

// ─── 스레드관리자 이벤트 싱크 ────────────────────────────────────────────────
BOOL CKPS9256TextService::_InitThreadMgrEventSink()
{
    ITfSource* pSource = nullptr;
    if (FAILED(_pThreadMgr->QueryInterface(IID_ITfSource, (void**)&pSource)) || !pSource)
        return FALSE;
    HRESULT hr = pSource->AdviseSink(IID_ITfThreadMgrEventSink,
                                     static_cast<ITfThreadMgrEventSink*>(this),
                                     &_dwThreadMgrCookie);
    pSource->Release();
    return SUCCEEDED(hr);
}

void CKPS9256TextService::_UninitThreadMgrEventSink()
{
    if (_dwThreadMgrCookie == TF_INVALID_COOKIE) return;
    ITfSource* pSource = nullptr;
    if (SUCCEEDED(_pThreadMgr->QueryInterface(IID_ITfSource, (void**)&pSource)) && pSource) {
        pSource->UnadviseSink(_dwThreadMgrCookie);
        pSource->Release();
    }
    _dwThreadMgrCookie = TF_INVALID_COOKIE;
}

// ─── 키 이벤트 싱크 ──────────────────────────────────────────────────────────
BOOL CKPS9256TextService::_InitKeyEventSink()
{
    ITfKeystrokeMgr* pKsm = nullptr;
    if (FAILED(_pThreadMgr->QueryInterface(IID_ITfKeystrokeMgr, (void**)&pKsm)) || !pKsm)
        return FALSE;
    HRESULT hr = pKsm->AdviseKeyEventSink(_tfClientId, static_cast<ITfKeyEventSink*>(this), TRUE);
    pKsm->Release();
    return SUCCEEDED(hr);
}

void CKPS9256TextService::_UninitKeyEventSink()
{
    ITfKeystrokeMgr* pKsm = nullptr;
    if (SUCCEEDED(_pThreadMgr->QueryInterface(IID_ITfKeystrokeMgr, (void**)&pKsm)) && pKsm) {
        pKsm->UnadviseKeyEventSink(_tfClientId);
        pKsm->Release();
    }
}

// ─── 토글건(preserved key) ───────────────────────────────────────────────────
BOOL CKPS9256TextService::_InitPreservedKeys()
{
    ITfKeystrokeMgr* pKsm = nullptr;
    if (FAILED(_pThreadMgr->QueryInterface(IID_ITfKeystrokeMgr, (void**)&pKsm)) || !pKsm)
        return FALSE;

    // VK_HANGUL(0x15): 조선/한국 자판 드라이버가 한/영(오른쪽 Alt)키에서 보낸다.
    //   TF_MOD_IGNORE_ALL_MODIFIER: Shift/Ctrl 등을 같이 눌러도 토글되게(수식키 무시).
    TF_PRESERVEDKEY pkHangul = { VK_HANGUL, TF_MOD_IGNORE_ALL_MODIFIER };
    pKsm->PreserveKey(_tfClientId, c_guidPreservedHangul, &pkHangul, nullptr, 0);

    // Shift+Space 도 토글.
    TF_PRESERVEDKEY pkShiftSpace = { VK_SPACE, TF_MOD_SHIFT };
    pKsm->PreserveKey(_tfClientId, c_guidPreservedShiftSpc, &pkShiftSpace, nullptr, 0);

    pKsm->Release();
    return TRUE;
}

void CKPS9256TextService::_UninitPreservedKeys()
{
    ITfKeystrokeMgr* pKsm = nullptr;
    if (SUCCEEDED(_pThreadMgr->QueryInterface(IID_ITfKeystrokeMgr, (void**)&pKsm)) && pKsm) {
        TF_PRESERVEDKEY pkHangul = { VK_HANGUL, TF_MOD_IGNORE_ALL_MODIFIER };
        pKsm->UnpreserveKey(c_guidPreservedHangul, &pkHangul);
        TF_PRESERVEDKEY pkShiftSpace = { VK_SPACE, TF_MOD_SHIFT };
        pKsm->UnpreserveKey(c_guidPreservedShiftSpc, &pkShiftSpace);
        pKsm->Release();
    }
}

// ─── 표시속성 GUID 등록 ──────────────────────────────────────────────────────
BOOL CKPS9256TextService::_InitDisplayAttributeGuid()
{
    ITfCategoryMgr* pCat = nullptr;
    if (FAILED(CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER,
                                IID_ITfCategoryMgr, (void**)&pCat)) || !pCat)
        return FALSE;
    HRESULT hr = pCat->RegisterGUID(c_guidDisplayAttribute, &_gaDisplayAttribute);
    pCat->Release();
    return SUCCEEDED(hr);
}

// ─── 한/영 상태 저장(레지스트리) — 앱을 새로 열어도 마지막 상태를 기억 ────────
static LONG _ReadSavedConvMode()
{
    DWORD val = 0, sz = sizeof(val);
    HKEY k = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\KPS9256", 0, KEY_READ, &k) == ERROR_SUCCESS) {
        RegQueryValueExW(k, L"ConvMode", nullptr, nullptr, (LPBYTE)&val, &sz);
        RegCloseKey(k);
    }
    return (LONG)val;
}
static void _WriteSavedConvMode(LONG mode)
{
    HKEY k = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\KPS9256", 0, nullptr, 0,
                        KEY_WRITE, nullptr, &k, nullptr) == ERROR_SUCCESS) {
        DWORD v = (DWORD)mode;
        RegSetValueExW(k, L"ConvMode", 0, REG_DWORD, (const BYTE*)&v, sizeof(v));
        RegCloseKey(k);
    }
}

// ─── 변환모드 칸 (한/영 상태) ────────────────────────────────────────────────
BOOL CKPS9256TextService::_InitConversionCompartment()
{
    ITfCompartmentMgr* pCompMgr = nullptr;
    if (FAILED(_pThreadMgr->QueryInterface(IID_ITfCompartmentMgr, (void**)&pCompMgr)) || !pCompMgr)
        return FALSE;
    pCompMgr->GetCompartment(GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION, &_pConvMode);
    pCompMgr->Release();
    if (!_pConvMode) return FALSE;

    // 저장해 둔 한/영 상태를 복원한다 — 새 앱도 마지막 상태를 기억(MS 입력기와 동일).
    VARIANT z; z.vt = VT_I4; z.lVal = _ReadSavedConvMode();
    _pConvMode->SetValue(_tfClientId, &z);

    // 변경 싱크
    ITfSource* pSource = nullptr;
    if (SUCCEEDED(_pConvMode->QueryInterface(IID_ITfSource, (void**)&pSource)) && pSource) {
        pSource->AdviseSink(IID_ITfCompartmentEventSink,
                            static_cast<ITfCompartmentEventSink*>(this), &_dwConvModeCookie);
        pSource->Release();
    }
    return TRUE;
}

void CKPS9256TextService::_UninitConversionCompartment()
{
    if (_pConvMode && _dwConvModeCookie != TF_INVALID_COOKIE) {
        ITfSource* pSource = nullptr;
        if (SUCCEEDED(_pConvMode->QueryInterface(IID_ITfSource, (void**)&pSource)) && pSource) {
            pSource->UnadviseSink(_dwConvModeCookie);
            pSource->Release();
        }
    }
    _dwConvModeCookie = TF_INVALID_COOKIE;
}

bool CKPS9256TextService::_IsHangulMode()
{
    if (!_pConvMode) return false;
    VARIANT v; VariantInit(&v);
    bool on = false;
    if (SUCCEEDED(_pConvMode->GetValue(&v)) && v.vt == VT_I4)
        on = (v.lVal & TF_CONVERSIONMODE_NATIVE) != 0;
    VariantClear(&v);
    return on;
}

void CKPS9256TextService::_ToggleHangulMode()
{
    if (!_pConvMode) return;
    VARIANT v; VariantInit(&v);
    LONG cur = 0;
    if (SUCCEEDED(_pConvMode->GetValue(&v)) && v.vt == VT_I4)
        cur = v.lVal;
    VariantClear(&v);

    cur ^= TF_CONVERSIONMODE_NATIVE;       // 한글(NATIVE) 비트만 뒤집는다 (§5)
    VARIANT n; n.vt = VT_I4; n.lVal = cur;
    _pConvMode->SetValue(_tfClientId, &n); // → OnChange 가 불려 조합을 확정한다
}

// ─── ITfThreadMgrEventSink ───────────────────────────────────────────────────
STDMETHODIMP CKPS9256TextService::OnSetFocus(ITfDocumentMgr* /*pdimFocus*/, ITfDocumentMgr* /*pdimPrev*/)
{
    // 초점이 바뀌면 조합중 음절을 확정 (§5).
    if (_pComposition && _pCompositionContext)
        _FinalizeComposition(_pCompositionContext);
    // 다른 칸으로 옮기면 CUAS 우회 상태를 비운다(새 칸에서 다시 판단).
    _composer.Reset();
    _cuasMode = false;
    _cuasDoc.clear();
    _pendingBack = 0;
    return S_OK;
}

// ─── ITfCompartmentEventSink ─────────────────────────────────────────────────
STDMETHODIMP CKPS9256TextService::OnChange(REFGUID rguid)
{
    if (IsEqualGUID(rguid, GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION)) {
        // 한/영이 바뀌었으니 조합중 음절을 확정.
        if (_pComposition && _pCompositionContext) {
            _FinalizeComposition(_pCompositionContext);
        } else {
            ITfDocumentMgr* pDim = nullptr;
            if (SUCCEEDED(_pThreadMgr->GetFocus(&pDim)) && pDim) {
                ITfContext* pCtx = nullptr;
                if (SUCCEEDED(pDim->GetTop(&pCtx)) && pCtx) {
                    _FinalizeComposition(pCtx);
                    pCtx->Release();
                }
                pDim->Release();
            }
        }
        _UpdateLangBarButton();          // 단추 아이콘/글자(조↔A) 갱신

        // 바뀐 한/영 상태를 저장 → 다음에 여는 앱이 이 상태로 시작.
        if (_pConvMode) {
            VARIANT v; VariantInit(&v);
            if (SUCCEEDED(_pConvMode->GetValue(&v)) && v.vt == VT_I4)
                _WriteSavedConvMode(v.lVal);
            VariantClear(&v);
        }
    }
    return S_OK;
}

// ─── ITfCompositionSink ──────────────────────────────────────────────────────
STDMETHODIMP CKPS9256TextService::OnCompositionTerminated(TfEditCookie /*ec*/, ITfComposition* /*pComposition*/)
{
    if (_pComposition)        { _pComposition->Release();        _pComposition = nullptr; }
    if (_pCompositionContext) { _pCompositionContext->Release(); _pCompositionContext = nullptr; }

    // 조합 중인데 강제 종료됐다 = CUAS(실행창/탐색기 이름칸)가 매 글자 확정한 것.
    //   이후로는 TSF 조합 대신 키 주입으로 처리하도록 표시하고, 자모 상태는 유지한다.
    if (_composer.IsComposing()) {
        _cuasMode = true;
        _cuasDoc  = _composer.Preedit();   // 방금 CUAS 가 확정해 문서에 남긴 글자
    } else {
        _composer.Reset();
        _cuasMode = false;
        _cuasDoc.clear();
    }
    return S_OK;
}

// ─── ITfDisplayAttributeProvider ─────────────────────────────────────────────
STDMETHODIMP CKPS9256TextService::EnumDisplayAttributeInfo(IEnumTfDisplayAttributeInfo** ppEnum)
{
    if (!ppEnum) return E_INVALIDARG;
    *ppEnum = new (std::nothrow) CEnumDisplayAttributeInfo();
    return *ppEnum ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP CKPS9256TextService::GetDisplayAttributeInfo(REFGUID guid, ITfDisplayAttributeInfo** ppInfo)
{
    if (!ppInfo) return E_INVALIDARG;
    *ppInfo = nullptr;
    if (IsEqualGUID(guid, c_guidDisplayAttribute)) {
        *ppInfo = new (std::nothrow) CDisplayAttributeInfo();
        return *ppInfo ? S_OK : E_OUTOFMEMORY;
    }
    return E_INVALIDARG;
}
