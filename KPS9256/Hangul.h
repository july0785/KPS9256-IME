// Hangul.h — KPS 9256 두벌식 조합 자동기 (순수 산법, 윈도 의존 없음)
//
// 작업지령서 §4 를 그대로 구현한다. 유니코드 한글 자모 순서를 색인으로 쓴다.
//   초성 19, 중성 21, 종성 28(0=없음).
//   음절 = 0xAC00 + (cho*21 + jung)*28 + jong
//
// 이 파일은 TSF/COM 과 무관하다. test_hangul.cpp 로 단독 검사할 수 있다.
#pragma once
#include <string>

class HangulComposer
{
public:
    // 자모 한 글자(호환자모 U+3131..U+3163)를 넣는다.
    // 확정(commit)되는 글자가 있으면 out 뒤에 덧붙인다. 현재 조합중인 음절은
    // Preedit() 로 얻는다.
    void Input(wchar_t jamo, std::wstring& out);

    // 물림(Backspace) 한 단계. 한 단계라도 되돌렸으면 true,
    // 조합이 비어 있어(되돌릴게 없어) 키를 통과시켜야 하면 false.
    bool Backspace();

    // 현재 조합중 음절을 확정해 out 에 덧붙이고 버퍼를 비운다.
    // (공백·Enter·초점상실·비활성화 때 부른다.)
    void Flush(std::wstring& out);

    // 버퍼를 그냥 비운다(확정 안 함).
    void Reset();

    // 조합중인가(버퍼에 무엇이라도 있는가).
    bool IsComposing() const { return !(_cho < 0 && _jung < 0); }

    // 현재 조합중 음절의 표시 문자렬(밑줄 preedit). 비었으면 빈 문자렬.
    std::wstring Preedit() const;

private:
    int _cho  = -1; // 초성 색인 0..18, -1=없음
    int _jung = -1; // 중성 색인 0..20, -1=없음
    int _jong = 0;  //  종성 색인 0..27, 0=없음

    void _InputConsonant(wchar_t cp, std::wstring& out);
    void _InputVowel(wchar_t cp, std::wstring& out);
    void _Flush(std::wstring& out);          // 현재 상태를 out 에 그리고 비움
    void _Emit(std::wstring& out) const;     // 현재 상태를 out 에 그림(비우지 않음)
};
