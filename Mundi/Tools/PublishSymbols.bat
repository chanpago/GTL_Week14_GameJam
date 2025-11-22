@echo off
setlocal

:: ========================================================
:: [Setup 0] Check Configuration
:: ========================================================
set CONFIG=%1

if "%CONFIG%"=="" (
    echo [SymStore] Configuration argument is missing. Skipping symbol upload.
    exit /b 0
)

:: ========================================================
:: [Setup 1] Path to symstore.exe
:: ========================================================
set TOOL_DIR=C:\Program Files (x86)\Windows Kits\10\Debuggers\x64

:: ========================================================
:: [Setup 2] Build Output Directory (Relative Path)
:: ========================================================
set BIN_DIR=%~dp0..\..\Binaries\%CONFIG%

:: ========================================================
:: [Setup 3] Symbol Server Path
:: ========================================================
set STORE_PATH=\\172.21.11.109\SymbolStore

:: ========================================================
:: [Setup 4] Version Tag (Auto-generated from Date_Time)
:: ========================================================
set VERSION_TAG=Build_%CONFIG%_%date:~0,4%-%date:~5,2%-%date:~8,2%_%time:~0,2%%time:~3,2%

echo.
echo ========================================================
echo  Target Config : %CONFIG%
echo  Source Path   : %BIN_DIR%
echo  Target Server : %STORE_PATH%
echo ========================================================

:: Validate Path
if not exist "%BIN_DIR%" (
    echo [Error] Directory "%BIN_DIR%" not found!
    exit /b 1
)

echo [SymStore] Starting upload...

:: 1. Upload .pdb files
"%TOOL_DIR%\symstore.exe" add /f "%BIN_DIR%\*.pdb" /s "%STORE_PATH%" /t "MundiEngine" /v "%VERSION_TAG%" /r /o

:: 2. Upload .exe files
"%TOOL_DIR%\symstore.exe" add /f "%BIN_DIR%\*.exe" /s "%STORE_PATH%" /t "MundiEngine" /v "%VERSION_TAG%" /r /o

:: 3. Upload .dll files
"%TOOL_DIR%\symstore.exe" add /f "%BIN_DIR%\*.dll" /s "%STORE_PATH%" /t "MundiEngine" /v "%VERSION_TAG%" /r /o

echo.
echo [Success] Uploaded symbols for %CONFIG% version.