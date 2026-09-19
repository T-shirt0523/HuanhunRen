@echo off
rem ============================================================
rem Huanhun Ren (还魂人) - build script (MinGW-w64 G++ + raylib 5.5)
rem Usage: run build.bat, output: game.exe (needs raylib.dll beside it)
rem ============================================================
setlocal
cd /d %~dp0

set GXX=D:\codetool\mingw64\bin\g++.exe
set WINDRES=D:\codetool\mingw64\bin\windres.exe
set RAYLIB=D:\codetool\raylib-5.5_win64_mingw-w64

rem Fallback: auto search raylib on D: drive if default path missing
if not exist "%GXX%" set GXX=g++
if not exist "%RAYLIB%\lib\libraylibdll.a" (
    for /d %%D in (D:\codetool\raylib* D:\raylib* D:\*\raylib*) do (
        if exist "%%D\lib\libraylibdll.a" set RAYLIB=%%D
    )
)

echo [1/3] Compiling...
set ICON_OBJ=
if exist icon.ico (
    echo       Embedding icon.ico into game.exe ...
    echo 1 ICON "icon.ico" > icon.rc
    %WINDRES% icon.rc -O coff -o icon_res.o
    if errorlevel 1 (
        echo.
        echo [ERROR] windres failed!
        exit /b 1
    )
    set ICON_OBJ=icon_res.o
)
%GXX% main.cpp world.cpp creature.cpp assets.cpp audio.cpp water.cpp progress.cpp net.cpp l10n.cpp %ICON_OBJ% -o game.exe ^
    -std=c++17 -O2 -Wall ^
    -I%RAYLIB%\include -L%RAYLIB%\lib ^
    -lraylibdll -lopengl32 -lgdi32 -lwinmm -lws2_32
if errorlevel 1 (
    echo.
    echo [ERROR] Build failed!
    exit /b 1
)

echo [2/3] Copying raylib.dll...
copy /y "%RAYLIB%\lib\raylib.dll" . >nul

echo [3/3] Build OK: game.exe
pause
