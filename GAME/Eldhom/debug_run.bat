@echo off
rem ─────────────────────────────────────────────────────────────────────────────
rem  Le Pergamene di Eldhom — debug launcher with separate windows
rem
rem  Launches GUI and Engine in separate console windows with full debug output.
rem
rem ─────────────────────────────────────────────────────────────────────────────

setlocal

set "WORKSPACE=%~dp0..\.."
set "ENGINE_EXE=%WORKSPACE%\build\GAME\Eldhom\CoreEngine\Debug\eldhom_engine.exe"
set "DATA_DIR=%~dp0data"

if not exist "%ENGINE_EXE%" (
    echo [ERROR] eldhom_engine.exe non trovato. Esegui prima la build CMake.
    pause
    exit /b 1
)

echo [EldhomDebug] Fermando processi precedenti...
taskkill /F /IM eldhom_engine.exe >nul 2>&1
taskkill /F /IM python.exe /FI "COMMANDLINE like main.py" >nul 2>&1
timeout /t 1 /nobreak >nul

echo [EldhomDebug] Avvio GUI in finestra separata (porta 9210)...
start "Eldhom GUI" cmd /K "cd /d "%~dp0" && python GUI\main.py"

echo [EldhomDebug] Attesa 5 secondi per socket server della GUI...
timeout /t 5 /nobreak >nul

echo [EldhomDebug] Avvio Engine in finestra separata (porta 9211)...
start "Eldhom Engine" cmd /K "cd /d "%~dp0" && "%ENGINE_EXE%" "%DATA_DIR%""

echo [EldhomDebug] Entrambi i processi sono in esecuzione. Seleziona una missione nella GUI...
echo [EldhomDebug] Premi un tasto per terminare tutti i processi.
pause

echo [EldhomDebug] Terminando...
taskkill /F /IM eldhom_engine.exe >nul 2>&1
taskkill /F /IM python.exe /FI "COMMANDLINE like main.py" >nul 2>&1

endlocal
