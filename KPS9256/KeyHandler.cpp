// KeyHandler.cpp — ITfKeyEventSink + 조합 편집세션 (§5 키 처리 흐름)
#include "KPS9256.h"
#include "KeyMap.h"
#include <vector>

// ─── 보조 ────────────────────────────────────────────────────────────────────
static inline bool ShiftDown()   { return (GetKeyState(VK_SHIFT)   & 0x8000) != 0; }
static inline bool CtrlDown()    { return (GetKeyState(VK_CONTROL) & 0x8000) != 0; }
static inline bool AltDown()     { return (GetKeyState(VK_MENU)    & 0x8000) != 0; }

bool CKPS9256TextService::_IsModifierVK(UINT vk)
{
    switch (vk) {
    case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT:
    case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL:
    case VK_MENU: case VK_LMENU: case VK_RMENU:
    case VK_CAPITAL: case VK_LWIN: case VK_RWIN:
    case VK_HANGUL: case VK_HANJA:        // 0x15 / 0x19
    case VK_PROCESSKEY:
        return true;
    }
    return false;
}

// ─── ITfKeyEventSink: 초점/Up ────────────────────────────────────────────────
STDMETHODIMP CKPS9256TextService::OnSetFocus(BOOL /*fForeground*/) { return S_OK; }
STDMETHODIMP CKPS9256TextService::OnTestKeyUp(ITfContext*, WPARAM, LPARAM, BOOL* pfEaten)
{ if (pfEaten) *pfEaten = FALSE; return S_OK; }
STDMETHODIMP CKPS9256TextService::OnKeyUp(ITfContext*, WPARAM, LPARAM, BOOL* pfEaten)
{ if (pfEaten) *pfEaten = FALSE; return S_OK; }

// ─── 토글건 ──────────────────────────────────────────────────────────────────
STDMETHODIMP CKPS9256TextService::OnPreservedKey(ITfContext* /*pic*/, REFGUID rguid, BOOL* pfEaten)
{
    if (pfEaten) *pfEaten = FALSE;
    if (IsEqualGUID(rguid, c_guidPreservedHangul) ||
        IsEqualGUID(rguid, c_guidPreservedShiftSpc)) {
        _ToggleHangulMode();              // 칸 값을 뒤집는다. OnChange 가 조합을 확정.
        if (pfEaten) *pfEaten = TRUE;
    }
    return S_OK;
}

// ─── OnTestKeyDown: 이 키를 먹을지 예고 ──────────────────────────────────────
STDMETHODIMP CKPS9256TextService::OnTestKeyDown(ITfContext* /*pic*/, WPARAM wParam, LPARAM /*lParam*/, BOOL* pfEaten)
{
    if (pfEaten) *pfEaten = FALSE;
    if (!pfEaten) return S_OK;

    if ((UINT)wParam == VK_PACKET) return S_OK;                    // 유니코드 주입 — 통과
    if (_pendingBack > 0 && (UINT)wParam == VK_BACK) return S_OK;  // 우리가 쏜 백스페이스 — 통과
    if (!_IsHangulMode()) return S_OK;        // 영문모드: 전부 통과
    if (CtrlDown() || AltDown()) return S_OK; // 단축키는 통과(확정 안 함)

    UINT vk = (UINT)wParam;
    if (KpsMapKey(vk, ShiftDown()) != 0) { *pfEaten = TRUE; return S_OK; } // 자모 키

    if (!_composer.IsComposing()) return S_OK; // 조합 안 함 + 비자모 → 통과
    if (_IsModifierVK(vk)) return S_OK;        // 순수 수식키는 무시

    // 조합중 + 비자모(Backspace/Space/Enter/구두점/이동키 등) → OnKeyDown 으로 넘겨
    // 확정 처리를 시킨다.
    *pfEaten = TRUE;
    return S_OK;
}

// ─── OnKeyDown: 실제 처리 ────────────────────────────────────────────────────
STDMETHODIMP CKPS9256TextService::OnKeyDown(ITfContext* pic, WPARAM wParam, LPARAM /*lParam*/, BOOL* pfEaten)
{
    if (pfEaten) *pfEaten = FALSE;
    if (!pfEaten) return S_OK;

    if ((UINT)wParam == VK_PACKET) return S_OK;  // 유니코드 주입 — 통과(먹지 않음)
    if (_pendingBack > 0 && (UINT)wParam == VK_BACK) { _pendingBack--; return S_OK; } // 합성 백스페이스: 세고 통과
    if (!_IsHangulMode()) return S_OK;
    if (CtrlDown() || AltDown()) return S_OK;

    UINT vk = (UINT)wParam;

    // 자모 키: 자동기에 넣는다.
    wchar_t jamo = KpsMapKey(vk, ShiftDown());
    if (jamo != 0) {
        _HandleJamo(pic, jamo);
        *pfEaten = TRUE;
        return S_OK;
    }

    if (!_composer.IsComposing()) return S_OK;
    if (_IsModifierVK(vk)) return S_OK;

    if (vk == VK_BACK) {                  // 물림
        _HandleBackspace(pic);
        *pfEaten = TRUE;
        return S_OK;
    }

    // 그 밖의 키(Space/Enter/구두점/이동 등): 현 음절 확정 후 키 통과 (§5).
    //   OnTestKeyDown 에서 TRUE 를 줘 여기로 왔지만, 확정만 하고 *pfEaten=FALSE
    //   로 돌려 응용이 키를 받게 한다.
    _FinalizeComposition(pic);
    *pfEaten = FALSE;
    return S_OK;
}

// ─── CUAS 키 주입 (백스페이스로 지우고 유니코드로 넣기) ──────────────────────
static void SendBackspaces(int n)
{
    if (n <= 0) return;
    std::vector<INPUT> in;
    for (int i = 0; i < n; ++i) {
        INPUT d = {}; d.type = INPUT_KEYBOARD; d.ki.wVk = VK_BACK;
        INPUT u = {}; u.type = INPUT_KEYBOARD; u.ki.wVk = VK_BACK; u.ki.dwFlags = KEYEVENTF_KEYUP;
        in.push_back(d); in.push_back(u);
    }
    SendInput((UINT)in.size(), in.data(), sizeof(INPUT));
}
static void SendUnicode(const std::wstring& s)
{
    if (s.empty()) return;
    std::vector<INPUT> in;
    for (wchar_t c : s) {
        INPUT d = {}; d.type = INPUT_KEYBOARD; d.ki.wScan = c; d.ki.dwFlags = KEYEVENTF_UNICODE;
        INPUT u = {}; u.type = INPUT_KEYBOARD; u.ki.wScan = c; u.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
        in.push_back(d); in.push_back(u);
    }
    SendInput((UINT)in.size(), in.data(), sizeof(INPUT));
}

// 문서에 남아 있는 직전 미리편집(_cuasDoc)을 지우고 commit+preedit 를 넣는다.
void CKPS9256TextService::_InjectReplace(const std::wstring& commit, const std::wstring& preedit)
{
    int back = (int)_cuasDoc.length();
    _pendingBack += back;                  // 우리가 쏘는 백스페이스는 무시하도록 표시
    SendBackspaces(back);
    SendUnicode(commit + preedit);
    _cuasDoc = preedit;                    // commit 분은 영구, preedit 분만 다음에 교체
}

// ─── 처리 본체 ───────────────────────────────────────────────────────────────
void CKPS9256TextService::_HandleJamo(ITfContext* pic, wchar_t jamo)
{
    std::wstring commit;
    _composer.Input(jamo, commit);
    if (_cuasMode) _InjectReplace(commit, _composer.Preedit());
    else           _RequestApply(pic, commit, _composer.Preedit());
}

void CKPS9256TextService::_HandleBackspace(ITfContext* pic)
{
    _composer.Backspace();
    if (_cuasMode) _InjectReplace(std::wstring(), _composer.Preedit());
    else           _RequestApply(pic, std::wstring(), _composer.Preedit());
}

void CKPS9256TextService::_FinalizeComposition(ITfContext* pic)
{
    if (_cuasMode) {
        // 미리편집 글자는 이미 보통글자로 문서에 들어가 있다. 상태만 정리.
        std::wstring c; _composer.Flush(c);
        _cuasDoc.clear();
        _pendingBack = 0;
        return;
    }
    if (!pic) return;
    std::wstring commit;
    _composer.Flush(commit);
    if (commit.empty() && !_pComposition) return;
    _RequestApply(pic, commit, std::wstring());
}

// ─── 편집세션 요청 ───────────────────────────────────────────────────────────
void CKPS9256TextService::_RequestApply(ITfContext* pic,
                                        const std::wstring& commit, const std::wstring& preedit)
{
    if (!pic) return;
    CEditSession* pes = new (std::nothrow) CEditSession(this, pic, commit, preedit);
    if (!pes) return;

    HRESULT hrSession = E_FAIL;
    // 키 처리 문맥에서는 동기 세션이 보통 허락된다.
    HRESULT hr = pic->RequestEditSession(_tfClientId, pes,
                                         TF_ES_SYNC | TF_ES_READWRITE, &hrSession);
    if (FAILED(hr) || FAILED(hrSession)) {
        // 동기 거부 시 비동기로.
        pic->RequestEditSession(_tfClientId, pes,
                                TF_ES_ASYNCDONTCARE | TF_ES_READWRITE, &hrSession);
    }
    pes->Release();
}

// ─── 조합 적용 (편집세션 안) ─────────────────────────────────────────────────
void CKPS9256TextService::ApplyComposition(TfEditCookie ec, ITfContext* pic,
                                           const std::wstring& commit, const std::wstring& preedit)
{
    // 직전 조합이 살아남아 여기 왔다 = 정상 앱(TSF 동작) → CUAS 오인 카운터 초기화.
    if (_pComposition) _cuasConfirm = 0;

    if (!commit.empty()) {
        _StartCompositionWithText(ec, pic, commit);  // 시작(글자 먼저) 또는 갱신
        _MoveCaretToEnd(ec, pic);            // 확정 글자 뒤로 캐럿 이동(다음 조합이 안 덮게)
        _EndComposition(ec, pic);            // 확정 → 보통글자로 남고 조합 종료
    }

    if (!preedit.empty()) {
        _StartCompositionWithText(ec, pic, preedit);
        _ApplyDisplayAttribute(ec, pic);     // 밑줄
    } else if (_pComposition) {
        // 조합 글자가 없으면(예: 물림으로 비워짐) 남은 조합을 지우고 종료.
        _SetText(ec, std::wstring());
        _EndComposition(ec, pic);
    }
}

void CKPS9256TextService::_StartCompositionWithText(TfEditCookie ec, ITfContext* pic,
                                                    const std::wstring& text)
{
    if (_pComposition) { _SetText(ec, text); return; }   // 이미 조합중이면 글자만 갱신

    ITfInsertAtSelection* pIns = nullptr;
    if (FAILED(pic->QueryInterface(IID_ITfInsertAtSelection, (void**)&pIns)) || !pIns)
        return;

    // 빈 범위가 아니라 실제 글자를 넣고 그 위에 조합을 건다.
    //   CUAS(탐색기 이름칸·실행창 등 옛 편집칸)는 빈 범위 조합을 유지하지 못해
    //   자모가 분리된다. 글자를 먼저 넣으면 조합이 제대로 걸린다.
    ITfRange* pRange = nullptr;
    HRESULT hr = pIns->InsertTextAtSelection(ec, 0, text.c_str(), (LONG)text.length(), &pRange);
    pIns->Release();
    if (FAILED(hr) || !pRange) return;

    ITfContextComposition* pCC = nullptr;
    if (SUCCEEDED(pic->QueryInterface(IID_ITfContextComposition, (void**)&pCC)) && pCC) {
        pCC->StartComposition(ec, pRange, static_cast<ITfCompositionSink*>(this), &_pComposition);
        pCC->Release();
    }
    pRange->Release();

    if (_pComposition) {
        _pCompositionContext = pic;
        _pCompositionContext->AddRef();
    }
}

void CKPS9256TextService::_SetText(TfEditCookie ec, const std::wstring& text)
{
    if (!_pComposition) return;
    ITfRange* pRange = nullptr;
    if (FAILED(_pComposition->GetRange(&pRange)) || !pRange) return;
    pRange->SetText(ec, 0, text.c_str(), (LONG)text.length());
    pRange->Release();
}

void CKPS9256TextService::_ApplyDisplayAttribute(TfEditCookie ec, ITfContext* pic)
{
    if (_gaDisplayAttribute == TF_INVALID_GUIDATOM || !_pComposition) return;
    ITfProperty* pProp = nullptr;
    if (FAILED(pic->GetProperty(GUID_PROP_ATTRIBUTE, &pProp)) || !pProp) return;

    ITfRange* pRange = nullptr;
    if (SUCCEEDED(_pComposition->GetRange(&pRange)) && pRange) {
        VARIANT v; VariantInit(&v);
        v.vt = VT_I4; v.lVal = (LONG)_gaDisplayAttribute;
        pProp->SetValue(ec, pRange, &v);
        pRange->Release();
    }
    pProp->Release();
}

void CKPS9256TextService::_ClearDisplayAttribute(TfEditCookie ec, ITfContext* pic, ITfRange* pRange)
{
    if (!pRange) return;
    ITfProperty* pProp = nullptr;
    if (SUCCEEDED(pic->GetProperty(GUID_PROP_ATTRIBUTE, &pProp)) && pProp) {
        pProp->Clear(ec, pRange);
        pProp->Release();
    }
}

void CKPS9256TextService::_MoveCaretToEnd(TfEditCookie ec, ITfContext* pic)
{
    if (!_pComposition) return;
    ITfRange* pRange = nullptr;
    if (FAILED(_pComposition->GetRange(&pRange)) || !pRange) return;

    ITfRange* pEnd = nullptr;
    if (SUCCEEDED(pRange->Clone(&pEnd)) && pEnd) {
        pEnd->Collapse(ec, TF_ANCHOR_END);
        TF_SELECTION sel;
        sel.range = pEnd;
        sel.style.ase = TF_AE_NONE;
        sel.style.fInterimChar = FALSE;
        pic->SetSelection(ec, 1, &sel);
        pEnd->Release();
    }
    pRange->Release();
}

void CKPS9256TextService::_EndComposition(TfEditCookie ec, ITfContext* pic)
{
    if (!_pComposition) return;

    // 확정될 범위의 밑줄 속성을 지워 보통글자로 남게 한다.
    ITfRange* pRange = nullptr;
    if (SUCCEEDED(_pComposition->GetRange(&pRange)) && pRange) {
        _ClearDisplayAttribute(ec, pic, pRange);
        pRange->Release();
    }

    _pComposition->EndComposition(ec);
    _pComposition->Release();
    _pComposition = nullptr;
    if (_pCompositionContext) { _pCompositionContext->Release(); _pCompositionContext = nullptr; }
}

// ─── CEditSession ────────────────────────────────────────────────────────────
CEditSession::CEditSession(CKPS9256TextService* tip, ITfContext* pic,
                           const std::wstring& commit, const std::wstring& preedit)
    : _cRef(1), _pTip(tip), _pContext(pic), _commit(commit), _preedit(preedit)
{
    if (_pTip)     _pTip->AddRef();
    if (_pContext) _pContext->AddRef();
}

CEditSession::~CEditSession()
{
    if (_pContext) _pContext->Release();
    if (_pTip)     _pTip->Release();
}

STDMETHODIMP CEditSession::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv) return E_INVALIDARG;
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfEditSession)) {
        *ppv = static_cast<ITfEditSession*>(this);
        AddRef();
        return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CEditSession::AddRef()  { return ++_cRef; }
STDMETHODIMP_(ULONG) CEditSession::Release()
{
    LONG c = --_cRef;
    if (c == 0) delete this;
    return c;
}

STDMETHODIMP CEditSession::DoEditSession(TfEditCookie ec)
{
    if (_pTip) _pTip->ApplyComposition(ec, _pContext, _commit, _preedit);
    return S_OK;
}
