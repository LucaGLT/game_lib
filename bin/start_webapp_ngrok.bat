@echo off
setlocal

REM Usage:
REM   start_webapp_ngrok.bat [eldhom|tris] [port]
REM Examples:
REM   start_webapp_ngrok.bat eldhom
REM   start_webapp_ngrok.bat tris 5173

set "APP=%~1"
if "%APP%"=="" set "APP=eldhom"

set "PORT=%~2"
if "%PORT%"=="" set "PORT=5173"

set "ROOT=%~dp0.."
set "SCRIPT_DIR=%~dp0"

if /I "%APP%"=="eldhom" (
    set "FRONTEND_DIR=%ROOT%\GAME\Eldhom\WebApp\webapp_frontend"
) else if /I "%APP%"=="tris" (
    set "FRONTEND_DIR=%ROOT%\GAME\Tic-Tac-Toe\WebApp\webapp_frontend"
) else if /I "%APP%"=="tictactoe" (
    set "FRONTEND_DIR=%ROOT%\GAME\Tic-Tac-Toe\WebApp\webapp_frontend"
) else (
    echo [ERROR] App non riconosciuta: %APP%
    echo         Valori validi: eldhom ^| tris
    exit /b 1
)

if not exist "%FRONTEND_DIR%\package.json" (
    echo [ERROR] Frontend non trovato in:
    echo         %FRONTEND_DIR%
    exit /b 1
)

where ngrok >nul 2>&1
set "NGROK_CMD="

if defined NGROK_EXE (
    if exist "%NGROK_EXE%" (
        set "NGROK_CMD=%NGROK_EXE%"
    )
)

if not defined NGROK_CMD (
    for /f "delims=" %%I in ('where ngrok 2^>nul') do (
        set "NGROK_CMD=%%I"
        goto :ngrok_found
    )
)

if not defined NGROK_CMD (
    for /d %%D in ("%LOCALAPPDATA%\Microsoft\WinGet\Packages\Ngrok.Ngrok_*") do (
        if exist "%%~fD\ngrok.exe" (
            set "NGROK_CMD=%%~fD\ngrok.exe"
            goto :ngrok_found
        )
    )
)

if not defined NGROK_CMD (
    if exist "%SCRIPT_DIR%ngrok.path" (
        set /p NGROK_CMD=<"%SCRIPT_DIR%ngrok.path"
        if not exist "%NGROK_CMD%" (
            set "NGROK_CMD="
        )
    )
)

if not defined NGROK_CMD (
    echo [WARN] ngrok non trovato nel PATH.
    set /p NGROK_CMD=Inserisci il percorso completo di ngrok.exe: 
    if "%NGROK_CMD%"=="" (
        echo [ERROR] Nessun percorso inserito.
        exit /b 1
    )
    if not exist "%NGROK_CMD%" (
        echo [ERROR] File non trovato: %NGROK_CMD%
        exit /b 1
    )
    >"%SCRIPT_DIR%ngrok.path" echo %NGROK_CMD%
    echo [INFO] Percorso ngrok salvato in %SCRIPT_DIR%ngrok.path
)

:ngrok_found

where npm >nul 2>&1
if errorlevel 1 (
    echo [ERROR] npm non trovato nel PATH.
    exit /b 1
)

echo.
echo [INFO] Frontend scelto: %APP%
echo [INFO] Cartella frontend: %FRONTEND_DIR%
echo [INFO] Porta Vite/ngrok: %PORT%
echo [INFO] ngrok: %NGROK_CMD%
echo.
echo [INFO] Avvio Vite in una nuova finestra...
start "Vite - %APP%" cmd /k "cd /d ""%FRONTEND_DIR%"" && npm install && npm run dev -- --host 0.0.0.0 --port %PORT%"

echo [INFO] Avvio ngrok (CTRL+C per fermare il tunnel)...
echo [INFO] URL pubblico visibile nell'output ngrok (Forwarding).
echo.
"%NGROK_CMD%" http %PORT%

endlocal
