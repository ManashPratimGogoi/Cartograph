@echo off
REM Runs the API with the MSYS2 UCRT64 runtime DLLs on PATH.
REM The SQLite database (worlds.db) is created in the working directory.
set PATH=C:\msys64\ucrt64\bin;%PATH%
"%~dp0..\bin\TerrainServer.exe"
