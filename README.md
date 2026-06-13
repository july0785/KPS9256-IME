# 조선 표준건반 배렬 (국규 9256) 두벌식 입력기

조선민주주의인민공화국 국가규격 **KPS 9256** 두벌식 자판 배렬로 조선글을 입력하는
**Windows 입력기(TSF TIP)** 입니다. 자모를 조합해 음절을 만드는 일에만 집중하며,
한자 변환·후보창·문구 예측·사전 같은 기능은 들어 있지 않습니다.

- **대상**: Windows 10 / 11 (64비트·32비트 모두 지원)
- **방식**: 두벌식, 자모 조합 전용
- **배렬**: KPS 9256 (남측 KS X 5002 와는 자음·모음 배치가 다릅니다)

## 설치

[Releases](https://github.com/july0785/KPS9256-IME/releases) 에서 `KPS9256-setup.exe`
를 받아 실행하면 됩니다. 관리자 권한을 한 번 묻고(UAC), 64비트·32비트 입력기를 모두
등록합니다.

설치한 뒤 **《설정 → 시간 및 언어 → 언어 및 지역 → 한국어 → 옵션 → 키보드 추가》**
에서 **「조선 표준건반 배렬 (국규 9256)」** 을 선택하면 입력기 목록에 나타납니다.

- 입력기 전환: **Win + Space**
- 한글 ↔ 영문: **오른쪽 Alt** 또는 **Shift + Space**

제거는 《설정 → 앱 → 설치된 앱》 에서 하거나 `C:\Program Files\KPS9256\uninstall.exe`
를 실행하면 됩니다.

## 기능

- KPS 9256 두벌식 자판 배렬
- 복모음 조합 — ㅘ ㅙ ㅚ ㅝ ㅞ ㅟ ㅢ
- 겹받침 조합 — ㄳ ㄵ ㄶ ㄺ ㄻ ㄼ ㄽ ㄾ ㄿ ㅀ ㅄ
- 받침 넘김(연음) — `ㄱㅏㄴㅏ` → `가나`, 겹받침은 뒤 자음만 넘어갑니다 (`ㄷㅏㄹㄱㅏ` → `달가`)
- 된소리(Shift) — ㅃ ㄸ ㅉ ㄲ ㅆ, 그리고 ㅒ ㅖ
- 조합 중인 음절은 밑줄로 표시되며, Backspace 로 조합 단계를 하나씩 되돌립니다

## 소스에서 빌드하기

Visual Studio 2022(「C++를 사용한 데스크톱 개발」 워크로드)와 Windows 10/11 SDK 가
필요합니다.

```powershell
# 입력기 DLL (x64 · x86)
$ms = "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
& $ms .\KPS9256.sln /p:Configuration=Release /p:Platform=x64
& $ms .\KPS9256.sln /p:Configuration=Release /p:Platform=x86

# 두 DLL 을 묶어 단일 설치기(KPS9256-setup.exe) 생성
.\build-installer.ps1
```

설치기 없이 빌드한 DLL 을 바로 등록하려면 관리자 권한 PowerShell 에서 다음과 같이
합니다(또는 `install.bat` / `uninstall.bat` 를 더블클릭).

```powershell
regsvr32 .\build\x64\Release\KPS9256_x64.dll
regsvr32 .\build\Win32\Release\KPS9256_x86.dll
```

## 시험

자모 조합 논리는 Windows 없이도 단독으로 시험할 수 있습니다.

```powershell
cd test
cl /nologo /utf-8 /EHsc /I ..\KPS9256 test_hangul.cpp ..\KPS9256\Hangul.cpp
.\test_hangul.exe
```

기본 입력·복모음·겹받침·받침 넘김·된소리·Backspace 등 37개 항목을 확인합니다.
(`cl` 은 "x64 Native Tools Command Prompt" 또는 vcvars 적재 후 사용합니다.)

## 구성

```
KPS9256/        입력기 본체 (TSF TIP)
  Hangul.*        두벌식 조합 자동기
  KeyMap.h        자판 배렬표
  TextService.*   활성화 · 한/영 전환
  KeyHandler.*    키 처리 · 조합 표시
  Register.cpp    입력기 등록
installer/      단일 설치기 소스
test/           조합 자동기 시험
```

## 한/영 전환 방식

한글·영문 상태는 Windows TSF 의 변환모드(input-mode conversion) 칸으로 관리합니다.
토글 키(오른쪽 Alt / Shift+Space)를 누르면 이 칸 값을 뒤집으며, 상태를 따로 세지 않으므로
다른 응용과 어긋날 일이 없습니다.

## 라이선스

이 프로그램은 **GNU General Public License v3.0 (GPL-3.0)** 으로 배포됩니다.
전문은 [`LICENSE`](LICENSE) 를 참고하세요. 자유롭게 사용·수정·재배포할 수 있으나,
파생물 역시 같은 GPL-3.0 으로 소스를 공개해야 합니다.

```
조선 표준건반 배렬 (국규 9256) 두벌식 입력기
Copyright (C) 2026 JULY

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
```
