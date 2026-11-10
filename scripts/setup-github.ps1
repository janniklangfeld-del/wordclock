# Einmalig: lokales Git-Repo anlegen und als oeffentliches Repo "wordclock" auf GitHub veroeffentlichen.
# Aufruf im Projektordner:
#   powershell -ExecutionPolicy Bypass -File scripts\setup-github.ps1
$ErrorActionPreference = "Continue"
Set-Location (Split-Path $PSScriptRoot -Parent)
Write-Host "== Projektordner: $(Get-Location)" -ForegroundColor Cyan

function Have($cmd) { [bool](Get-Command $cmd -ErrorAction SilentlyContinue) }
function Refresh-Path {
  $env:Path = [Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [Environment]::GetEnvironmentVariable("Path", "User")
}

# --- Werkzeuge -------------------------------------------------------------
if (-not (Have git)) {
  Write-Host "== Git fehlt -> installiere mit winget" -ForegroundColor Yellow
  winget install --id Git.Git -e --source winget --accept-package-agreements --accept-source-agreements
  Refresh-Path
}
if (-not (Have gh)) {
  Write-Host "== GitHub CLI fehlt -> installiere mit winget" -ForegroundColor Yellow
  winget install --id GitHub.cli -e --source winget --accept-package-agreements --accept-source-agreements
  Refresh-Path
}

# --- GitHub-Anmeldung (machst du selbst im Browser) ------------------------
gh auth status 2>$null | Out-Null
if ($LASTEXITCODE -ne 0) {
  Write-Host "== Bei GitHub anmelden (Browser oeffnet sich)" -ForegroundColor Yellow
  gh auth login --hostname github.com --git-protocol https --web
}
gh auth setup-git
$login = gh api user --jq .login
$ghid  = gh api user --jq .id
$name  = gh api user --jq '.name // .login'
Write-Host "== GitHub-Konto: $login" -ForegroundColor Green

# --- Workflow an seinen Platz (.github ist fuer Claude schreibgeschuetzt) ---
if (Test-Path "_move_to_github_workflows\firmware.yml") {
  New-Item -ItemType Directory -Force ".github\workflows" | Out-Null
  Move-Item -Force "_move_to_github_workflows\firmware.yml" ".github\workflows\firmware.yml"
  Remove-Item -Recurse -Force "_move_to_github_workflows"
}

# --- Git-Repo ----------------------------------------------------------------
if (-not (Test-Path ".git")) { git init -b main | Out-Null }
if (-not (git config user.name))  { git config user.name  "$name" }
if (-not (git config user.email)) { git config user.email "$ghid+$login@users.noreply.github.com" }
git add -A
git diff --cached --quiet
if ($LASTEXITCODE -ne 0) { git commit -m "Projektstruktur, LED-Test und Uhr-Grundgeruest mit OTA" | Out-Null }

# --- GitHub-Repo anlegen + pushen ------------------------------------------
gh repo view "$login/wordclock" 2>$null | Out-Null
if ($LASTEXITCODE -ne 0) {
  gh repo create wordclock --public --source . --remote origin --push `
    --description "Wordclock fuer Maja - ESP32, WS2812B, OTA ueber GitHub Releases"
} else {
  if (-not (git remote)) { git remote add origin "https://github.com/$login/wordclock.git" }
  git push -u origin main
}
Write-Host "== Fertig: https://github.com/$login/wordclock" -ForegroundColor Green
