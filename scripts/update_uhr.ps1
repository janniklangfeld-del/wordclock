# Firmware per WLAN auf die Uhr laden (POST /update).
# Das Admin-Passwort kommt aus der Benutzer-Umgebungsvariable WORDCLOCK_OTA_PASSWORD
# (einmal setzen: [Environment]::SetEnvironmentVariable("WORDCLOCK_OTA_PASSWORD", "...", "User")).
#
#   powershell -File scripts\update_uhr.ps1 private\wordclock_v1.1.6.bin [192.168.15.125]
param(
  [Parameter(Mandatory = $true)][string]$Bin,
  [string]$Clock = "wordclock.local"
)
$pw = $env:WORDCLOCK_OTA_PASSWORD
if (-not $pw) { $pw = [Environment]::GetEnvironmentVariable("WORDCLOCK_OTA_PASSWORD", "User") }
if (-not $pw) { Write-Error "WORDCLOCK_OTA_PASSWORD ist nicht gesetzt."; exit 1 }
if (-not (Test-Path $Bin)) { Write-Error "Datei fehlt: $Bin"; exit 1 }
Write-Host "Lade $Bin auf http://$Clock ..."
$out = & curl.exe -s -m 180 -u "admin:$pw" -F "fw=@$Bin" "http://$Clock/update"
Write-Host $out
if ($out -notmatch '"ok":true') { exit 1 }
