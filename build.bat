@echo off
REM Build ytdl.exe via MSYS2 MinGW64 toolchain.
REM Erwartet: C:\msys64 vorhanden + pacman-Pakete mingw-w64-x86_64-{gcc,cmake,ninja}

setlocal
set MSYS2_ROOT=C:\msys64
set PATH=%MSYS2_ROOT%\mingw64\bin;%MSYS2_ROOT%\usr\bin;%PATH%

cd /d "%~dp0"

echo === Configure (CMake + Ninja) ===
cmake -S . -B build -G Ninja ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_C_COMPILER=gcc ^
    -DCMAKE_CXX_COMPILER=g++
if errorlevel 1 goto :fail

echo === Build ===
cmake --build build --config Release -j
if errorlevel 1 goto :fail

echo.
echo === Fertig ===
echo Binary: %CD%\build\ytdl.exe
echo yt-dlp: %CD%\build\yt-dlp.exe (kopiert)
echo.
echo Test: build\ytdl.exe "https://www.youtube.com/watch?v=dQw4w9WgXcQ"
exit /b 0

:fail
echo Build fehlgeschlagen.
exit /b 1
