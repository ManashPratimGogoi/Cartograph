@echo off
REM Builds the Crow API server using the MSYS2 UCRT64 toolchain.
REM
REM Crow ships as a single header (server/third_party/crow_all.h) since it is
REM not in the MSYS2 repos. The remaining dependencies come from pacman; run
REM once inside the "MSYS2 UCRT64" terminal:
REM   pacman -Syu
REM   pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-asio ^
REM     mingw-w64-ucrt-x86_64-nlohmann-json mingw-w64-ucrt-x86_64-sqlite3 ^
REM     mingw-w64-ucrt-x86_64-pkgconf

setlocal
set MSYS2=C:\msys64\ucrt64
if not exist "%MSYS2%\bin\g++.exe" (
    echo Could not find MSYS2 UCRT64 at %MSYS2%.
    echo Install MSYS2 from https://www.msys2.org/ and the packages listed at
    echo the top of this script, then run build_server.bat again.
    exit /b 1
)

set PATH=%MSYS2%\bin;%PATH%
if not exist "..\bin" mkdir "..\bin"

"%MSYS2%\bin\g++.exe" -std=c++17 -O2 -Wall -Wextra ^
    main.cpp TerrainService.cpp WorldStore.cpp ^
    ..\engine\Noise.cpp ..\engine\Terrain.cpp ^
    -I"%MSYS2%\include" ^
    -L"%MSYS2%\lib" ^
    -pthread ^
    -lsqlite3 -lws2_32 -lwsock32 -lmswsock ^
    -o ..\bin\TerrainServer.exe

if errorlevel 1 (
    echo Server build failed.
    exit /b 1
)
echo Built ..\bin\TerrainServer.exe
endlocal
