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

echo [Eldhom] Avvio GUI in finestra separata (porta 9210)...
start "Eldhom GUI - Python" python "%~dp0GUI\main.py"

echo [Eldhom] Attesa 5 secondi per il socket server della GUI...
timeout /t 5 /nobreak > nul

echo [Eldhom] Avvio CoreEngine in finestra separata (porta 9211)...
start "Eldhom Engine - C++" "%ENGINE_EXE%" "%DATA_DIR%"

echo [Eldhom] Entrambe le finestre sono state aperte.
echo [Eldhom] GUI: seleziona una missione per avviare il gioco
echo [Eldhom] Chiudi le finestre GUI e Engine quando termini.
pause
endlocal
