#requires -Version 5
# Windows build wrapper for UILO -- counterpart of build.sh.
# Runs from ANY PowerShell: locates Visual Studio via vswhere, imports the
# developer environment itself, auto-detects the MSVC toolset + Windows SDK,
# builds bgfx once (GENie + MSBuild), then configures and builds UILO.
#
# Usage:
#   .\build.ps1                    # Release static
#   .\build.ps1 debug              # Debug static
#   .\build.ps1 release dynamic    # Release shared (DLL)
#   .\build.ps1 clean release      # wipe build dir then rebuild (never touches ext/)
$ErrorActionPreference = 'Stop'
$Root = $PSScriptRoot
Set-Location $Root

$Mode = 'Release'; $Link = 'static'; $Clean = $false; $Extra = @()
foreach ($a in $args) {
    switch ($a.ToLower()) {
        'clean'                       { $Clean = $true }
        'debug'                       { $Mode  = 'Debug' }
        'release'                     { $Mode  = 'Release' }
        'static'                      { $Link  = 'static' }
        { $_ -in 'dynamic','shared' } { $Link  = 'dynamic' }
        default                       { $Extra += $a }
    }
}
$Shared = if ($Link -eq 'dynamic') { 'ON' } else { 'OFF' }
$BuildDir = "build/$Mode-$Link"

# --- locate Visual Studio and import the developer environment -------------
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) { throw "vswhere not found -- is Visual Studio installed?" }
$vsPath = & $vswhere -latest -products * `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -property installationPath
if (-not $vsPath) { throw "No VS install with the C++ toolset found." }

# Always import the dev env (sets PATH + INCLUDE + LIB + LIBPATH together).
if (-not (Get-Command msbuild -ErrorAction SilentlyContinue) -or -not $env:INCLUDE) {
    Write-Host "[UILO] activating VS developer environment ($vsPath)"
    & "$vsPath\Common7\Tools\Launch-VsDevShell.ps1" -Arch amd64 -HostArch amd64 -SkipAutomaticLocation *> $null
    Set-Location $Root
}

# Prefer the CMake/Ninja bundled with VS if a standalone one isn't on PATH.
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    $bundledCMake = Join-Path $vsPath 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin'
    if (Test-Path (Join-Path $bundledCMake 'cmake.exe')) { $env:PATH = "$bundledCMake;$env:PATH" }
}

# Pick the newest installed platform toolset + Windows SDK (retargets bgfx's
# v143 vs2022 projects onto whatever this machine actually has, e.g. v145).
$Toolset = (Get-ChildItem "$vsPath\MSBuild\Microsoft\VC" -Directory -ErrorAction SilentlyContinue |
    ForEach-Object { Get-ChildItem "$($_.FullName)\Platforms\x64\PlatformToolsets" -Directory -ErrorAction SilentlyContinue } |
    Where-Object Name -match '^v\d+$' | Sort-Object Name | Select-Object -Last 1).Name
$WinSdk  = (Get-ChildItem "C:\Program Files (x86)\Windows Kits\10\bin" -Directory `
    -ErrorAction SilentlyContinue | Where-Object Name -match '^10\.' |
    Sort-Object Name | Select-Object -Last 1).Name
if (-not $Toolset) { throw "No MSVC platform toolset found." }
Write-Host "[UILO] toolset=$Toolset  winsdk=$WinSdk"

# Newest VS-generator CMake knows about (VS 18 2026 / VS 17 2022 / ...).
$Generator = (cmake --help 2>$null | Select-String '^\*?\s*Visual Studio \d+ \d+' |
    ForEach-Object { ($_.ToString() -replace '^\*?\s*','') -replace '\s*=.*$','' } |
    Select-Object -First 1)
if (-not $Generator) { throw "No Visual Studio CMake generator available." }
$Generator = $Generator.Trim()
Write-Host "[UILO] cmake generator: $Generator"

if ($Clean -and (Test-Path $BuildDir)) {
    Write-Host "[UILO] cleaning $BuildDir"; Remove-Item -Recurse -Force $BuildDir
}

# --- vendored dependencies -------------------------------------------------
$Ext = Join-Path $Root 'ext'
New-Item -ItemType Directory -Force -Path $Ext | Out-Null
function Clone-IfMissing($name, $url, $tag) {
    $dst = Join-Path $Ext $name
    if (-not (Test-Path $dst)) {
        Write-Host "[UILO] cloning $name into ext/"
        if ($tag) { git clone --depth 1 --branch $tag $url $dst } else { git clone --depth 1 $url $dst }
        if ($LASTEXITCODE -ne 0) { throw "git clone $name failed" }
    }
}
Clone-IfMissing 'SDL3' 'https://github.com/libsdl-org/SDL.git'  'release-3.2.10'
Clone-IfMissing 'bx'   'https://github.com/bkaradzic/bx.git'    $null
Clone-IfMissing 'bimg' 'https://github.com/bkaradzic/bimg.git'  $null
Clone-IfMissing 'bgfx' 'https://github.com/bkaradzic/bgfx.git'  $null

# --- one-time bgfx build (GENie + MSBuild, Release x64) --------------------
$BgfxBin = Join-Path $Ext 'bgfx/.build/win64_vs2022/bin'
if (-not (Test-Path (Join-Path $BgfxBin 'bgfxRelease.lib'))) {
    Write-Host "[UILO] building bgfx (win64_vs2022, Release) -- one time only"
    Push-Location (Join-Path $Ext 'bgfx')
    try {
        & ../bx/tools/bin/windows/genie.exe --with-tools --file=scripts/genie.lua vs2022
        if ($LASTEXITCODE -ne 0) { throw "genie failed" }
        $msbArgs = @('.build/projects/vs2022/bgfx.sln','/p:Configuration=Release','/p:Platform=x64',
                     "/p:PlatformToolset=$Toolset",'/m')
        if ($WinSdk) { $msbArgs += "/p:WindowsTargetPlatformVersion=$WinSdk" }
        msbuild @msbArgs
        if ($LASTEXITCODE -ne 0) { throw "bgfx msbuild failed" }
    } finally { Pop-Location }
}

# --- configure + build UILO ------------------------------------------------
Write-Host "[UILO] configure ($Mode, $Link)"
cmake -S $Root -B $BuildDir -G $Generator -A x64 "-DUILO_SHARED=$Shared"
if ($LASTEXITCODE -ne 0) { throw "cmake configure failed" }

Write-Host "[UILO] build"
if ($Extra.Count -gt 0) {
    cmake --build $BuildDir --config $Mode --parallel --target $Extra
} else {
    cmake --build $BuildDir --config $Mode --parallel
}
if ($LASTEXITCODE -ne 0) { throw "cmake build failed" }
Write-Host "[UILO] done -> $BuildDir"
