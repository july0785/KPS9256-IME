// Hangul.cpp — §4 두벌식 조합 자동기 구현
#include "Hangul.h"

namespace {

// ---- 색인표 (§4) ------------------------------------------------------------

// 초성 19: 호환자모 코드점
const wchar_t kCho[19] = {
    0x3131,0x3132,0x3134,0x3137,0x3138,0x3139,0x3141,0x3142,0x3143,0x3145,
    0x3146,0x3147,0x3148,0x3149,0x314A,0x314B,0x314C,0x314D,0x314E };
//   ㄱ    ㄲ    ㄴ    ㄷ    ㄸ    ㄹ    ㅁ    ㅂ    ㅃ    ㅅ
//   ㅆ    ㅇ    ㅈ    ㅉ    ㅊ    ㅋ    ㅌ    ㅍ    ㅎ

// 종성 28: 호환자모 코드점 (색인 0 = 없음)
const wchar_t kJong[28] = {
    0x0000,0x3131,0x3132,0x3133,0x3134,0x3135,0x3136,0x3137,0x3139,0x313A,
    0x313B,0x313C,0x313D,0x313E,0x313F,0x3140,0x3141,0x3142,0x3144,0x3145,
    0x3146,0x3147,0x3148,0x314A,0x314B,0x314C,0x314D,0x314E };
//   〔없음〕ㄱ  ㄲ   ㄳ   ㄴ   ㄵ   ㄶ   ㄷ   ㄹ   ㄺ
//   ㄻ   ㄼ   ㄽ   ㄾ   ㄿ   ㅀ   ㅁ   ㅂ   ㅄ   ㅅ
//   ㅆ   ㅇ   ㅈ   ㅊ   ㅋ   ㅌ   ㅍ   ㅎ

// 중성은 호환자모 U+314F..U+3163 이 색인 0..20 과 1:1 이므로 식으로 처리.
inline bool   IsVowel(wchar_t cp)      { return cp >= 0x314F && cp <= 0x3163; }
inline int    JungIndexOf(wchar_t cp)  { return IsVowel(cp) ? (cp - 0x314F) : -1; }
inline wchar_t JungChar(int idx)       { return (wchar_t)(0x314F + idx); }

int ChoIndexOf(wchar_t cp)
{
    for (int i = 0; i < 19; ++i) if (kCho[i] == cp) return i;
    return -1;
}

// 이 자음이 단독 종성으로 쓰일 수 있으면 그 종성 색인, 아니면 0(=불가, ㄸㅃㅉ).
int ConsonantJongIndex(wchar_t cp)
{
    for (int i = 1; i < 28; ++i) if (kJong[i] == cp) return i;
    return 0;
}

// 중성 결합(복모음): 되면 합성 색인, 아니면 -1.
int CombineVowel(int a, int b)
{
    switch (a) {
    case 8:  // ㅗ
        if (b == 0)  return 9;   // ㅗ+ㅏ → ㅘ
        if (b == 1)  return 10;  // ㅗ+ㅐ → ㅙ
        if (b == 20) return 11;  // ㅗ+ㅣ → ㅚ
        break;
    case 13: // ㅜ
        if (b == 4)  return 14;  // ㅜ+ㅓ → ㅝ
        if (b == 5)  return 15;  // ㅜ+ㅔ → ㅞ
        if (b == 20) return 16;  // ㅜ+ㅣ → ㅟ
        break;
    case 18: // ㅡ
        if (b == 20) return 19;  // ㅡ+ㅣ → ㅢ
        break;
    }
    return -1;
}

// 종성 결합(겹받침): 현 종성색인 + 들어온 단종성색인 → 합성 종성색인, 아니면 -1.
int CombineJong(int cur, int add)
{
    switch (cur) {
    case 1:  if (add == 19) return 3;  break;            // ㄱ+ㅅ → ㄳ
    case 4:  if (add == 22) return 5;                    // ㄴ+ㅈ → ㄵ
             if (add == 27) return 6;  break;            // ㄴ+ㅎ → ㄶ
    case 8:  if (add == 1)  return 9;                    // ㄹ+ㄱ → ㄺ
             if (add == 16) return 10;                   // ㄹ+ㅁ → ㄻ
             if (add == 17) return 11;                   // ㄹ+ㅂ → ㄼ
             if (add == 19) return 12;                   // ㄹ+ㅅ → ㄽ
             if (add == 25) return 13;                   // ㄹ+ㅌ → ㄾ
             if (add == 26) return 14;                   // ㄹ+ㅍ → ㄿ
             if (add == 27) return 15; break;            // ㄹ+ㅎ → ㅀ
    case 17: if (add == 19) return 18; break;            // ㅂ+ㅅ → ㅄ
    }
    return -1;
}

// 겹종성인가.
bool IsClusterJong(int j)
{
    switch (j) { case 3: case 5: case 6: case 9: case 10: case 11:
                 case 12: case 13: case 14: case 15: case 18: return true; }
    return false;
}

// 겹종성의 앞자음 종성색인(물림용).
int ClusterFrontJong(int j)
{
    switch (j) {
    case 3:  return 1;   // ㄳ → ㄱ
    case 5:  case 6:  return 4;   // ㄵ ㄶ → ㄴ
    case 9: case 10: case 11: case 12: case 13: case 14: case 15: return 8; // ㄹ*
    case 18: return 17;  // ㅄ → ㅂ
    }
    return 0;
}

// 단종성색인 → 그 자음의 초성색인(받침 넘김용).
int JongToCho(int j)
{
    switch (j) {
    case 1:  return 0;   // ㄱ
    case 2:  return 1;   // ㄲ
    case 4:  return 2;   // ㄴ
    case 7:  return 3;   // ㄷ
    case 8:  return 5;   // ㄹ
    case 16: return 6;   // ㅁ
    case 17: return 7;   // ㅂ
    case 19: return 9;   // ㅅ
    case 20: return 10;  // ㅆ
    case 21: return 11;  // ㅇ
    case 22: return 12;  // ㅈ
    case 23: return 14;  // ㅊ
    case 24: return 15;  // ㅋ
    case 25: return 16;  // ㅌ
    case 26: return 17;  // ㅍ
    case 27: return 18;  // ㅎ
    }
    return -1;
}

// 겹종성 분해: 앞자음은 종성으로 남기고(frontJong), 뒷자음은 새 음절 초성으로(backCho).
void SplitCluster(int j, int& frontJong, int& backCho)
{
    frontJong = ClusterFrontJong(j);
    switch (j) {
    case 3:  backCho = 9;  break;  // ㄳ → 남ㄱ / 초ㅅ
    case 5:  backCho = 12; break;  // ㄵ → 남ㄴ / 초ㅈ
    case 6:  backCho = 18; break;  // ㄶ → 남ㄴ / 초ㅎ
    case 9:  backCho = 0;  break;  // ㄺ → 남ㄹ / 초ㄱ
    case 10: backCho = 6;  break;  // ㄻ → 남ㄹ / 초ㅁ
    case 11: backCho = 7;  break;  // ㄼ → 남ㄹ / 초ㅂ
    case 12: backCho = 9;  break;  // ㄽ → 남ㄹ / 초ㅅ
    case 13: backCho = 16; break;  // ㄾ → 남ㄹ / 초ㅌ
    case 14: backCho = 17; break;  // ㄿ → 남ㄹ / 초ㅍ
    case 15: backCho = 18; break;  // ㅀ → 남ㄹ / 초ㅎ
    case 18: backCho = 9;  break;  // ㅄ → 남ㅂ / 초ㅅ
    default: backCho = -1; break;
    }
}

// 복모음인가.
bool IsCompoundVowel(int v)
{
    switch (v) { case 9: case 10: case 11: case 14: case 15: case 16: case 19: return true; }
    return false;
}

// 복모음의 앞모음(물림용).
int VowelFront(int v)
{
    switch (v) {
    case 9: case 10: case 11: return 8;   // ㅘ ㅙ ㅚ → ㅗ
    case 14: case 15: case 16: return 13; // ㅝ ㅞ ㅟ → ㅜ
    case 19: return 18;                   // ㅢ → ㅡ
    }
    return v;
}

inline void EmitSyllable(int cho, int jung, int jong, std::wstring& out)
{
    out += (wchar_t)(0xAC00 + (cho * 21 + jung) * 28 + jong);
}

} // namespace

// ---- 공개 멤버 --------------------------------------------------------------

void HangulComposer::Reset()
{
    _cho = -1; _jung = -1; _jong = 0;
}

void HangulComposer::_Emit(std::wstring& out) const
{
    if (_cho >= 0 && _jung >= 0)      EmitSyllable(_cho, _jung, _jong, out);
    else if (_cho >= 0)              out += kCho[_cho];          // 홑자음
    else if (_jung >= 0)             out += JungChar(_jung);     // 홑/복모음
    // 셋 다 비면 아무것도 안 그림
}

void HangulComposer::_Flush(std::wstring& out)
{
    _Emit(out);
    Reset();
}

void HangulComposer::Flush(std::wstring& out)
{
    _Flush(out);
}

std::wstring HangulComposer::Preedit() const
{
    std::wstring s;
    _Emit(s);
    return s;
}

void HangulComposer::Input(wchar_t jamo, std::wstring& out)
{
    if (IsVowel(jamo))            _InputVowel(jamo, out);
    else if (ChoIndexOf(jamo) >= 0) _InputConsonant(jamo, out);
    // 그 밖의(조합 대상이 아닌) 글자는 무시 — 호출측에서 거를 일이 없도록 방어.
}

void HangulComposer::_InputConsonant(wchar_t cp, std::wstring& out)
{
    const int cCho  = ChoIndexOf(cp);
    const int cJong = ConsonantJongIndex(cp); // 0 = 종성 불가(ㄸㅃㅉ)

    // 1. 빈 버퍼 → 초성.
    if (_cho < 0 && _jung < 0) { _cho = cCho; return; }

    // 2. 초성만 있음 → 그 초성 확정 후 새 초성.
    if (_jung < 0) { _Flush(out); _cho = cCho; return; }

    // 모음만 있던 경우(홑/복모음 단독) → 그 모음 확정 후 새 초성.
    if (_cho < 0) { _Flush(out); _cho = cCho; return; }

    // 초성+중성 있음.
    if (_jong == 0) {
        // 3. 종성 자리 빔 → 종성 가능하면 받침으로, 아니면 음절 확정 후 새 초성.
        if (cJong != 0) { _jong = cJong; return; }
        _Flush(out); _cho = cCho; return;
    } else {
        // 4. 종성 있음 → 겹받침 결합 시도, 안되면 음절 확정 후 새 초성.
        int comb = CombineJong(_jong, cJong);
        if (comb > 0) { _jong = comb; return; }
        _Flush(out); _cho = cCho; return;
    }
}

void HangulComposer::_InputVowel(wchar_t cp, std::wstring& out)
{
    const int v = JungIndexOf(cp);

    // 5/6. 중성이 아직 없음 → 중성 채움.
    //   (빈 버퍼면 홑모음 단독, 초성만 있으면 음절 완성)
    if (_jung < 0) { _jung = v; return; }

    // 8. 종성이 있음 → 받침 넘김.
    if (_jong > 0) {
        if (IsClusterJong(_jong)) {
            int frontJong, backCho;
            SplitCluster(_jong, frontJong, backCho);
            EmitSyllable(_cho, _jung, frontJong, out); // 앞자음 남긴 음절 확정
            _cho = backCho; _jung = v; _jong = 0;       // 뒷자음을 새 음절 초성으로
        } else {
            int backCho = JongToCho(_jong);
            EmitSyllable(_cho, _jung, 0, out);          // 받침 뗀 음절 확정
            _cho = backCho; _jung = v; _jong = 0;
        }
        return;
    }

    // 7. 종성 없음(초성±중성) → 복모음 결합 시도.
    int comb = CombineVowel(_jung, v);
    if (comb >= 0) { _jung = comb; return; }

    // 결합 안되면: 현 음절 확정 후 홑모음 단독으로.
    _Flush(out);
    _cho = -1; _jung = v; _jong = 0;
}

bool HangulComposer::Backspace()
{
    // 마지막 단계를 거꾸로.
    if (_jong > 0) {
        _jong = IsClusterJong(_jong) ? ClusterFrontJong(_jong) : 0;
        return true;
    }
    if (_jung >= 0) {
        if (IsCompoundVowel(_jung)) { _jung = VowelFront(_jung); return true; }
        _jung = -1;            // 초성만 남김(초성 없으면 빔)
        return true;
    }
    if (_cho >= 0) { _cho = -1; return true; }
    return false;              // 조합이 비었음 → 통과
}
