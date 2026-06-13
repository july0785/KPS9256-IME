// KPS9256.h — TSF 입력기 클래스 선언
//
// CKPS9256TextService 가 TSF/COM 껍데기 전부를 구현하고, 입력핵은 §4
// HangulComposer 로 위임한다. SampleIME 의 표/후보창 입력핵은 쓰지 않는다.
#pragma once
#include "Globals.h"
#include "Hangul.h"
#include <string>
#include <new>

class CEditSession;

// ─── 입력 처리기 본체 ────────────────────────────────────────────────────────
class CKPS9256TextService :
    public ITfTextInputProcessorEx,
    public ITfThreadMgrEventSink,
    public ITfKeyEventSink,
    public ITfCompositionSink,
    public ITfCompartmentEventSink,
    public ITfDisplayAttributeProvider
{
public:
    CKPS9256TextService();
    ~CKPS9256TextService();

    // IUnknown
    STDMETHODIMP          QueryInterface(REFIID riid, void** ppv) override;
    STDMETHODIMP_(ULONG)  AddRef() override;
    STDMETHODIMP_(ULONG)  Release() override;

    // ITfTextInputProcessor / Ex
    STDMETHODIMP Activate(ITfThreadMgr* ptim, TfClientId tid) override;
    STDMETHODIMP Deactivate() override;
    STDMETHODIMP ActivateEx(ITfThreadMgr* ptim, TfClientId tid, DWORD dwFlags) override;

    // ITfThreadMgrEventSink
    STDMETHODIMP OnInitDocumentMgr(ITfDocumentMgr*) override { return S_OK; }
    STDMETHODIMP OnUninitDocumentMgr(ITfDocumentMgr*) override { return S_OK; }
    STDMETHODIMP OnSetFocus(ITfDocumentMgr* pdimFocus, ITfDocumentMgr* pdimPrev) override;
    STDMETHODIMP OnPushContext(ITfContext*) override { return S_OK; }
    STDMETHODIMP OnPopContext(ITfContext*) override { return S_OK; }

    // ITfKeyEventSink
    STDMETHODIMP OnSetFocus(BOOL fForeground) override;
    STDMETHODIMP OnTestKeyDown(ITfContext* pic, WPARAM wParam, LPARAM lParam, BOOL* pfEaten) override;
    STDMETHODIMP OnKeyDown(ITfContext* pic, WPARAM wParam, LPARAM lParam, BOOL* pfEaten) override;
    STDMETHODIMP OnTestKeyUp(ITfContext* pic, WPARAM wParam, LPARAM lParam, BOOL* pfEaten) override;
    STDMETHODIMP OnKeyUp(ITfContext* pic, WPARAM wParam, LPARAM lParam, BOOL* pfEaten) override;
    STDMETHODIMP OnPreservedKey(ITfContext* pic, REFGUID rguid, BOOL* pfEaten) override;

    // ITfCompositionSink
    STDMETHODIMP OnCompositionTerminated(TfEditCookie ecWrite, ITfComposition* pComposition) override;

    // ITfCompartmentEventSink
    STDMETHODIMP OnChange(REFGUID rguid) override;

    // ITfDisplayAttributeProvider
    STDMETHODIMP EnumDisplayAttributeInfo(IEnumTfDisplayAttributeInfo** ppEnum) override;
    STDMETHODIMP GetDisplayAttributeInfo(REFGUID guid, ITfDisplayAttributeInfo** ppInfo) override;

    // CEditSession 가 부른다 (편집세션 안에서 조합 갱신)
    void ApplyComposition(TfEditCookie ec, ITfContext* pic,
                          const std::wstring& commit, const std::wstring& preedit);

private:
    // 설치/해제
    BOOL _InitThreadMgrEventSink();
    void _UninitThreadMgrEventSink();
    BOOL _InitKeyEventSink();
    void _UninitKeyEventSink();
    BOOL _InitPreservedKeys();
    void _UninitPreservedKeys();
    BOOL _InitDisplayAttributeGuid();
    BOOL _InitConversionCompartment();
    void _UninitConversionCompartment();

    // 한/영 상태 — 절대 자체 카운터로 추적하지 않는다 (§5). 칸 값을 읽고 쓴다.
    bool _IsHangulMode();
    void _ToggleHangulMode();

    // 키 → 처리
    void _HandleJamo(ITfContext* pic, wchar_t jamo);
    void _HandleBackspace(ITfContext* pic);
    void _FinalizeComposition(ITfContext* pic);            // 현 음절 확정
    void _RequestApply(ITfContext* pic,
                       const std::wstring& commit, const std::wstring& preedit);

    // 조합(편집세션 안에서만)
    void _StartCompositionIfNeeded(TfEditCookie ec, ITfContext* pic);
    void _SetText(TfEditCookie ec, const std::wstring& text);
    void _ApplyDisplayAttribute(TfEditCookie ec, ITfContext* pic);
    void _ClearDisplayAttribute(TfEditCookie ec, ITfContext* pic, ITfRange* pRange);
    void _MoveCaretToEnd(TfEditCookie ec, ITfContext* pic);
    void _EndComposition(TfEditCookie ec, ITfContext* pic);

    static bool _IsModifierVK(UINT vk);

private:
    LONG            _cRef;
    ITfThreadMgr*   _pThreadMgr;
    TfClientId      _tfClientId;
    DWORD           _dwActivateFlags;

    DWORD           _dwThreadMgrCookie;
    ITfComposition* _pComposition;
    ITfContext*     _pCompositionContext;

    ITfCompartment* _pConvMode;          // 변환모드 칸
    DWORD           _dwConvModeCookie;

    TfGuidAtom      _gaDisplayAttribute;

    HangulComposer  _composer;           // §4 입력핵
};

// ─── 편집세션 ────────────────────────────────────────────────────────────────
class CEditSession : public ITfEditSession
{
public:
    CEditSession(CKPS9256TextService* tip, ITfContext* pic,
                 const std::wstring& commit, const std::wstring& preedit);
    ~CEditSession();

    STDMETHODIMP          QueryInterface(REFIID riid, void** ppv) override;
    STDMETHODIMP_(ULONG)  AddRef() override;
    STDMETHODIMP_(ULONG)  Release() override;
    STDMETHODIMP          DoEditSession(TfEditCookie ec) override;

private:
    LONG                 _cRef;
    CKPS9256TextService* _pTip;
    ITfContext*          _pContext;
    std::wstring         _commit;
    std::wstring         _preedit;
};

// ─── 표시속성(밑줄) ──────────────────────────────────────────────────────────
class CDisplayAttributeInfo : public ITfDisplayAttributeInfo
{
public:
    CDisplayAttributeInfo();
    STDMETHODIMP          QueryInterface(REFIID riid, void** ppv) override;
    STDMETHODIMP_(ULONG)  AddRef() override;
    STDMETHODIMP_(ULONG)  Release() override;

    STDMETHODIMP GetGUID(GUID* pguid) override;
    STDMETHODIMP GetDescription(BSTR* pbstrDesc) override;
    STDMETHODIMP GetAttributeInfo(TF_DISPLAYATTRIBUTE* pda) override;
    STDMETHODIMP SetAttributeInfo(const TF_DISPLAYATTRIBUTE* pda) override;
    STDMETHODIMP Reset() override;
private:
    LONG _cRef;
};

class CEnumDisplayAttributeInfo : public IEnumTfDisplayAttributeInfo
{
public:
    CEnumDisplayAttributeInfo();
    STDMETHODIMP          QueryInterface(REFIID riid, void** ppv) override;
    STDMETHODIMP_(ULONG)  AddRef() override;
    STDMETHODIMP_(ULONG)  Release() override;

    STDMETHODIMP Clone(IEnumTfDisplayAttributeInfo** ppEnum) override;
    STDMETHODIMP Next(ULONG ulCount, ITfDisplayAttributeInfo** rgInfo, ULONG* pcFetched) override;
    STDMETHODIMP Reset() override;
    STDMETHODIMP Skip(ULONG ulCount) override;
private:
    LONG _cRef;
    ULONG _index; // 0 또는 1 (속성 하나뿐)
};
