#!/usr/bin/env pwsh
# Cross-platform build wrapper for UILO (Windows PowerShell / pwsh).
# Vendors bgfx: clones bx/bimg/bgfx under ext/ and builds them ONCE
# (GENie-generated VS solution + msbuild); CMake links the prebuilt libs.
# SDL3 is auto-fetched via CMake FetchContent when not installed (or use
# vcpkg: `vcpkg install sdl3` + -DCMAKE_TOOLCHAIN_FILE=...).
#
# Requires msbuild on PATH (an "x64 Native Tools" prompt, or VS Build Tools).
#
# Usage:
#   .\build.ps1                  # configure + build (Release)
#   .\build.ps1 debug            # Debug build
#   .\build.ps1 clean            # wipe build/ then rebuild (never touches ext/)
#   .\build.ps1 clean debug      # clean Debug rebuild
#   $env:UILO_AUTO_FETCH='OFF'; .\build.ps1  # require system SDL3

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
    # Deliberately never touches ext/ -- the one-time bgfx build survives cleans.
    Write-Host "[UILO] cleaning $BuildDir"
    Remove-Item -Recurse -Force $BuildDir
}

# ---------------------------------------------------------------------------
# Vendored bgfx: bx/bimg/bgfx MUST be sibling dirs under ext/ (bgfx's build
# references ../bx and ../bimg). Clone all three together -- they version-lock
# against each other. GENie needs no install; bx ships a prebuilt genie.exe.
# ---------------------------------------------------------------------------
$Ext = Join-Path $RootDir 'ext'
New-Item -ItemType Directory -Force -Path $Ext | Out-Null

function Clone-IfMissing([string]$Name, [string]$Url) {
    $Dest = Join-Path $Ext $Name
    if (-not (Test-Path $Dest)) {
        Write-Host "[UILO] cloning $Name into ext/"
        git clone --depth 1 $Url $Dest
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
}
Clone-IfMissing 'bx'   'https://github.com/bkaradzic/bx.git'
Clone-IfMissing 'bimg' 'https://github.com/bkaradzic/bimg.git'
Clone-IfMissing 'bgfx' 'https://github.com/bkaradzic/bgfx.git'

$IsWin = $IsWindows -or $env:OS -eq 'Windows_NT'
if ($IsWin) {
    # One-time build: GENie-generated VS solution + msbuild (bgfx's makefile
    # vs2022 targets shell out to devenv, which needs full VS; msbuild works
    # with Build Tools alone). Always Release, even for Debug UILO builds.
    $BgfxBinDir = Join-Path $Ext 'bgfx\.build\win64_vs2022\bin'
    if (-not (Test-Path (Join-Path $BgfxBinDir 'bgfxRelease.lib'))) {
        Write-Host '[UILO] building bgfx (win64_vs2022) -- one time only'
        $Genie = Join-Path $Ext 'bx\tools\bin\windows\genie.exe'
        Push-Location (Join-Path $Ext 'bgfx')
        try {
            & $Genie --with-tools vs2022
            if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
            msbuild .build\projects\vs2022\bgfx.sln /m `
                /p:Configuration=Release /p:Platform=x64
            if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        } finally {
            Pop-Location
        }
    }
} else {
    # pwsh on macOS/Linux: same make flow as build.sh.
    $Uname = uname -s
    $Arch  = uname -m
    if ($Uname -eq 'Darwin') {
        if ($Arch -eq 'arm64') {
            $BgfxTarget = 'osx-arm64-release';  $BgfxPlatformDir = 'osx-arm64'
        } else {
            $BgfxTarget = 'osx-x64-release';    $BgfxPlatformDir = 'osx-x64'
        }
    } else {
        $BgfxTarget = 'linux-gcc-release64';    $BgfxPlatformDir = 'linux64_gcc'
    }
    $BgfxBinDir = Join-Path $Ext "bgfx/.build/$BgfxPlatformDir/bin"
    if (-not (Test-Path (Join-Path $BgfxBinDir 'libbgfxRelease.a'))) {
        Write-Host "[UILO] building bgfx ($BgfxTarget) -- one time only"
        make -C (Join-Path $Ext 'bgfx') $BgfxTarget
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
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
