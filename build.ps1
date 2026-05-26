#!/usr/bin/env pwsh
# Cross-platform build wrapper for UILO (Windows PowerShell / pwsh).
# Auto-fetches SDL3 and bgfx via CMake FetchContent when not installed.
#
# Usage:
#   .\build.ps1                  # configure + build (Release)
#   .\build.ps1 debug            # Debug build
#   .\build.ps1 clean            # wipe build/ then rebuild
#   .\build.ps1 clean debug      # clean Debug rebuild
#   $env:UILO_AUTO_FETCH='OFF'; .\build.ps1  # require system SDL3/bgfx

[CmdletBinding()]
param(
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$Args
)

$ErrorActionPreference = 'Stop'
$RootDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $RootDir

$Mode      = 'Release'
$DoClean   = $false
$Targets   = @()

foreach ($a in $Args) {
    switch ($a.ToLower()) {
        'clean'   { $DoClean = $true }
        'debug'   { $Mode = 'Debug' }
        'release' { $Mode = 'Release' }
        default   { $Targets += $a }
    }
}

$BuildDir = Join-Path 'build' $Mode
if ($DoClean -and (Test-Path $BuildDir)) {
    Write-Host "[UILO] cleaning $BuildDir"
    Remove-Item -Recurse -Force $BuildDir
}

# Pick a generator
$Generator = $null
if (Get-Command ninja -ErrorAction SilentlyContinue) {
    $Generator = 'Ninja'
} elseif ($IsWindows -or $env:OS -eq 'Windows_NT') {
    # Fallback to MSVC if installed
    $Generator = 'Visual Studio 17 2022'
} else {
    $Generator = 'Unix Makefiles'
}

$AutoFetch = if ($env:UILO_AUTO_FETCH) { $env:UILO_AUTO_FETCH } else { 'ON' }

Write-Host "[UILO] configure ($Mode, $Generator, AUTO_FETCH=$AutoFetch)"
cmake -S $RootDir -B $BuildDir -G $Generator `
    -DCMAKE_BUILD_TYPE=$Mode `
    -DUILO_AUTO_FETCH=$AutoFetch
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "[UILO] build"
if ($Targets.Count -gt 0) {
    cmake --build $BuildDir --config $Mode --parallel --target $Targets
} else {
    cmake --build $BuildDir --config $Mode --parallel
}
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "[UILO] done -> $BuildDir"
