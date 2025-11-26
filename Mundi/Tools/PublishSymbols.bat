@echo off
setlocal enabledelayedexpansion

:: ========================================================
:: [Setup 0] Check Configuration
:: ========================================================
set CONFIG=%1

if "%CONFIG%"=="" (
    echo [SymStore] Configuration argument is missing. Skipping symbol upload.
    exit /b 0
)

:: ========================================================
:: [Setup 1] Path Setup
:: ========================================================
:: Directory containing symstore.exe and pdbstr.exe
set TOOL_DIR=%~dp0SymStore
:: Target binary directory to process
set BIN_DIR=%~dp0..\..\Binaries\%CONFIG%

:: Path to pdbstr.exe (Required for source indexing)
set PDBSTR_EXE=%TOOL_DIR%\pdbstr.exe

:: ========================================================
:: [Setup 2] Git Information Extraction
:: ========================================================
echo [Git] Extracting version info...

for /f "delims=" %%i in ('git rev-parse --short HEAD') do set GIT_HASH=%%i
for /f "delims=" %%i in ('git remote get-url origin') do set GIT_URL=%%i

if "%GIT_HASH%"=="" set GIT_HASH=NoGit
if "%GIT_URL%"=="" set GIT_URL=UnknownRepo

echo [Git] Current Commit: %GIT_HASH%
echo [Git] Repository: %GIT_URL%

:: ========================================================
:: [Setup 3] Source Indexing
:: Inject source repository info into PDB files.
:: This allows the debugger to fetch source code from the remote repo.
:: ========================================================
if exist "%PDBSTR_EXE%" (
    echo [SourceIndexing] Injecting source info into PDBs...
    
    :: Generate srcsrv.ini file (Temporary)
    set SRCSRV_INI=%TEMP%\srcsrv_%GIT_HASH%.ini
    
    :: Note: This mapping assumes a standard GitHub structure.
    :: Adjust the mapping (*.cpp*MYSERVER/...) if your folder structure differs.
    (
        echo SRCSRV: variables ------------------------------------------
        echo VERSION=1
        echo VERCTRL=http
        echo SRCSRV: variables ------------------------------------------
        echo MYSERVER=%GIT_URL%/raw/%GIT_HASH%
        echo SRCSRV: source files ---------------------------------------
        echo %~dp0..\..\Source\*.cpp*MYSERVER/Source/*.cpp
        echo %~dp0..\..\Source\*.h*MYSERVER/Source/*.h
        echo %~dp0..\..\Source\*.inl*MYSERVER/Source/*.inl
    ) > "!SRCSRV_INI!"

    :: Loop through all PDBs and inject the stream
    for %%f in ("%BIN_DIR%\*.pdb") do (
        "%PDBSTR_EXE%" -w -p:"%%f" -i:"!SRCSRV_INI!" -s:srcsrv
    )
    
    echo [SourceIndexing] Done.
) else (
    echo [Warning] pdbstr.exe not found in TOOL_DIR. Skipping Source Indexing.
)

:: ========================================================
:: [Setup 4] Symbol Server Connection Check
:: ========================================================
set SERVER_IP=172.21.11.109
set STORE_PATH=\\%SERVER_IP%\SymbolStore

echo [SymStore] Checking server availability...

:: Check Port 445 (SMB) with 3000ms timeout
powershell -Command "$t = New-Object System.Net.Sockets.TcpClient; $r = $t.BeginConnect('%SERVER_IP%', 445, $null, $null); if ($r.AsyncWaitHandle.WaitOne(3000)) { exit 0 } else { exit 1 }"

if errorlevel 1 (
    echo [Warning] Server connection timeout or unreachable.
    exit /b 0
)

if not exist "%STORE_PATH%" (
    echo [Warning] Server reachable but path "%STORE_PATH%" not found.
    exit /b 0
)

:: ========================================================
:: [Setup 5] Upload to Symbol Server
:: ========================================================
set "NOW_TIME=%time: =0%"
:: Version Tag: Includes Config, GitHash, and Timestamp
set VERSION_TAG=Build_%CONFIG%_%GIT_HASH%_%date:~0,4%-%date:~5,2%-%date:~8,2%_%NOW_TIME:~0,2%%NOW_TIME:~3,2%

echo ========================================================
echo    Target Config : %CONFIG%
echo    Git Commit    : %GIT_HASH%
echo    Target Server : %STORE_PATH%
echo    Version Tag   : %VERSION_TAG%
echo ========================================================

if not exist "%BIN_DIR%" (
    echo [Error] Directory "%BIN_DIR%" not found!
    exit /b 1
)

echo [SymStore] Starting upload...

:: Add Git info to the transaction comment
set COMMENT_MSG="Git: %GIT_HASH% (%GIT_URL%)"

:: 1. Upload .pdb files
"%TOOL_DIR%\symstore.exe" add /f "%BIN_DIR%\*.pdb" /s "%STORE_PATH%" /t "MundiEngine" /v "%VERSION_TAG%" /c %COMMENT_MSG% /r /o

:: 2. Upload .exe files
"%TOOL_DIR%\symstore.exe" add /f "%BIN_DIR%\*.exe" /s "%STORE_PATH%" /t "MundiEngine" /v "%VERSION_TAG%" /c %COMMENT_MSG% /r /o

:: 3. Upload .dll files
"%TOOL_DIR%\symstore.exe" add /f "%BIN_DIR%\*.dll" /s "%STORE_PATH%" /t "MundiEngine" /v "%VERSION_TAG%" /c %COMMENT_MSG% /r /o

echo.
echo [Success] Uploaded symbols for %CONFIG% version with Git Info.