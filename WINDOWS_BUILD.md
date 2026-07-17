# Building UILO on Windows

This document describes everything needed to build UILO on Windows. The C++
sources are already largely Windows-ready (the renderer has a `SDL_PLATFORM_WIN32`
path, the Mac-only shims are stubbed out on non-Apple platforms via
`MacStubs.cpp`, and `CMakeLists.txt` already has `WIN32`/`MSVC` branches). The two
things that are actually missing are:

1. **A Windows build driver.** `build.sh` is bash-only and explicitly bails on
   Windows with *"on Windows use build.bat / build.ps1"* — but neither script
   exists yet. bgfx is built on macOS/Linux with `make`, which isn't available on
   Windows; there it must be built with **GENie + MSBuild**.
2. **One source bug** in the Windows shader include paths (`Renderer.cpp` includes
   a `dx11/` directory that the build never produces). See
   [§2](#2-required-source-fix-shader-include-paths).

Work through the sections in order. [§5](#5-automated-build-buildps1) gives a
`build.ps1` that automates the whole flow once the fix in §2 is applied.

---

## 1. Prerequisites

Install the following (once):

| Tool | Notes |
|------|-------|
| **Visual Studio 2022 or 2026** (Community is fine) | Must include the **"Desktop development with C++"** workload. This provides the MSVC toolchain, the **Windows 10/11 SDK** (needed for `gdi32`/`user32`/`psapi` and, importantly, `d3dcompiler_47.dll` which bgfx's `shaderc` loads to compile the D3D shaders), and MSBuild. **This workload also bundles CMake and Ninja**, so you don't need to install them separately. |
| **Python 3** | Required by bgfx's build (GENie codegen steps). Ensure `python` is on `PATH`. |
| **Git** | Used to clone the vendored dependencies. |

You do **not** need a separate DirectX SDK — the DirectX headers/libs are vendored
inside bgfx, and the CMake already links `gdi32 psapi user32` on Windows. You also
don't need a standalone CMake install; the C++ workload ships one (VS 2026 bundles
CMake 4.x under `Common7\IDE\CommonExtensions\Microsoft\CMake\`).

> This guide has been validated against **Visual Studio Community 2026 (v18)** with
> MSVC toolset **v145** (14.51) and Windows SDK **10.0.26100.0**. On VS 2022 the
> flow is identical except the toolset is `v143` and the CMake generator is
> `"Visual Studio 17 2022"` — see the notes inline.

### Activating the compiler in your shell

**This is the thing that trips people up:** after installing Visual Studio,
`cl`, `msbuild`, and `cmake` are **deliberately not on your global `PATH`**. They
only become available in a shell that has had the VS "developer environment"
imported. Running `cl` in a plain PowerShell/CMD and getting *"not recognized"* is
expected — nothing is broken. You have two ways to get a working shell:

**Option A — use the Start-menu shortcut (easiest).** Search the Start menu for
**"Developer PowerShell for VS 2026"** (or *"x64 Native Tools Command Prompt for
VS 2026"*) and run everything from there. It's a normal shell with the toolchain
pre-activated.

**Option B — activate an existing PowerShell.** Dot-source VS's dev-shell launcher.
For VS 2026 Community:

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\Launch-VsDevShell.ps1" -Arch amd64 -HostArch amd64 -SkipAutomaticLocation
```

(For VS 2022 the path uses `...\Microsoft Visual Studio\2022\Community\...`. A
harmless `'vswhere.exe' is not recognized` warning may print — activation still
succeeds.) After this, `cl`, `msbuild`, `cmake`, and `ninja` all resolve. Verify
with `cl` (prints the compiler banner) or `where.exe cl`.

The `build.ps1` in [§5](#5-automated-build-buildps1) activates this automatically,
so if you use it you can run it from any shell.

All the `msbuild`/`cmake` commands in the sections below must be run from an
activated shell.

---

## 2. Source changes (already applied)

Getting Windows to build **and run** took four source fixes. These are already
committed to the repo — this section documents *why* they exist so nobody
reintroduces the bugs. If you're building from a checkout that already has them,
skip to [§3](#3-one-time-dependency-build-bgfx). Windows uses **Direct3D11 (DXBC)
shaders only**; the Direct3D12 (DXIL) path is deliberately disabled (see below).

### 2a. `BX_PLATFORM_WINDOWS` must be defined before the shader-backend guards

The top of `Renderer.cpp` disables embedded-shader backends it doesn't ship:

```cpp
#if !defined(BX_PLATFORM_WINDOWS) || !BX_PLATFORM_WINDOWS
#  define BGFX_PLATFORM_SUPPORTS_DXBC 0
#endif
```

But `BX_PLATFORM_WINDOWS` comes from `<bx/platform.h>`, which nothing had included
yet at that point (`bgfx.h` pulls in only `defines.h`). So the macro read as
*undefined*, the guard was true, and **DXBC got disabled on Windows** — leaving the
embedded-shader table with no Direct3D11 entry. The build succeeded but every run
died with `Failed to create shaders (renderer=Direct3D 11)`. On macOS/Linux the bug
is invisible (disabling DXBC is correct there). **Fix:** `#include <bx/platform.h>`
before the guard block.

### 2b. Shader include paths: `dx11/` → `dxbc/`

`bgfx_compile_shaders` names each output dir after the profile —
`s_5_0` → `dxbc/` (see `_bgfx_get_profile_path_ext`,
`cmake/bgfxToolUtils.cmake:542`) — but `Renderer.cpp` included them from a `dx11/`
directory that is never generated. The `#if BX_PLATFORM_WINDOWS` block now includes
the seven `dxbc/*.sc.bin.h` headers (matching the `vs_solid_dxbc` … array names the
`BGFX_EMBEDDED_SHADER` macro references for `RendererType::Direct3D11`).

### 2c. Direct3D12 / DXIL disabled

`BGFX_EMBEDDED_SHADER` would otherwise also reference `<name>_dxil` arrays for
Direct3D12. Compiling those (`s_6_0`) requires Microsoft's **DXC** compiler, which
isn't in a stock VS install — `shaderc` fails with *"Unable to load DXC compiler"*.
Since bgfx defaults to Direct3D11 on Windows anyway, DXIL is turned off rather than
adding a DXC dependency:

- `Renderer.cpp`: `#define BGFX_PLATFORM_SUPPORTS_DXIL 0` (before
  `<bgfx/embedded_shader.h>`), and no `dxil/` includes.
- `CMakeLists.txt`: the Windows profile list is `s_5_0` only (no `s_6_0`).

> If you ever need the D3D12 backend, install the DirectX Shader Compiler (DXC), add
> `s_6_0` back to the profiles, re-enable DXIL, and include the `dxil/*` headers
> alongside the `dxbc/*` ones.

### 2d. MSVC compiler flags bx requires

bx's `platform.h` hard-errors on MSVC unless the compiler reports the true C++
standard and uses the conforming preprocessor. `CMakeLists.txt` adds these as
**PUBLIC** options on the `uilo` target (so consumers that include the vendored
bgfx/bx headers get them too):

```cmake
target_compile_options(uilo PUBLIC /Zc:__cplusplus /Zc:preprocessor /utf-8)
```

---

## 3. One-time dependency build (bgfx)

UILO vendors SDL3, bx, bimg, and bgfx under `ext/`. SDL3 is built by CMake as part
of the UILO configure step, so you only have to hand-build **bgfx** once. On
Windows that means GENie (to generate a VS solution) + MSBuild.

The GENie binary is already vendored at
`ext\bx\tools\bin\windows\genie.exe`.

### 3a. Clone the dependencies (if `ext/` is empty)

`ext/SDL3`, `ext/bx`, `ext/bimg`, `ext/bgfx` are git-ignored and must be cloned at
the same versions `build.sh` uses:

```powershell
cd <UILO root>
git clone --depth 1 --branch release-3.2.10 https://github.com/libsdl-org/SDL.git  ext/SDL3
git clone --depth 1 https://github.com/bkaradzic/bx.git   ext/bx
git clone --depth 1 https://github.com/bkaradzic/bimg.git ext/bimg
git clone --depth 1 https://github.com/bkaradzic/bgfx.git ext/bgfx
```

> bx/bimg/bgfx **must** be siblings under `ext/` — bgfx's build references `../bx`
> and `../bimg`.

### 3b. Generate the VS2022 project files

From the bgfx directory, run the vendored GENie against bgfx's genie script. Pass
`--with-tools` so `shaderc` (the shader compiler UILO needs) is included in the
solution:

```powershell
cd ext/bgfx
..\bx\tools\bin\windows\genie.exe --with-tools --file=scripts/genie.lua vs2022
```

This creates `ext\bgfx\.build\projects\vs2022\bgfx.sln`.

> **Note:** this GENie version has no `vs2026` action — `vs2022` is the newest it
> emits, and it pins the projects to the **v143** toolset. That's handled in the
> next step by retargeting at build time; you do **not** need to install the old
> VS 2022 build tools.

### 3c. Build the Release x64 libraries + shaderc

The generated projects target `v143` (VS 2022). On **VS 2026** that toolset isn't
installed, so retarget to your installed toolset (`v145`) and current SDK on the
MSBuild command line — no file edits needed:

```powershell
msbuild .build\projects\vs2022\bgfx.sln /p:Configuration=Release /p:Platform=x64 `
  /p:PlatformToolset=v145 /p:WindowsTargetPlatformVersion=10.0.26100.0 /m
```

> **On VS 2022** the projects already match your toolset, so drop the two
> `/p:` overrides:
> ```powershell
> msbuild .build\projects\vs2022\bgfx.sln /p:Configuration=Release /p:Platform=x64 /m
> ```
> If you get `MSB8020: build tools ... v143 cannot be found`, that's exactly the
> toolset mismatch — add the `/p:PlatformToolset=` / `/p:WindowsTargetPlatformVersion=`
> overrides shown above, using whatever toolset you have installed (check
> `MSBuild\Microsoft\VC\<ver>\Platforms\x64\PlatformToolsets\`).

Output lands in `ext\bgfx\.build\win64_vs2022\bin\`:

- `bgfxRelease.lib`
- `bimgRelease.lib`
- `bimg_decodeRelease.lib`
- `bxRelease.lib`
- `shadercRelease.exe`

These are exactly the names/locations `CMakeLists.txt` expects
(`UILO_BGFX_PLATFORM_DIR` is set to `win64_vs2022` on `WIN32`, and `find_library`
looks for `bgfxRelease` etc.). bgfx is always built **Release** even for Debug
UILO builds — you debug UILO, not bgfx.

> If MSBuild can't be found, you're not in a VS dev environment. Use the "x64
> Native Tools Command Prompt for VS 2022", or build the whole solution by
> right-clicking → Build in Visual Studio (set config to Release / x64 first).

---

## 4. Configure and build UILO

From the UILO root, with the §2 fix applied and bgfx built:

```powershell
cmake -S . -B build/Release-static -G "Visual Studio 18 2026" -A x64 -DUILO_SHARED=OFF
cmake --build build/Release-static --config Release --parallel
```

> On **VS 2022** use `-G "Visual Studio 17 2022"` instead. UILO's own sources are
> compiled by CMake with the default (installed) toolset, so no `/p:PlatformToolset`
> override is needed here — that was only for bgfx's pre-generated projects. Because
> bgfx was built with `v145` `/MT`, UILO must use the same toolset family and static
> CRT; the default VS 2026 toolchain does exactly that.

- `-DUILO_SHARED=ON` builds a DLL instead of a static lib (`WINDOWS_EXPORT_ALL_SYMBOLS`
  is already set, so no `__declspec` annotations are required).
- The library is emitted to `<UILO>/lib` (`uilo.lib`, or `uilo.dll` + import lib for
  shared).
- Example executables land in `examples/bin/` when `UILO_BUILD_EXAMPLES=ON` (the
  default).

### MSVC runtime note

`CMakeLists.txt:24-28` forces the **static** CRT (`/MT`, `/MTd` for Debug) because
bgfx's GENie build uses the static runtime. Everything in the project must match or
you'll get `LNK2038` "RuntimeLibrary mismatch" errors. This is already handled for
UILO and SDL3; just don't override `CMAKE_MSVC_RUNTIME_LIBRARY` yourself.

If you build bgfx **Debug** for any reason, its libs use `/MTd` and won't link
against a Release UILO — keep bgfx on Release as described in §3.

---

## 5. Automated build (`build.ps1`)

Save the following as `build.ps1` in the UILO root. It's the Windows counterpart of
`build.sh`: it clones the vendored deps, builds bgfx once (gated on the output
archive existing, like `build.sh`), then configures and builds UILO. It still
requires the §2 source fix.

```powershell
#requires -Version 5
# Windows build wrapper for UILO — counterpart of build.sh.
# Can be run from ANY PowerShell: it locates Visual Studio via vswhere, imports the
# developer environment itself, auto-detects the MSVC toolset + Windows SDK, builds
# bgfx once (GENie + MSBuild), then configures and builds UILO.
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

if (-not (Get-Command cl -ErrorAction SilentlyContinue)) {
    Write-Host "[UILO] activating VS developer environment ($vsPath)"
    & "$vsPath\Common7\Tools\Launch-VsDevShell.ps1" -Arch amd64 -HostArch amd64 -SkipAutomaticLocation *> $null
    Set-Location $Root
}

# Pick the newest installed platform toolset + Windows SDK (retargets bgfx's
# v143 vs2022 projects onto whatever this machine actually has, e.g. v145).
$Toolset = (Get-ChildItem "$vsPath\MSBuild\Microsoft\VC\*\Platforms\x64\PlatformToolsets" `
    -Directory -ErrorAction SilentlyContinue | Where-Object Name -match '^v\d+$' |
    Sort-Object Name | Select-Object -Last 1).Name
$WinSdk  = (Get-ChildItem "C:\Program Files (x86)\Windows Kits\10\bin" -Directory `
    -ErrorAction SilentlyContinue | Where-Object Name -match '^10\.' |
    Sort-Object Name | Select-Object -Last 1).Name
if (-not $Toolset) { throw "No MSVC platform toolset found." }
Write-Host "[UILO] toolset=$Toolset  winsdk=$WinSdk"

# Newest VS-generator CMake knows about (VS 18 2026 / VS 17 2022 / ...).
$Generator = (cmake --help 2>$null | Select-String '^\*?\s*Visual Studio \d+ \d+' |
    ForEach-Object { ($_ -replace '^\*?\s*','') -replace '\s*=.*$','' } |
    Select-Object -First 1)
if (-not $Generator) { throw "No Visual Studio CMake generator available." }
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
    & ../bx/tools/bin/windows/genie.exe --with-tools --file=scripts/genie.lua vs2022
    if ($LASTEXITCODE -ne 0) { Pop-Location; throw "genie failed" }
    $msbArgs = @('.build/projects/vs2022/bgfx.sln','/p:Configuration=Release','/p:Platform=x64',
                 "/p:PlatformToolset=$Toolset",'/m')
    if ($WinSdk) { $msbArgs += "/p:WindowsTargetPlatformVersion=$WinSdk" }
    msbuild @msbArgs
    if ($LASTEXITCODE -ne 0) { Pop-Location; throw "bgfx msbuild failed" }
    Pop-Location
}

# --- configure + build UILO ------------------------------------------------
Write-Host "[UILO] configure ($Mode, $Link)"
cmake -S $Root -B $BuildDir -G $Generator -A x64 -DUILO_SHARED=$Shared
if ($LASTEXITCODE -ne 0) { throw "cmake configure failed" }

Write-Host "[UILO] build"
if ($Extra.Count -gt 0) {
    cmake --build $BuildDir --config $Mode --parallel --target $Extra
} else {
    cmake --build $BuildDir --config $Mode --parallel
}
if ($LASTEXITCODE -ne 0) { throw "cmake build failed" }
Write-Host "[UILO] done -> $BuildDir"
```

Run it from **any** PowerShell (it self-activates the VS environment):

```powershell
powershell -ExecutionPolicy Bypass -File build.ps1 release static
```

---

## 6. Summary checklist

**Fastest path:** open any PowerShell and run `.\build.ps1` — it self-activates the
VS environment, builds bgfx once, and builds UILO. The source fixes in
[§2](#2-source-changes-already-applied) are already committed. The manual steps
below are only needed if you'd rather not use the script:

- [ ] Install VS 2022/2026 (+ C++ workload, which includes the Windows SDK, CMake, and Ninja), Python 3, Git.
- [ ] Open a **Developer PowerShell** (or activate one — [§1](#activating-the-compiler-in-your-shell)) so `cl`/`msbuild`/`cmake` resolve.
- [ ] Clone SDL3 / bx / bimg / bgfx into `ext/` ([§3a](#3a-clone-the-dependencies-if-ext-is-empty)).
- [ ] Build bgfx once with GENie + MSBuild, retargeting to your toolset ([§3b](#3b-generate-the-vs2022-project-files)–[§3c](#3c-build-the-release-x64-libraries--shaderc)).
- [ ] `cmake` configure + build UILO ([§4](#4-configure-and-build-uilo)).

## 7. Optional follow-ups (not required to build)

- Update `README.md`'s "Requirements" section to list the Windows prerequisites and
  point at this document / `build.ps1`.
- Update `build.sh`'s Windows error message (line ~104) — it already tells users to
  run `build.ps1`, so committing the script closes that gap.
- The examples build as **console** apps (`int main()`), so a console window opens
  alongside the UILO window on Windows. Harmless; if you want a windowed subsystem
  later you'd add `SDL3main` + `/SUBSYSTEM:WINDOWS`.
</content>
</invoke>
