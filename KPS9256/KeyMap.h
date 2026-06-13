// KeyMap.h — §3 자판 배렬표 (정본). US QWERTY 물리건 → 호환자모.
//
// VK 코드(US 배치 기준 'A'..'Z')로 대응한다. shift=true 면 윗줄(된소리/ㅒㅖ).
// 단, 윗줄에 쌍자모가 없는 키는 Shift 를 눌러도 **기본 자모**가 나온다
// (표준 두벌식 동작: Shift 는 ㅃㄸㅉㄲㅆㅒㅖ 일곱 곳에서만 의미가 있다).
// 대응 없으면 0 을 돌려준다. 순수 함수 — TSF 와 test 양쪽이 쓴다.
//
// !!! 이 표는 의뢰자가 사진에서 확인한 정본이다. 임의로 고치지 말것 (§3). !!!
#pragma once

inline wchar_t KpsMapKey(unsigned vk, bool shift)
{
    wchar_t base = 0, sh = 0;
    switch (vk) {
    // 윗줄 QWERTYUIOP
    case 'Q': base = 0x3142; sh = 0x3143; break; // ㅂ  ㅃ
    case 'W': base = 0x3141;              break; // ㅁ
    case 'E': base = 0x3137; sh = 0x3138; break; // ㄷ  ㄸ
    case 'R': base = 0x3139;              break; // ㄹ
    case 'T': base = 0x314E;              break; // ㅎ
    case 'Y': base = 0x3155;              break; // ㅕ
    case 'U': base = 0x315C;              break; // ㅜ
    case 'I': base = 0x3153;              break; // ㅓ
    case 'O': base = 0x3150; sh = 0x3152; break; // ㅐ  ㅒ
    case 'P': base = 0x3154; sh = 0x3156; break; // ㅔ  ㅖ
    // 가운뎃줄 ASDFGHJKL
    case 'A': base = 0x3148; sh = 0x3149; break; // ㅈ  ㅉ
    case 'S': base = 0x3131; sh = 0x3132; break; // ㄱ  ㄲ
    case 'D': base = 0x3147;              break; // ㅇ
    case 'F': base = 0x3134;              break; // ㄴ
    case 'G': base = 0x3145; sh = 0x3146; break; // ㅅ  ㅆ
    case 'H': base = 0x3157;              break; // ㅗ
    case 'J': base = 0x314F;              break; // ㅏ
    case 'K': base = 0x3163;              break; // ㅣ
    case 'L': base = 0x3161;              break; // ㅡ
    // 아랫줄 ZXCVBNM
    case 'Z': base = 0x314B;              break; // ㅋ
    case 'X': base = 0x314C;              break; // ㅌ
    case 'C': base = 0x314A;              break; // ㅊ
    case 'V': base = 0x314D;              break; // ㅍ
    case 'B': base = 0x3160;              break; // ㅠ
    case 'N': base = 0x315B;              break; // ㅛ
    case 'M': base = 0x3151;              break; // ㅑ
    default:  return 0;
    }
    return (shift && sh) ? sh : base; // 쌍자모 없으면 Shift 무시 → 기본 자모
}
