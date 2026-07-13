# Test script para debugare la connessione GUI-Engine

Write-Host "[Test] Avvio test con messaggi visibili" -ForegroundColor Cyan

# Termina i processi in esecuzione
Write-Host "[Test] Fermando processi precedenti..." -ForegroundColor Yellow
Get-Process eldhom_engine -ErrorAction SilentlyContinue | Stop-Process -Force
Get-Process python -ErrorAction SilentlyContinue | Where-Object {$_.CommandLine -match 'main.py'} | Stop-Process -Force
Start-Sleep -Seconds 1

# Avvia la GUI (Python main.py) in una finestra PowerShell visibile
Write-Host "[Test] Avvio GUI (Python main.py)..." -ForegroundColor Green
$gui_process = Start-Process -FilePath "python" -ArgumentList "GUI\main.py" -WorkingDirectory $PSScriptRoot -PassThru -NoNewWindow
Write-Host "[Test] GUI PID: $($gui_process.Id)" -ForegroundColor Green

# Attendi 5 secondi per l'inizializzazione del server socket
Write-Host "[Test] Attesa 5 secondi per socket server GUI..." -ForegroundColor Yellow
Start-Sleep -Seconds 5

# Verifica le porte in ascolto
Write-Host "[Test] Status porte:" -ForegroundColor Cyan
netstat -an | Select-String ":921[01]"

# Avvia l'engine
Write-Host "[Test] Avvio engine (eldhom_engine.exe)..." -ForegroundColor Green
$exe_path = "c:\_GLT_\Qt Prj\game_lib\build\GAME\Eldhom\CoreEngine\Debug\eldhom_engine.exe"
$data_dir = "`"c:\_GLT_\Qt Prj\game_lib\GAME\Eldhom\data`""
$engine_process = Start-Process -FilePath $exe_path -ArgumentList $data_dir -PassThru -NoNewWindow
Write-Host "[Test] Engine PID: $($engine_process.Id)" -ForegroundColor Green

# Attendi 3 secondi per il connect
Write-Host "[Test] Attesa 3 secondi per TCP connect..." -ForegroundColor Yellow
Start-Sleep -Seconds 3

# Verifica lo status di nuovo
Write-Host "[Test] Status porte dopo engine startup:" -ForegroundColor Cyan
netstat -an | Select-String ":921[01]"

# Attendi che i processi terminino (Ctrl+C per interrompere)
Write-Host "[Test] Premi Ctrl+C per terminare..." -ForegroundColor Yellow
$gui_process | Wait-Process -ErrorAction SilentlyContinue
$engine_process | Wait-Process -ErrorAction SilentlyContinue
