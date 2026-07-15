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
| **Visual Studio 2022** (Community is fine) | Must include the **"Desktop development with C++"** workload. This provides the MSVC toolchain, the **Windows 10/11 SDK** (needed for `gdi32`/`user32`/`psapi` and, importantly, `d3dcompiler_47.dll` which bgfx's `shaderc` loads to compile the D3D shaders), and MSBuild. |
| **CMake 3.16+** | 3.20+ recommended. The VS installer can add it, or install standalone and ensure it's on `PATH`. |
| **Python 3** | Required by bgfx's build (GENie codegen steps). Ensure `python` is on `PATH`. |
| **Git** | Used to clone the vendored dependencies. |

You do **not** need a separate DirectX SDK — the DirectX headers/libs are vendored
inside bgfx, and the CMake already links `gdi32 psapi user32` on Windows.

All commands below assume the **"x64 Native Tools Command Prompt for VS 2022"** or a
PowerShell where you've run the VS dev-shell import (so `msbuild` and `cl` are on
`PATH`). The easiest reliable option: open **"x64 Native Tools Command Prompt for
VS 2022"** from the Start menu, then launch `powershell` from inside it.

---

## 2. Required source fix: shader include paths

This is a real bug that will break the Windows build regardless of how you drive
it, so fix it first.

### Why

The shaders are compiled to embedded C headers at configure/build time by
`bgfx_compile_shaders` (`cmake/bgfxToolUtils.cmake`). That helper names each
output directory after the **profile**, mapping:

- `s_5_0` → `dxbc/`  (Direct3D11 bytecode)
- `s_6_0` → `dxil/`  (Direct3D12 IL)

(see `_bgfx_get_profile_path_ext` at `cmake/bgfxToolUtils.cmake:542-543`).

`CMakeLists.txt:154-156` already adds `s_5_0` and `s_6_0` to the profile list on
Windows, so the build produces `generated/shaders/dxbc/*.bin.h` and
`generated/shaders/dxil/*.bin.h`.

But `include/renderer/Renderer.cpp:65-73` includes them from a **`dx11/`**
directory that is never generated:

```cpp
#if BX_PLATFORM_WINDOWS
#  include "dx11/vs_solid.sc.bin.h"   // <-- no such directory
   ...
#endif
```

On top of that, on Windows bgfx's `BGFX_EMBEDDED_SHADER(...)` macro references
**both** a `<name>_dxbc` array *and* a `<name>_dxil` array (Direct3D11 and
Direct3D12 are both "supported" platforms in `embedded_shader.h`). So the file
must include **both** the `dxbc/` and `dxil/` headers, which together define
`vs_solid_dxbc`, `vs_solid_dxil`, etc. The current single `dx11/` block satisfies
neither.

### Fix

Replace the `#if BX_PLATFORM_WINDOWS` shader block in
`include/renderer/Renderer.cpp` (lines 65-73) with:

```cpp
#if BX_PLATFORM_WINDOWS
#  include "dxbc/vs_solid.sc.bin.h"
#  include "dxbc/vs_tex.sc.bin.h"
#  include "dxbc/fs_solid.sc.bin.h"
#  include "dxbc/fs_tex.sc.bin.h"
#  include "dxbc/fs_text.sc.bin.h"
#  include "dxbc/fs_blur.sc.bin.h"
#  include "dxbc/fs_glass.sc.bin.h"

#  include "dxil/vs_solid.sc.bin.h"
#  include "dxil/vs_tex.sc.bin.h"
#  include "dxil/fs_solid.sc.bin.h"
#  include "dxil/fs_tex.sc.bin.h"
#  include "dxil/fs_text.sc.bin.h"
#  include "dxil/fs_blur.sc.bin.h"
#  include "dxil/fs_glass.sc.bin.h"
#endif
```

No CMake change is needed with this approach, since the CMake already compiles
both `s_5_0` and `s_6_0`.

> **Alternative (Direct3D11 only, simpler headers):** if you'd rather not carry the
> D3D12 IL, you can instead force DXIL off and keep only `dxbc/`. Add, near the top
> of `Renderer.cpp` alongside the other `BGFX_PLATFORM_SUPPORTS_*` defines
> (before `#include <bgfx/embedded_shader.h>`):
> ```cpp
> #define BGFX_PLATFORM_SUPPORTS_DXIL 0
> ```
> then include only the seven `dxbc/...` headers above, and drop `s_6_0` from
> `_UILO_SHADER_PROFILES` in `CMakeLists.txt:155`. bgfx defaults to Direct3D11 on
> Windows, so this loses nothing in practice. The dual-include fix above is
> recommended because it leaves the D3D12 path intact.

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

### 3c. Build the Release x64 libraries + shaderc

```powershell
msbuild .build\projects\vs2022\bgfx.sln /p:Configuration=Release /p:Platform=x64 /m
```

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
cmake -S . -B build/Release-static -G "Visual Studio 17 2022" -A x64 -DUILO_SHARED=OFF
cmake --build build/Release-static --config Release --parallel
```

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
# Run from an "x64 Native Tools Command Prompt for VS 2022" (so msbuild is on PATH),
# e.g.:  powershell -ExecutionPolicy Bypass -File build.ps1 release static
#
# Usage:
#   .\build.ps1                    # Release static
#   .\build.ps1 debug              # Debug static
#   .\build.ps1 release dynamic    # Release shared (DLL)
#   .\build.ps1 clean release      # wipe build dir then rebuild (never touches ext/)
$ErrorActionPreference = 'Stop'

$Root = $PSScriptRoot
Set-Location $Root

$Mode  = 'Release'
$Link  = 'static'
$Clean = $false
$Extra = @()
foreach ($a in $args) {
    switch ($a.ToLower()) {
        'clean'                    { $Clean = $true }
        'debug'                    { $Mode  = 'Debug' }
        'release'                  { $Mode  = 'Release' }
        'static'                   { $Link  = 'static' }
        { $_ -in 'dynamic','shared' } { $Link = 'dynamic' }
        default                    { $Extra += $a }
    }
}
$Shared  = if ($Link -eq 'dynamic') { 'ON' } else { 'OFF' }
$BuildDir = "build/$Mode-$Link"

if ($Clean -and (Test-Path $BuildDir)) {
    Write-Host "[UILO] cleaning $BuildDir"
    Remove-Item -Recurse -Force $BuildDir
}

# --- vendored dependencies -------------------------------------------------
$Ext = Join-Path $Root 'ext'
New-Item -ItemType Directory -Force -Path $Ext | Out-Null

function Clone-IfMissing($name, $url, $tag) {
    $dst = Join-Path $Ext $name
    if (-not (Test-Path $dst)) {
        Write-Host "[UILO] cloning $name into ext/"
        if ($tag) { git clone --depth 1 --branch $tag $url $dst }
        else      { git clone --depth 1 $url $dst }
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
    if ($LASTEXITCODE -ne 0) { throw "genie failed" }
    msbuild .build/projects/vs2022/bgfx.sln /p:Configuration=Release /p:Platform=x64 /m
    if ($LASTEXITCODE -ne 0) { throw "bgfx msbuild failed" }
    Pop-Location
}

# --- configure + build UILO ------------------------------------------------
Write-Host "[UILO] configure ($Mode, $Link)"
cmake -S $Root -B $BuildDir -G "Visual Studio 17 2022" -A x64 -DUILO_SHARED=$Shared
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

Run it from a VS x64 dev prompt:

```powershell
powershell -ExecutionPolicy Bypass -File build.ps1 release static
```

---

## 6. Summary checklist

- [ ] Install VS 2022 (+ C++ workload / Windows SDK), CMake, Python 3, Git.
- [ ] **Apply the shader-include fix** in `include/renderer/Renderer.cpp` ([§2](#2-required-source-fix-shader-include-paths)).
- [ ] Clone SDL3 / bx / bimg / bgfx into `ext/` ([§3a](#3a-clone-the-dependencies-if-ext-is-empty)).
- [ ] Build bgfx once with GENie + MSBuild ([§3b](#3b-generate-the-vs2022-project-files)–[§3c](#3c-build-the-release-x64-libraries--shaderc)).
- [ ] `cmake` configure + build UILO from a VS x64 dev prompt ([§4](#4-configure-and-build-uilo)).
- [ ] (Optional) Drop in `build.ps1` ([§5](#5-automated-build-buildps1)) to automate steps 3–4.

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
