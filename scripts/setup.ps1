# Run with: powershell -ExecutionPolicy Bypass -File scripts/setup.ps1

$ErrorActionPreference = "Stop"

function Info    { param($msg) Write-Host "[nova2d] $msg" -ForegroundColor Cyan }
function Success { param($msg) Write-Host "[nova2d] $msg" -ForegroundColor Green }
function Warn    { param($msg) Write-Host "[nova2d] $msg" -ForegroundColor Yellow }
function Err     { param($msg) Write-Host "[nova2d] $msg" -ForegroundColor Red; exit 1 }

# ── Install Chocolatey if missing ─────────────────────────────────────────────
if (-not (Get-Command choco -ErrorAction SilentlyContinue)) {
    Info "Installing Chocolatey..."
    Set-ExecutionPolicy Bypass -Scope Process -Force
    [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
    Invoke-Expression ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
}

# ── Install cmake and git ─────────────────────────────────────────────────────
Info "Installing cmake and git..."
choco install cmake git -y --no-progress
refreshenv

# ── Install vcpkg ─────────────────────────────────────────────────────────────
$vcpkgDir = "C:\vcpkg"
if (-not (Test-Path "$vcpkgDir\vcpkg.exe")) {
    Info "Installing vcpkg..."
    git clone https://github.com/microsoft/vcpkg.git $vcpkgDir
    & "$vcpkgDir\bootstrap-vcpkg.bat" -disableMetrics
} else {
    Info "vcpkg already installed — updating..."
    git -C $vcpkgDir pull
}

# ── Install deps via vcpkg using our manifest ─────────────────────────────────
Info "Installing dependencies via vcpkg manifest (vcpkg.json)..."
$repoRoot = Split-Path $PSScriptRoot -Parent
& "$vcpkgDir\vcpkg.exe" install `
    --triplet x64-windows `
    --x-manifest-root $repoRoot `
    --x-install-root "$vcpkgDir\installed"

# ── Integrate vcpkg with CMake ────────────────────────────────────────────────
$toolchain = "$vcpkgDir\scripts\buildsystems\vcpkg.cmake"

Success "All dependencies installed."
Write-Host ""
Write-Host "Build with:" -ForegroundColor White
Write-Host "    cmake -B build -DCMAKE_TOOLCHAIN_FILE=$toolchain" -ForegroundColor White
Write-Host "    cmake --build build --config Release" -ForegroundColor White
Write-Host "    .\build\bin\Release\nova2d_game.exe" -ForegroundColor White
Write-Host ""