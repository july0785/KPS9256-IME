@echo off
setlocal
rem ============================================================
rem  Joseon Gukgyu (KPS 9256) dubeolsik IME  --  UNINSTALL
rem  Unregisters both DLLs (regsvr32 /u). Needs admin.
rem  ASCII only on purpose (cmd cannot parse Korean in a .bat).
rem ============================================================

whoami /groups | find "S-1-16-12288" >nul 2>&1
if %errorlevel% neq 0 (
    echo Requesting administrator rights... click "Yes" on the UAC prompt.
    powershell -NoProfile -ExecutionPolicy Bypass -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b
)

cd /d "%~dp0"
set "X64=%~dp0build\x64\Release\KPS9256_x64.dll"
set "X86=%~dp0build\Win32\Release\KPS9256_x86.dll"

echo.
echo [1/2] Unregistering x64 DLL ...
if exist "%X64%" regsvr32 /u "%X64%"

echo [2/2] Unregistering x86 DLL ...
if exist "%X86%" regsvr32 /u "%X86%"

echo.
echo Done. If the layout still shows in the list, sign out and back in.
pause
