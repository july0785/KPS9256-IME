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
class CLangBarButton;

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

    // 언어바 단추(한/영 토글)가 부른다
    bool LbIsHangul() { return _IsHangulMode(); }
    void LbToggle()   { _ToggleHangulMode(); }

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
    BOOL _InitLangBarButton();          // 한/영 토글 단추(작업표시줄)
    void _UninitLangBarButton();
    void _UpdateLangBarButton();        // 모드 바뀌면 아이콘/글자 갱신

    // 한/영 상태 — 절대 자체 카운터로 추적하지 않는다 (§5). 칸 값을 읽고 쓴다.
    bool _IsHangulMode();
    void _ToggleHangulMode();

    // 키 → 처리
    void _HandleJamo(ITfContext* pic, wchar_t jamo);
    void _HandleBackspace(ITfContext* pic);
    void _FinalizeComposition(ITfContext* pic);            // 현 음절 확정
    void _InjectReplace(const std::wstring& commit, const std::wstring& preedit); // CUAS 키주입
    void _RequestApply(ITfContext* pic,
                       const std::wstring& commit, const std::wstring& preedit);

    // 조합(편집세션 안에서만)
    //   글자를 먼저 넣고 그 위에 조합을 건다(CUAS 호환). 이미 조합중이면 글자만 갱신.
    void _StartCompositionWithText(TfEditCookie ec, ITfContext* pic, const std::wstring& text);
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

    // CUAS(실행창·탐색기 이름칸) 우회 — TSF 조합 대신 키 주입
    bool         _cuasMode;     // 이 문맥은 조합이 매 글자 종료됨 → 키 주입 사용
    std::wstring _cuasDoc;      // 문서에 들어가 있는 현재 미리편집 글자(다음에 지울 분량)

    ITfCompartment* _pConvMode;          // 변환모드 칸
    DWORD           _dwConvModeCookie;

    TfGuidAtom      _gaDisplayAttribute;
    CLangBarButton* _pLangBarButton;     // 한/영 토글 단추

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

// ─── 한/영 토글 단추 (작업표시줄 입력 표시기) ────────────────────────────────
//   GUID_LBI_INPUTMODE 입력모드 단추로 등록한다. 한글이면 《조》, 영문이면 《A》.
//   누르면 변환모드 칸을 뒤집어 한/영을 전환한다.
class CLangBarButton : public ITfLangBarItemButton, public ITfSource
{
public:
    CLangBarButton(CKPS9256TextService* tip);
    ~CLangBarButton();

    // IUnknown
    STDMETHODIMP          QueryInterface(REFIID riid, void** ppv) override;
    STDMETHODIMP_(ULONG)  AddRef() override;
    STDMETHODIMP_(ULONG)  Release() override;

    // ITfLangBarItem
    STDMETHODIMP GetInfo(TF_LANGBARITEMINFO* pInfo) override;
    STDMETHODIMP GetStatus(DWORD* pdwStatus) override;
    STDMETHODIMP Show(BOOL fShow) override;
    STDMETHODIMP GetTooltipString(BSTR* pbstrToolTip) override;

    // ITfLangBarItemButton
    STDMETHODIMP OnClick(TfLBIClick click, POINT pt, const RECT* prcArea) override;
    STDMETHODIMP InitMenu(ITfMenu* pMenu) override;
    STDMETHODIMP OnMenuSelect(UINT wID) override;
    STDMETHODIMP GetIcon(HICON* phIcon) override;
    STDMETHODIMP GetText(BSTR* pbstrText) override;

    // ITfSource
    STDMETHODIMP AdviseSink(REFIID riid, IUnknown* punk, DWORD* pdwCookie) override;
    STDMETHODIMP UnadviseSink(DWORD dwCookie) override;

    void NotifyUpdate();                 // 모드 바뀜 → 언어바에 갱신 통지

private:
    LONG                 _cRef;
    CKPS9256TextService* _pTip;          // 약한 참조(소유자가 살아있는 동안만 사용)
    ITfLangBarItemSink*  _pSink;
};
