@echo off
setlocal

set "ROOT=%~dp0..\..\"
set "PY=%ROOT%.venv\Scripts\python.exe"
set "PYLIB=%ROOT%pyLib"
set "SANDBOX=%ROOT%GAME\gmGui-Sandbox"
set "EVENT_PORT=19000"
set "COMMAND_PORT=19001"

if not exist "%PY%" (
    echo [gmGui-Sandbox] ERRORE: Python venv non trovato in "%PY%".
    exit /b 1
)

echo [gmGui-Sandbox] Avvio GUI libreria gmGui...
start "gmGui Sandbox - GUI" cmd /c "cd /d "%ROOT%" && set PYTHONPATH=%PYLIB% && set GMGUI_EVENT_PORT=%EVENT_PORT% && set GMGUI_COMMAND_PORT=%COMMAND_PORT% && "%PY%" -m gmGui.main"

echo [gmGui-Sandbox] Attendo apertura receiver (porta %EVENT_PORT%)...
timeout /t 2 /nobreak >nul

echo [gmGui-Sandbox] Avvio mock engine eventi...
start "gmGui Sandbox - MockEngine" cmd /c "cd /d "%ROOT%" && set PYTHONPATH=%PYLIB% && "%PY%" "%SANDBOX%\mock_engine.py" --port %EVENT_PORT% --cmd-port %COMMAND_PORT%"

echo [gmGui-Sandbox] Avviato.
exit /b 0
