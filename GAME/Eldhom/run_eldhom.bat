@echo off
rem ─────────────────────────────────────────────────────────────────────────────
rem  Le Pergamene di Eldhom — launcher
rem
rem  1. Starts the C++ CoreEngine first: listens on port 9211 (commands) and
rem     waits up to ~10 s for the GUI to connect.
rem  2. Waits 1.5 seconds for the engine to bind its socket.
rem  3. Starts the PySide6 GUI: binds port 9210 (events) and immediately
rem     tries to connect to the engine on 9211 (3-second timeout).
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

echo [Eldhom] Avvio CoreEngine in finestra separata (porta 9211)...
start "Eldhom Engine - C++" "%ENGINE_EXE%" "%DATA_DIR%"

echo [Eldhom] Attesa 1.5 secondi per il socket server dell'Engine...
timeout /t 2 /nobreak > nul

echo [Eldhom] Avvio GUI in finestra separata (porta 9210)...
start "Eldhom GUI - Python" python "%~dp0GUI\main.py"

echo [Eldhom] Entrambe le finestre sono state aperte.
echo [Eldhom] La GUI si connette automaticamente all'Engine.
echo [Eldhom] Usa il menu "Gioca ^> Inizia Nuova Missione" per iniziare.
echo [Eldhom] Chiudi le finestre GUI e Engine quando termini.
pause
endlocal
