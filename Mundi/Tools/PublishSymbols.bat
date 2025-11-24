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
set TOOL_DIR=%~dp0SymStore

:: ========================================================
:: [Setup 2] Build Output Directory (Relative Path)
:: ========================================================
set BIN_DIR=%~dp0..\..\Binaries\%CONFIG%

:: ========================================================
:: [Setup 3] Symbol Server Path & IP Check
:: ========================================================
set SERVER_IP=172.21.11.109
set STORE_PATH=\\%SERVER_IP%\SymbolStore

echo [SymStore] Checking server availability...

:: 1. TCP Port 445 Check (3000ms Timeout)
powershell -Command "$t = New-Object System.Net.Sockets.TcpClient; $r = $t.BeginConnect('%SERVER_IP%', 445, $null, $null); if ($r.AsyncWaitHandle.WaitOne(3000)) { exit 0 } else { exit 1 }"

if errorlevel 1 (
    echo [Warning] Server connection timeout or unreachable.
    echo Target: %SERVER_IP%
    echo Skipping symbol upload.
    exit /b 0
)

:: 2. Path Check
if not exist "%STORE_PATH%" (
    echo [Warning] Server is reachable, but path not found: "%STORE_PATH%"
    echo Skipping symbol upload.
    exit /b 0
)

echo [SymStore] Server is accessible.

:: ========================================================
:: [Setup 4] Version Tag (Auto-generated from Date_Time)
:: ========================================================
set "NOW_TIME=%time: =0%"
set VERSION_TAG=Build_%CONFIG%_%date:~0,4%-%date:~5,2%-%date:~8,2%_%NOW_TIME:~0,2%%NOW_TIME:~3,2%


echo ========================================================
echo   Target Config : %CONFIG%
echo   Source Path   : %BIN_DIR%
echo   Target Server : %STORE_PATH%
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