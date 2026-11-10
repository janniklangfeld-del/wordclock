# Firmware bauen und per USB auf den ESP32 spielen, danach seriellen Monitor oeffnen.
#   powershell -ExecutionPolicy Bypass -File scripts\flash.ps1            -> LED-Test
#   powershell -ExecutionPolicy Bypass -File scripts\flash.ps1 wordclock  -> Uhr-Firmware
#   powershell -ExecutionPolicy Bypass -File scripts\flash.ps1 ledtest -BuildOnly
param(
  [string]$Env = "ledtest",
  [switch]$BuildOnly
)
$ErrorActionPreference = "Continue"
Set-Location (Split-Path $PSScriptRoot -Parent)

$pio = Join-Path $env:USERPROFILE ".platformio\penv\Scripts\pio.exe"
if (-not (Test-Path $pio)) { $pio = "pio" }

if ($BuildOnly) {
  & $pio run -e $Env
} else {
  Write-Host "== $Env bauen + flashen (ESP32 per USB anschliessen; Abbruch Monitor: Strg+C)" -ForegroundColor Cyan
  & $pio run -e $Env -t upload -t monitor
}
