#requires -Version 5
# Windows wrapper -- counterpart of examples/build.sh. Delegates to the root
# build.ps1 from within the examples dir. All arguments are forwarded as-is,
# so the interface is identical:
#
#   .\build.ps1                       release build of uilo + all examples
#   .\build.ps1 clean containers      clean + rebuild containers only
#   .\build.ps1 debug uilo containers debug build of library + containers
#
# Extra convenience: pass `run <example>` (e.g. `.\build.ps1 run containers`
# or `.\build.ps1 release run containers`) to build and then execute the
# resulting binary. The example is run from this directory so relative
# asset paths (assets/fonts/...) resolve correctly.
$ErrorActionPreference = 'Stop'
$Here = $PSScriptRoot
$Root = (Resolve-Path (Join-Path $Here '..')).Path

$RunTarget = ''
$Mode      = 'Release'
$Forward   = @()

$i = 0
while ($i -lt $args.Count) {
    $a = $args[$i]
    switch ($a.ToLower()) {
        'run' {
            $i++
            if ($i -ge $args.Count) {
                Write-Error "build.ps1: 'run' requires an example name"
                exit 1
            }
            $RunTarget = $args[$i]
            $Forward  += $RunTarget
        }
        'debug'   { $Mode = 'Debug';   $Forward += $a }
        'release' { $Mode = 'Release'; $Forward += $a }
        default   { $Forward += $a }
    }
    $i++
}

& (Join-Path $Root 'build.ps1') @Forward
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if ($RunTarget) {
    # Multi-config generators (VS) place the exe under bin/<Config>/; a
    # single-config generator places it directly under bin/.
    $Candidates = @(
        (Join-Path $Here "bin/$Mode/$RunTarget.exe"),
        (Join-Path $Here "bin/$RunTarget.exe")
    )
    $Bin = $Candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
    if (-not $Bin) {
        Write-Error "build.ps1: binary not found for '$RunTarget' under $Here\bin"
        exit 1
    }
    Set-Location $Here
    & $Bin
    exit $LASTEXITCODE
}
