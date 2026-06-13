// DisplayAttribute.cpp — 조합 밑줄 표시속성 (preedit underline)
#include "KPS9256.h"

// ─── CDisplayAttributeInfo ───────────────────────────────────────────────────
CDisplayAttributeInfo::CDisplayAttributeInfo() : _cRef(1) {}

STDMETHODIMP CDisplayAttributeInfo::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv) return E_INVALIDARG;
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfDisplayAttributeInfo)) {
        *ppv = static_cast<ITfDisplayAttributeInfo*>(this);
        AddRef();
        return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}
STDMETHODIMP_(ULONG) CDisplayAttributeInfo::AddRef()  { return ++_cRef; }
STDMETHODIMP_(ULONG) CDisplayAttributeInfo::Release()
{
    LONG c = --_cRef;
    if (c == 0) delete this;
    return c;
}

STDMETHODIMP CDisplayAttributeInfo::GetGUID(GUID* pguid)
{
    if (!pguid) return E_INVALIDARG;
    *pguid = c_guidDisplayAttribute;
    return S_OK;
}

STDMETHODIMP CDisplayAttributeInfo::GetDescription(BSTR* pbstrDesc)
{
    if (!pbstrDesc) return E_INVALIDARG;
    *pbstrDesc = SysAllocString(L"KPS9256 Composition");
    return *pbstrDesc ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP CDisplayAttributeInfo::GetAttributeInfo(TF_DISPLAYATTRIBUTE* pda)
{
    if (!pda) return E_INVALIDARG;
    ZeroMemory(pda, sizeof(*pda));
    pda->crText.type = TF_CT_NONE;
    pda->crBk.type   = TF_CT_NONE;
    pda->crLine.type = TF_CT_NONE;
    pda->lsStyle     = TF_LS_SOLID;   // 실선 밑줄
    pda->fBoldLine   = FALSE;
    pda->bAttr       = TF_ATTR_INPUT; // 입력(조합) 중
    return S_OK;
}

STDMETHODIMP CDisplayAttributeInfo::SetAttributeInfo(const TF_DISPLAYATTRIBUTE* /*pda*/)
{
    return E_NOTIMPL; // 고정 속성
}

STDMETHODIMP CDisplayAttributeInfo::Reset()
{
    TF_DISPLAYATTRIBUTE da;
    return GetAttributeInfo(&da);
}

// ─── CEnumDisplayAttributeInfo (속성 하나) ───────────────────────────────────
CEnumDisplayAttributeInfo::CEnumDisplayAttributeInfo() : _cRef(1), _index(0) {}

STDMETHODIMP CEnumDisplayAttributeInfo::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv) return E_INVALIDARG;
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IEnumTfDisplayAttributeInfo)) {
        *ppv = static_cast<IEnumTfDisplayAttributeInfo*>(this);
        AddRef();
        return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}
STDMETHODIMP_(ULONG) CEnumDisplayAttributeInfo::AddRef()  { return ++_cRef; }
STDMETHODIMP_(ULONG) CEnumDisplayAttributeInfo::Release()
{
    LONG c = --_cRef;
    if (c == 0) delete this;
    return c;
}

STDMETHODIMP CEnumDisplayAttributeInfo::Clone(IEnumTfDisplayAttributeInfo** ppEnum)
{
    if (!ppEnum) return E_INVALIDARG;
    CEnumDisplayAttributeInfo* p = new (std::nothrow) CEnumDisplayAttributeInfo();
    if (!p) return E_OUTOFMEMORY;
    p->_index = _index;
    *ppEnum = p;
    return S_OK;
}

STDMETHODIMP CEnumDisplayAttributeInfo::Next(ULONG ulCount, ITfDisplayAttributeInfo** rgInfo, ULONG* pcFetched)
{
    ULONG fetched = 0;
    while (fetched < ulCount && _index < 1) {
        ITfDisplayAttributeInfo* p = new (std::nothrow) CDisplayAttributeInfo();
        if (!p) break;
        rgInfo[fetched++] = p;
        _index++;
    }
    if (pcFetched) *pcFetched = fetched;
    return (fetched == ulCount) ? S_OK : S_FALSE;
}

STDMETHODIMP CEnumDisplayAttributeInfo::Reset() { _index = 0; return S_OK; }

STDMETHODIMP CEnumDisplayAttributeInfo::Skip(ULONG ulCount)
{
    _index += ulCount;
    if (_index > 1) _index = 1;
    return S_OK;
}
