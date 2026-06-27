@echo off
REM ============================================================================
REM  run_dungeon_mock.bat — Avvia Dungeon Crawler Basic con Mock Engine Python
REM
REM  Architettura porte:
REM    - GUI e' il server TCP sulla porta 9200 (eventi MockEngine -> GUI)
REM    - MockEngine ascolta comandi dalla GUI sulla porta 9201
REM
REM  Ordine di avvio:
REM    1) GUI PySide6       -> apre server eventi (9200) e attende
REM    2) Mock Dungeon Engine -> si connette alla GUI e ascolta comandi (9201)
REM
REM  Uso: doppio clic oppure "run_dungeon_mock.bat" da terminale.
REM ============================================================================

setlocal

set "ROOT=%~dp0..\.."
set "PY=%ROOT%\.venv\Scripts\python.exe"
set "PYLIB=%ROOT%\pyLib"
set "DUNGEON=%ROOT%\GAME\Dungeon-Crawler-Basic"
set "GUI_DIR=%DUNGEON%\GUI"

if not exist "%PY%" (
    echo [Dungeon] ERRORE: Python venv non trovato in "%PY%".
    echo           Esegui prima: python -m venv .venv  dal root del workspace.
    exit /b 1
)

if not exist "%GUI_DIR%\main.py" (
    echo [Dungeon] ERRORE: GUI non trovata in "%GUI_DIR%".
    exit /b 1
)

if not exist "%DUNGEON%\mock_dungeon_engine.py" (
    echo [Dungeon] ERRORE: Mock engine non trovato in "%DUNGEON%".
    exit /b 1
)

echo [Dungeon Mock] Avvio GUI Dungeon Crawler...
start "Dungeon Crawler - GUI" cmd /c "cd /d "%ROOT%" && set PYTHONPATH=%PYLIB% && "%PY%" "%GUI_DIR%\main.py""

echo [Dungeon Mock] Attendo apertura receiver GUI (porta 9200)...
timeout /t 3 /nobreak >nul

echo [Dungeon Mock] Avvio Mock Dungeon Engine...
start "Dungeon Crawler - MockEngine" cmd /c "cd /d "%ROOT%" && set PYTHONPATH=%PYLIB% && "%PY%" "%DUNGEON%\mock_dungeon_engine.py""

echo [Dungeon Mock] Avviato.
echo.
echo   GUI     : finestra "Dungeon Crawler - GUI"
echo   Engine  : finestra "Dungeon Crawler - MockEngine"
echo.
echo   Trascina le carte dalla Mano (CardHand) verso Giocate (PlayArea)
echo   per vedere gli effetti applicati agli Actors.
echo.
exit /b 0
