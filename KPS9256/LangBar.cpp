// LangBar.cpp — 한/영 토글 단추 (작업표시줄 입력 표시기)
//
// GUID_LBI_INPUTMODE 입력모드 단추로 등록한다. 한글이면 《조》, 영문이면 《A》 를
// 보이고, 누르면 변환모드 칸을 뒤집어 한/영을 전환한다(MS 입력기의 A↔한 단추와 같다).
#include "KPS9256.h"
#include "resource.h"
#include <olectl.h>   // CONNECT_E_*

// ─── CLangBarButton ──────────────────────────────────────────────────────────
CLangBarButton::CLangBarButton(CKPS9256TextService* tip)
    : _cRef(1), _pTip(tip), _pSink(nullptr) {}

CLangBarButton::~CLangBarButton()
{
    if (_pSink) { _pSink->Release(); _pSink = nullptr; }
}

STDMETHODIMP CLangBarButton::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv) return E_INVALIDARG;
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfLangBarItem) ||
        IsEqualIID(riid, IID_ITfLangBarItemButton))
        *ppv = static_cast<ITfLangBarItemButton*>(this);
    else if (IsEqualIID(riid, IID_ITfSource))
        *ppv = static_cast<ITfSource*>(this);
    else { *ppv = nullptr; return E_NOINTERFACE; }
    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) CLangBarButton::AddRef() { return ++_cRef; }
STDMETHODIMP_(ULONG) CLangBarButton::Release()
{
    LONG c = --_cRef;
    if (c == 0) delete this;
    return c;
}

// ── ITfLangBarItem ──
STDMETHODIMP CLangBarButton::GetInfo(TF_LANGBARITEMINFO* pInfo)
{
    if (!pInfo) return E_INVALIDARG;
    pInfo->clsidService = c_clsidKPS9256;
    pInfo->guidItem     = GUID_LBI_INPUTMODE;
    pInfo->dwStyle      = TF_LBI_STYLE_BTN_BUTTON | TF_LBI_STYLE_SHOWNINTRAY;
    pInfo->ulSort       = 0;
    lstrcpynW(pInfo->szDescription, L"한/영 전환", ARRAYSIZE(pInfo->szDescription));
    return S_OK;
}

STDMETHODIMP CLangBarButton::GetStatus(DWORD* pdwStatus)
{
    if (!pdwStatus) return E_INVALIDARG;
    *pdwStatus = 0;                      // 보임 + 사용가능
    return S_OK;
}

STDMETHODIMP CLangBarButton::Show(BOOL /*fShow*/) { return S_OK; }

STDMETHODIMP CLangBarButton::GetTooltipString(BSTR* pbstrToolTip)
{
    if (!pbstrToolTip) return E_INVALIDARG;
    *pbstrToolTip = SysAllocString(L"한/영 전환");
    return *pbstrToolTip ? S_OK : E_OUTOFMEMORY;
}

// ── ITfLangBarItemButton ──
STDMETHODIMP CLangBarButton::OnClick(TfLBIClick click, POINT /*pt*/, const RECT* /*prc*/)
{
    if (click == TF_LBI_CLK_LEFT && _pTip) _pTip->LbToggle();  // 한/영 토글
    return S_OK;
}

STDMETHODIMP CLangBarButton::InitMenu(ITfMenu* /*pMenu*/)   { return E_NOTIMPL; }
STDMETHODIMP CLangBarButton::OnMenuSelect(UINT /*wID*/)     { return E_NOTIMPL; }

STDMETHODIMP CLangBarButton::GetIcon(HICON* phIcon)
{
    if (!phIcon) return E_INVALIDARG;
    bool han = _pTip && _pTip->LbIsHangul();
    *phIcon = (HICON)LoadImageW(g_hInst, MAKEINTRESOURCEW(han ? IDI_MODE : IDI_MODE_ENG),
                                IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    return *phIcon ? S_OK : E_FAIL;
}

STDMETHODIMP CLangBarButton::GetText(BSTR* pbstrText)
{
    if (!pbstrText) return E_INVALIDARG;
    bool han = _pTip && _pTip->LbIsHangul();
    *pbstrText = SysAllocString(han ? L"조" : L"A");
    return *pbstrText ? S_OK : E_OUTOFMEMORY;
}

// ── ITfSource (언어바가 갱신 통지를 받으려고 싱크를 건다) ──
STDMETHODIMP CLangBarButton::AdviseSink(REFIID riid, IUnknown* punk, DWORD* pdwCookie)
{
    if (!punk || !pdwCookie) return E_INVALIDARG;
    if (!IsEqualIID(riid, IID_ITfLangBarItemSink)) return CONNECT_E_CANNOTCONNECT;
    if (_pSink) return CONNECT_E_ADVISELIMIT;
    if (FAILED(punk->QueryInterface(IID_ITfLangBarItemSink, (void**)&_pSink))) {
        _pSink = nullptr;
        return E_NOINTERFACE;
    }
    *pdwCookie = 0;                      // 싱크 하나만 받는다
    return S_OK;
}

STDMETHODIMP CLangBarButton::UnadviseSink(DWORD dwCookie)
{
    if (dwCookie != 0 || !_pSink) return CONNECT_E_NOCONNECTION;
    _pSink->Release();
    _pSink = nullptr;
    return S_OK;
}

void CLangBarButton::NotifyUpdate()
{
    if (_pSink) _pSink->OnUpdate(TF_LBI_ICON | TF_LBI_TEXT);  // 아이콘/글자 다시 그려라
}

// ─── CKPS9256TextService: 단추 설치/해제/갱신 ────────────────────────────────
BOOL CKPS9256TextService::_InitLangBarButton()
{
    if (!_pThreadMgr) return FALSE;
    ITfLangBarItemMgr* pMgr = nullptr;
    if (FAILED(_pThreadMgr->QueryInterface(IID_ITfLangBarItemMgr, (void**)&pMgr)) || !pMgr)
        return FALSE;

    _pLangBarButton = new (std::nothrow) CLangBarButton(this);
    BOOL ok = FALSE;
    if (_pLangBarButton)
        ok = SUCCEEDED(pMgr->AddItem(_pLangBarButton));
    pMgr->Release();

    if (!ok && _pLangBarButton) { _pLangBarButton->Release(); _pLangBarButton = nullptr; }
    return ok;
}

void CKPS9256TextService::_UninitLangBarButton()
{
    if (!_pLangBarButton) return;
    if (_pThreadMgr) {
        ITfLangBarItemMgr* pMgr = nullptr;
        if (SUCCEEDED(_pThreadMgr->QueryInterface(IID_ITfLangBarItemMgr, (void**)&pMgr)) && pMgr) {
            pMgr->RemoveItem(_pLangBarButton);
            pMgr->Release();
        }
    }
    _pLangBarButton->Release();
    _pLangBarButton = nullptr;
}

void CKPS9256TextService::_UpdateLangBarButton()
{
    if (_pLangBarButton) _pLangBarButton->NotifyUpdate();
}
