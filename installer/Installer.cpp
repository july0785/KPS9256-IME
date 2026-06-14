// Installer.cpp — 조선 표준건반 배렬 (국규 9256) 자체 설치기
//
// 동작:
//   설치   : Program Files\KPS9256 에 두 DLL 을 풀고, 64/32비트 regsvr32 로 등록,
//            《프로그램 추가/제거》 항목 생성. 설치기 자신을 uninstall.exe 로 복사.
//   제거   : setup.exe /uninstall  (또는 복사된 uninstall.exe) — 등록 해제 후 삭제.
//
// 관리자 권한은 매니페스트(requireAdministrator)로 자동 승격된다.
#include <windows.h>
#include <shlobj.h>
#include <string>
#include "resource.h"

static const wchar_t* kAppName  = L"조선 표준건반 배렬 (국규 9256)";
static const wchar_t* kInstDir  = L"KPS9256";        // Program Files 아래 폴더
static const wchar_t* kArpKey   =
    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\KPS9256_KPS9256IME";

// ─── 도움이 ──────────────────────────────────────────────────────────────────
static std::wstring ProgramFiles()
{
    PWSTR p = nullptr; std::wstring r;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_ProgramFiles, 0, nullptr, &p)) && p) { r = p; }
    if (p) CoTaskMemFree(p);
    return r;
}

static std::wstring SelfPath()
{
    wchar_t buf[MAX_PATH]; GetModuleFileNameW(nullptr, buf, MAX_PATH); return buf;
}

static std::wstring ParentDir(const std::wstring& path)
{
    size_t i = path.find_last_of(L"\\/");
    return (i == std::wstring::npos) ? path : path.substr(0, i);
}

static bool ExtractResource(WORD id, const std::wstring& path)
{
    HRSRC h = FindResourceW(nullptr, MAKEINTRESOURCEW(id), RT_RCDATA);
    if (!h) return false;
    HGLOBAL g = LoadResource(nullptr, h);
    if (!g) return false;
    void* data = LockResource(g);
    DWORD  size = SizeofResource(nullptr, h);
    if (!data || !size) return false;

    // 대상이 이미 있고 로드되어 잠겨 있으면(재설치) 그대로는 못 덮어쓴다.
    // 잠긴 DLL 도 이름 바꾸기는 되므로, 옆으로 치워 경로를 비운 뒤 새로 쓴다.
    // 치운 옛 파일은 다음 재시작 때 정리되도록 예약한다.
    if (GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES) {
        std::wstring aside = path + L".old" + std::to_wstring(GetTickCount64());
        if (MoveFileExW(path.c_str(), aside.c_str(), MOVEFILE_REPLACE_EXISTING))
            MoveFileExW(aside.c_str(), nullptr, MOVEFILE_DELAY_UNTIL_REBOOT);
    }

    HANDLE f = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE) return false;
    DWORD wrote = 0;
    BOOL ok = WriteFile(f, data, size, &wrote, nullptr);
    CloseHandle(f);
    return ok && wrote == size;
}

static DWORD RunWait(const std::wstring& exe, const std::wstring& args)
{
    SHELLEXECUTEINFOW si = { sizeof(si) };
    si.fMask  = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NO_CONSOLE;
    si.lpVerb = L"open";
    si.lpFile = exe.c_str();
    si.lpParameters = args.c_str();
    si.nShow  = SW_HIDE;
    if (!ShellExecuteExW(&si) || !si.hProcess) return (DWORD)-1;
    WaitForSingleObject(si.hProcess, INFINITE);
    DWORD code = (DWORD)-1;
    GetExitCodeProcess(si.hProcess, &code);
    CloseHandle(si.hProcess);
    return code;
}

static std::wstring SysRegsvr()   { wchar_t b[MAX_PATH]; GetSystemDirectoryW(b, MAX_PATH);      return std::wstring(b) + L"\\regsvr32.exe"; }
static std::wstring WowRegsvr()    { wchar_t b[MAX_PATH]; UINT n = GetSystemWow64DirectoryW(b, MAX_PATH); return n ? std::wstring(b) + L"\\regsvr32.exe" : L""; }
static std::wstring Q(const std::wstring& s) { return L"\"" + s + L"\""; }

// ─── 설치 ────────────────────────────────────────────────────────────────────
static int DoInstall()
{
    if (MessageBoxW(nullptr,
            L"조선 표준건반 배렬 (국규 9256) 두벌식 입력기를 설치합니다.\n\n계속하시겠습니까?",
            kAppName, MB_ICONQUESTION | MB_YESNO) != IDYES)
        return 1;

    std::wstring dir = ProgramFiles() + L"\\" + kInstDir;
    SHCreateDirectoryExW(nullptr, dir.c_str(), nullptr);

    std::wstring x64 = dir + L"\\KPS9256_x64.dll";
    std::wstring x86 = dir + L"\\KPS9256_x86.dll";
    std::wstring uninst = dir + L"\\uninstall.exe";

    if (!ExtractResource(IDR_DLL_X64, x64) || !ExtractResource(IDR_DLL_X86, x86)) {
        MessageBoxW(nullptr, L"파일을 푸는 데 실패했습니다.", kAppName, MB_ICONERROR);
        return 2;
    }
    CopyFileW(SelfPath().c_str(), uninst.c_str(), FALSE);

    // 등록: 64비트는 System32\regsvr32, 32비트는 SysWOW64\regsvr32
    DWORD r64 = RunWait(SysRegsvr(), L"/s " + Q(x64));
    std::wstring wow = WowRegsvr();
    DWORD r86 = wow.empty() ? 0 : RunWait(wow, L"/s " + Q(x86));

    if (r64 != 0) {
        MessageBoxW(nullptr, L"입력기 등록(64비트)에 실패했습니다.", kAppName, MB_ICONERROR);
        return 3;
    }

    // 《프로그램 추가/제거》 항목
    HKEY k = nullptr;
    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, kArpKey, 0, nullptr, 0, KEY_WRITE, nullptr, &k, nullptr) == ERROR_SUCCESS) {
        auto S = [&](const wchar_t* n, const std::wstring& v) {
            RegSetValueExW(k, n, 0, REG_SZ, (const BYTE*)v.c_str(), (DWORD)((v.size() + 1) * sizeof(wchar_t)));
        };
        DWORD one = 1;
        S(L"DisplayName", kAppName);
        S(L"DisplayVersion", L"1.0.0");
        S(L"Publisher", L"KPS9256");
        S(L"InstallLocation", dir);
        S(L"DisplayIcon", x64 + L",0");
        S(L"UninstallString", Q(uninst) + L" /uninstall");
        RegSetValueExW(k, L"NoModify", 0, REG_DWORD, (const BYTE*)&one, sizeof(one));
        RegSetValueExW(k, L"NoRepair", 0, REG_DWORD, (const BYTE*)&one, sizeof(one));
        RegCloseKey(k);
    }

    std::wstring msg = L"설치가 끝났습니다.\n\n";
    msg += L"《설정 → 시간 및 언어 → 언어 및 지역 → 한국어 → … → 옵션\n";
    msg += L" → 키보드 추가》 에서 \"조선 표준건반 배렬 (국규 9256)\" 을 고르세요.\n\n";
    msg += L"전환: Win+Space  ·  한/영: 오른쪽 Alt 또는 Shift+Space";
    if (!wow.empty() && r86 != 0)
        msg += L"\n\n(주의: 32비트 등록은 실패 — 32비트 응용에선 안 뜰 수 있습니다.)";
    MessageBoxW(nullptr, msg.c_str(), kAppName, MB_ICONINFORMATION);
    return 0;
}

// ─── 제거 ────────────────────────────────────────────────────────────────────
static int DoUninstall()
{
    std::wstring dir = ParentDir(SelfPath());     // uninstall.exe 가 놓인 폴더
    std::wstring x64 = dir + L"\\KPS9256_x64.dll";
    std::wstring x86 = dir + L"\\KPS9256_x86.dll";

    RunWait(SysRegsvr(), L"/u /s " + Q(x64));
    std::wstring wow = WowRegsvr();
    if (!wow.empty()) RunWait(wow, L"/u /s " + Q(x86));

    // 파일 삭제 (로드중이면 재시작 때 정리되게 예약)
    if (!DeleteFileW(x64.c_str())) MoveFileExW(x64.c_str(), nullptr, MOVEFILE_DELAY_UNTIL_REBOOT);
    if (!DeleteFileW(x86.c_str())) MoveFileExW(x86.c_str(), nullptr, MOVEFILE_DELAY_UNTIL_REBOOT);

    RegDeleteKeyW(HKEY_LOCAL_MACHINE, kArpKey);

    // 실행중인 uninstall.exe 자신과 폴더는 재시작 때 정리.
    MoveFileExW(SelfPath().c_str(), nullptr, MOVEFILE_DELAY_UNTIL_REBOOT);
    RemoveDirectoryW(dir.c_str());   // 비어 있으면 즉시, 아니면 무시

    MessageBoxW(nullptr,
        L"제거가 끝났습니다.\n목록에 남아 보이면 로그아웃/로그인 하세요.",
        kAppName, MB_ICONINFORMATION);
    return 0;
}

// ─── 진입점 ──────────────────────────────────────────────────────────────────
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR lpCmdLine, int)
{
    std::wstring cmd = lpCmdLine ? lpCmdLine : L"";
    bool uninstall = (cmd.find(L"/uninstall") != std::wstring::npos) ||
                     (cmd.find(L"/u") != std::wstring::npos);
    return uninstall ? DoUninstall() : DoInstall();
}
