@echo off
REM ============================================================================
REM  run_tris.bat - Avvia il gioco Tic-Tac-Toe nel giusto ordine.
REM
REM  Ordine corretto:
REM    1) GUI PySide6  -> apre il server eventi sulla porta 9100 e attende
REM    2) CoreEngine   -> si connette alla GUI e ascolta i comandi sulla 9001
REM
REM  La GUI deve partire per prima: e' lei a fare da server degli eventi.
REM  Ogni applicazione viene avviata in una propria finestra.
REM ============================================================================

setlocal

REM Cartella di questo script (radice del progetto Tic-Tac-Toe).
set "ROOT=%~dp0"
set "GUI_DIR=%ROOT%GUI"
set "ENGINE=%ROOT%..\..\build\GAME\Tic-Tac-Toe\CoreEngine\Debug\tris_engine.exe"

echo [Tris] Avvio in corso...

REM --- Controllo prerequisiti -------------------------------------------------
if not exist "%GUI_DIR%\main.py" (
    echo [Tris] ERRORE: GUI non trovata in "%GUI_DIR%".
    goto :error
)
if not exist "%ENGINE%" (
    echo [Tris] ERRORE: eseguibile engine non trovato in "%ENGINE%".
    echo [Tris] Compila prima con:
    echo        cmake --build build --target tris_engine --config Debug
    goto :error
)

REM --- 1) Avvio della GUI (server eventi) -------------------------------------
echo [Tris] 1/2 Avvio GUI (porta eventi 9100)...
start "Tris GUI" cmd /c "cd /d "%GUI_DIR%" && python main.py"

REM Attende qualche secondo che la GUI apra il socket server prima dell'engine.
echo [Tris] Attendo l'avvio della GUI...
timeout /t 3 /nobreak >nul

REM --- 2) Avvio del CoreEngine ------------------------------------------------
echo [Tris] 2/2 Avvio CoreEngine (porta comandi 9001)...
start "Tris CoreEngine" cmd /c ""%ENGINE%""

echo [Tris] Avviato. Nella GUI premi "Nuova partita" per iniziare.
goto :end

:error
echo [Tris] Avvio interrotto.
exit /b 1

:end
endlocal
