@echo off
rem ─────────────────────────────────────────────────────────────────────────────
rem  Le Pergamene di Eldhom — launcher
rem
rem  1. Starts the PySide6 GUI on port 9210 (event server).
rem  2. Waits 1.5 seconds for the GUI to bind the socket.
rem  3. Starts the C++ CoreEngine (connects to 9210, listens on 9211).
rem
rem  Run this script from the GAME\Eldhom\ directory.
rem ─────────────────────────────────────────────────────────────────────────────

setlocal

rem Path to workspace root (two levels up from GAME\Eldhom\)
set "WORKSPACE=%~dp0..\.."

rem Path to C++ engine executable (built by CMake in build\)
set "ENGINE_EXE=%WORKSPACE%\build\GAME\Eldhom\CoreEngine\Debug\eldhom_engine.exe"
if not exist "%ENGINE_EXE%" (
    set "ENGINE_EXE=%WORKSPACE%\build\GAME\Eldhom\CoreEngine\Release\eldhom_engine.exe"
)
if not exist "%ENGINE_EXE%" (
    echo [ERROR] eldhom_engine.exe non trovato. Esegui prima la build CMake.
    pause
    exit /b 1
)

rem Data directory path (absolute, forwarded to engine as first argument)
set "DATA_DIR=%~dp0data"

echo [Eldhom] Avvio GUI (porta 9210)...
start "" /B python "%~dp0GUI\main.py"

echo [Eldhom] Attesa 1.5 secondi per la GUI...
timeout /t 1 /nobreak > nul

echo [Eldhom] Avvio CoreEngine (porta 9211)...
start "" /B "%ENGINE_EXE%" "%DATA_DIR%"

echo [Eldhom] Avviato. Chiudi questa finestra per terminare entrambi i processi.
echo [Eldhom] (oppure chiudi le finestre GUI e engine separatamente)
pause
endlocal
