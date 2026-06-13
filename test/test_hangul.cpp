// test_hangul.cpp — §4 자동기 + §3 자판표 단독 검사 (TSF 불필요)
//
// 빌드/실행:  cl /nologo /utf-8 /EHsc /I..\KPS9256 test_hangul.cpp ..\KPS9256\Hangul.cpp && test_hangul.exe
//
// §9 검사 항목을 자모렬로 친 것으로 흉내내어 결과를 대조한다.
#include <cstdio>
#include <string>
#include "Hangul.h"
#include "KeyMap.h"

static int g_pass = 0, g_fail = 0;

// 자모 호환문자렬을 순서대로 넣고, 확정+마지막 조합중까지 합친 결과를 돌려준다.
static std::wstring TypeJamo(const std::wstring& jamos)
{
    HangulComposer hc;
    std::wstring out;
    for (wchar_t j : jamos) hc.Input(j, out);
    out += hc.Preedit();          // 아직 조합중인 음절도 포함
    return out;
}

static void Check(const char* name, const std::wstring& got, const std::wstring& want)
{
    bool ok = (got == want);
    printf(ok ? "  PASS  %-22s\n" : "  FAIL  %-22s  got=[", name);
    if (!ok) {
        for (wchar_t c : got)  printf("U+%04X ", (unsigned)c);
        printf("] want=[");
        for (wchar_t c : want) printf("U+%04X ", (unsigned)c);
        printf("]\n");
    }
    (ok ? g_pass : g_fail)++;
}

int main()
{
    printf("== KPS9256 자동기 검사 ==\n");

    // 기본
    Check("안녕하십니까", TypeJamo(L"ㅇㅏㄴㄴㅕㅇㅎㅏㅅㅣㅂㄴㅣㄲㅏ"), L"안녕하십니까");

    // 복모음 (ㅇ 붙여서 음절로)
    Check("와", TypeJamo(L"ㅇㅗㅏ"), L"와");
    Check("워", TypeJamo(L"ㅇㅜㅓ"), L"워");
    Check("외", TypeJamo(L"ㅇㅗㅣ"), L"외");
    Check("위", TypeJamo(L"ㅇㅜㅣ"), L"위");
    Check("의", TypeJamo(L"ㅇㅡㅣ"), L"의");
    // 복모음 단독 표시(호환자모)
    Check("ㅘ단독", TypeJamo(L"ㅗㅏ"), L"ㅘ");

    // 겹받침
    Check("닭", TypeJamo(L"ㄷㅏㄹㄱ"), L"닭");
    Check("값", TypeJamo(L"ㄱㅏㅂㅅ"), L"값");
    Check("앉", TypeJamo(L"ㅇㅏㄴㅈ"), L"앉");
    Check("많", TypeJamo(L"ㅁㅏㄴㅎ"), L"많");
    Check("읽", TypeJamo(L"ㅇㅣㄹㄱ"), L"읽");

    // 받침 넘김
    Check("가나(넘김)",   TypeJamo(L"ㄱㅏㄴㅏ"),   L"가나");
    Check("달가(겹넘김)", TypeJamo(L"ㄷㅏㄹㄱㅏ"), L"달가");
    // 자음 직접 입력 → 넘김 안됨
    Check("막아", TypeJamo(L"ㅁㅏㄱㅇㅏ"), L"막아");

    // 된소리(Shift)
    Check("따", TypeJamo(L"ㄸㅏ"), L"따");
    Check("까", TypeJamo(L"ㄲㅏ"), L"까");
    Check("짜", TypeJamo(L"ㅉㅏ"), L"짜");
    Check("빠", TypeJamo(L"ㅃㅏ"), L"빠");
    Check("싸", TypeJamo(L"ㅆㅏ"), L"싸");

    // 물림(Backspace) — 음절 단계대로
    {
        HangulComposer hc; std::wstring out;
        for (wchar_t j : std::wstring(L"ㄷㅏㄹㄱ")) hc.Input(j, out); // 닭
        Check("닭→물림→달", (hc.Backspace(), hc.Preedit()), L"달");  // 겹종성→앞자음
        hc.Backspace();                                              // 종성 제거
        Check("→물림→다",  hc.Preedit(), L"다");
        hc.Backspace();                                              // 중성 제거
        Check("→물림→ㄷ",  hc.Preedit(), L"ㄷ");
        bool more = hc.Backspace();                                  // 초성 제거 → 빔
        Check("→물림→빔",  hc.Preedit(), L"");
        Check("→통과여부",  more ? L"x" : L"통과", L"x"); // 마지막엔 true(되돌림), 다음번이 false
        bool pass = hc.Backspace();
        Check("빈조합 통과", pass ? L"x" : L"통과", L"통과");
    }

    // §3 자판표 표본 (VK→자모)
    Check("키S(ㄱ)",     std::wstring(1, KpsMapKey('S', false)), L"ㄱ");
    Check("키S+Shift(ㄲ)", std::wstring(1, KpsMapKey('S', true)),  L"ㄲ");
    Check("키J(ㅏ)",     std::wstring(1, KpsMapKey('J', false)), L"ㅏ");
    Check("키Q(ㅂ)",     std::wstring(1, KpsMapKey('Q', false)), L"ㅂ");
    Check("키Q+Shift(ㅃ)", std::wstring(1, KpsMapKey('Q', true)),  L"ㅃ");
    Check("키T(ㅎ)",     std::wstring(1, KpsMapKey('T', false)), L"ㅎ");
    // Shift 인데 쌍자모 없는 키 → 기본 자모(영문 아님)
    Check("키W+Shift(ㅁ)", std::wstring(1, KpsMapKey('W', true)), L"ㅁ");
    Check("키T+Shift(ㅎ)", std::wstring(1, KpsMapKey('T', true)), L"ㅎ");
    Check("키J+Shift(ㅏ)", std::wstring(1, KpsMapKey('J', true)), L"ㅏ");
    Check("키F+Shift(ㄴ)", std::wstring(1, KpsMapKey('F', true)), L"ㄴ");

    // 한 음절을 자판표로 끝까지: S(ㄱ) J(ㅏ) → 가
    {
        HangulComposer hc; std::wstring out;
        hc.Input(KpsMapKey('S', false), out);
        hc.Input(KpsMapKey('J', false), out);
        Check("자판 S+J → 가", out + hc.Preedit(), L"가");
    }

    printf("\n결과: PASS=%d  FAIL=%d\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
