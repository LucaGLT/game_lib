@echo off
REM ============================================================================
REM  run_dungeon.bat - Avvia Dungeon Crawler Basic nel giusto ordine.
REM
REM  Architettura porte:
REM    - GUI e' il server TCP sulla porta 9200 (eventi CoreEngine -> GUI)
REM    - CoreEngine si connette alla GUI su 9200 e ascolta comandi su 9201
REM
REM  Ordine di avvio:
REM    1) GUI PySide6  -> apre il server eventi sulla porta 9200 e attende
REM    2) CoreEngine   -> si connette alla GUI e ascolta i comandi sulla 9201
REM
REM  La GUI deve partire per prima: e' lei a fare da server degli eventi.
REM  Ogni applicazione viene avviata in una propria finestra di console.
REM ============================================================================

setlocal

REM Cartella di questo script (radice del progetto Dungeon Crawler Basic).
set "ROOT=%~dp0"
set "GUI_DIR=%ROOT%GUI"
set "ENGINE=%ROOT%..\..\build\GAME\Dungeon-Crawler-Basic\CoreEngine\Debug\dungeon_engine.exe"

echo [Dungeon] Avvio in corso...

REM --- Controllo prerequisiti -------------------------------------------------
if not exist "%GUI_DIR%\main.py" (
    echo [Dungeon] ERRORE: GUI non trovata in "%GUI_DIR%".
    goto :error
)
if not exist "%ENGINE%" (
    echo [Dungeon] ERRORE: eseguibile engine non trovato.
    echo [Dungeon] Percorso atteso:
    echo           %ENGINE%
    echo [Dungeon] Compila prima con:
    echo           cmake --build build --target dungeon_engine --config Debug
    goto :error
)

REM --- 1) Avvio della GUI (server eventi porta 9200) --------------------------
echo [Dungeon] 1/2 Avvio GUI (porta eventi 9200)...
start "Dungeon GUI" /D "%GUI_DIR%" "%COMSPEC%" /k "python main.py || (echo. & echo [Dungeon GUI] Avvio GUI fallito. & pause)"

REM Attende che la GUI apra il socket server prima di avviare il CoreEngine.
echo [Dungeon] Attendo l'avvio della GUI (3 secondi)...
timeout /t 3 /nobreak >nul

REM --- 2) Avvio del CoreEngine ------------------------------------------------
echo [Dungeon] 2/2 Avvio CoreEngine (porta comandi 9201)...
start "Dungeon CoreEngine" /D "%ROOT%" "%COMSPEC%" /k ""%ENGINE%" || (echo. & echo [Dungeon CoreEngine] Avvio CoreEngine fallito. & pause)"

echo.
echo [Dungeon] Entrambi i processi avviati.
echo [Dungeon] Nella GUI premi "New Game" nella toolbar per iniziare la partita.
goto :end

:error
echo [Dungeon] Avvio interrotto. Correggi gli errori sopra e riprova.
pause
exit /b 1

:end
endlocal
