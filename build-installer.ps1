# build-installer.ps1 — 두 비트 DLL 을 묻은 단일 설치기(KPS9256-setup.exe)를 만든다.
#   먼저 KPS9256.sln 을 x64/x86 Release 로 빌드해 DLL 이 있어야 한다.
$ErrorActionPreference = "Stop"
$root = $PSScriptRoot
$vcvars = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

$x64 = Join-Path $root "build\x64\Release\KPS9256_x64.dll"
$x86 = Join-Path $root "build\Win32\Release\KPS9256_x86.dll"
foreach ($d in @($x64,$x86)) {
  if (!(Test-Path $d)) { Write-Error "DLL 없음: $d  — 먼저 KPS9256.sln 을 Release x64/x86 으로 빌드하세요."; exit 1 }
}

$inst = Join-Path $root "installer"
$out  = Join-Path $root "KPS9256-setup.exe"
Remove-Item $out -ErrorAction SilentlyContinue

# 임시 .bat 를 거치지 않고(한글 경로가 ASCII 로 깨지는 것을 피함) cmd 로 바로 넘긴다.
# /MT = 정적 CRT(런타임 자체 포함) → VC++ 재배포 없는 깨끗한 PC 에서도 실행됨.
$cl = "cl /nologo /utf-8 /O2 /MT /EHsc /DUNICODE /D_UNICODE Installer.cpp Installer.res " +
      "/Fe:`"$out`" /link /MANIFEST:NO /SUBSYSTEM:WINDOWS " +
      "shell32.lib ole32.lib advapi32.lib user32.lib"
$cmd = "call `"$vcvars`" >nul 2>&1 && cd /d `"$inst`" && " +
       "rc /nologo /fo Installer.res Installer.rc && $cl"
cmd.exe /c $cmd
$code = $LASTEXITCODE

Remove-Item (Join-Path $inst "Installer.obj"),(Join-Path $inst "Installer.res") -ErrorAction SilentlyContinue

if ($code -eq 0 -and (Test-Path $out)) {
  "OK -> {0}  ({1:N0} bytes)" -f $out, (Get-Item $out).Length
} else {
  Write-Error "설치기 빌드 실패 (exit $code)"; exit 1
}
