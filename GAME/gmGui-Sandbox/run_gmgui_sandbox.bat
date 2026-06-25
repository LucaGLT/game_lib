@echo off
setlocal

set "ROOT=%~dp0..\..\"
set "PY=%ROOT%.venv\Scripts\python.exe"
set "PYLIB=%ROOT%pyLib"
set "SANDBOX=%ROOT%GAME\gmGui-Sandbox"

if not exist "%PY%" (
    echo [gmGui-Sandbox] ERRORE: Python venv non trovato in "%PY%".
    exit /b 1
)

echo [gmGui-Sandbox] Avvio GUI libreria gmGui...
start "gmGui Sandbox - GUI" cmd /c "cd /d "%ROOT%" && set PYTHONPATH=%PYLIB% && "%PY%" -m gmGui.main"

echo [gmGui-Sandbox] Attendo apertura receiver (porta 9000)...
timeout /t 2 /nobreak >nul

echo [gmGui-Sandbox] Avvio mock engine eventi...
start "gmGui Sandbox - MockEngine" cmd /c "cd /d "%ROOT%" && set PYTHONPATH=%PYLIB% && "%PY%" "%SANDBOX%\mock_engine.py""

echo [gmGui-Sandbox] Avviato.
exit /b 0
