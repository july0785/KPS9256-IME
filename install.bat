@echo off
setlocal
rem ============================================================
rem  Joseon Gukgyu (KPS 9256) dubeolsik IME  --  INSTALL
rem  Registers both DLLs (x64 + x86) via regsvr32. Needs admin.
rem  Double-click: it self-elevates (UAC) and registers.
rem  (ASCII only on purpose: cmd cannot parse a batch file that
rem   contains Korean text. regsvr32 shows the Korean name itself.)
rem ============================================================

rem --- require elevation; relaunch as admin if not (service-free check) ---
whoami /groups | find "S-1-16-12288" >nul 2>&1
if %errorlevel% neq 0 (
    echo Requesting administrator rights... click "Yes" on the UAC prompt.
    powershell -NoProfile -ExecutionPolicy Bypass -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b
)

cd /d "%~dp0"
set "X64=%~dp0build\x64\Release\KPS9256_x64.dll"
set "X86=%~dp0build\Win32\Release\KPS9256_x86.dll"

if not exist "%X64%" (
    echo [ERROR] Built DLL not found:
    echo         "%X64%"
    echo.
    echo   This script registers DLLs that you BUILT yourself, so it only
    echo   works on a machine where the project was compiled.
    echo.
    echo   ^>^> To just install the IME, run  KPS9256-setup.exe  instead.
    echo      ^(It has the DLLs built in - no Visual Studio needed.^)
    echo.
    echo   Developers: build KPS9256.sln in Release x64/x86 first ^(see README.md^).
    pause
    exit /b 1
)

echo.
echo [1/2] Registering x64 DLL ...
regsvr32 "%X64%"

echo [2/2] Registering x86 DLL ...
if exist "%X86%" (
    regsvr32 "%X86%"
) else (
    echo  - x86 DLL not found, skipped ^(only needed for 32-bit apps^).
)

echo.
echo ============================================================
echo  Done. regsvr32 should have shown a success dialog.
echo.
echo  Now add the keyboard:
echo   Settings ^> Time ^& Language ^> Language ^& region
echo    ^> Korean ^> ... ^> Language options ^> Add a keyboard
echo    ^> choose the  KPS 9256  layout
echo      (listed in Korean as:  Joseon pyojun keyboard / Gukgyu 9256)
echo.
echo  Switch input layouts with  Win+Space.
echo  Toggle Hangul/English with  Right-Alt  or  Shift+Space.
echo ============================================================
pause
