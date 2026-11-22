@echo off

echo ========================================================
echo       Building Anti-Goon VPN DNS Shield (C++)
echo ========================================================

where cl.exe >nul 2>nul
if errorlevel 1 goto init_vs
goto compile

:init_vs
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
    goto compile
)
if exist "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
    goto compile
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64
    goto compile
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" x64
    goto compile
)

echo [ERROR] Visual Studio C++ compiler not found!
exit /b 1

:compile
cd /d "%~dp0"

echo Compiling resources (icon, version, administrator manifest)...
cd res
rc.exe /nologo /fo app.res app.rc
if errorlevel 1 (
    echo [ERROR] Resource compilation failed!
    cd ..
    exit /b 1
)
cd ..

echo Compiling standalone C++ source files (/MT static runtime for 100%% portability)...
cl.exe /nologo /O2 /MT /W3 /EHsc /std:c++17 /DUNICODE /D_UNICODE ^
    /I src ^
    src\main.cpp ^
    src\dns_manager.cpp ^
    src\ui_renderer.cpp ^
    res\app.res ^
    /link /SUBSYSTEM:WINDOWS /OUT:AntiGoon.exe ^
    user32.lib gdi32.lib gdiplus.lib iphlpapi.lib ws2_32.lib advapi32.lib shell32.lib comctl32.lib

if errorlevel 1 (
    echo.
    echo [ERROR] Compilation failed!
    exit /b 1
)

echo.
echo ========================================================
echo  [SUCCESS] AntiGoon.exe compiled successfully!
echo ========================================================
echo Standalone Portable Executable: %~dp0AntiGoon.exe
del *.obj res\*.res 2>nul
