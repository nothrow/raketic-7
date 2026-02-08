---
name: building
description: Build instructions and configurations for the Raketic engine using MSVC and msbuild
---
# Building Raketic

## How to Build (from PowerShell)

Use `cmd /c` with `VsDevCmd.bat` to initialize MSVC environment and run msbuild in one shot:

```
cmd /c '"z:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x86 -host_arch=amd64 && msbuild <target> /p:Configuration=<config> /p:Platform=Win32'
```

Note the quoting: outer single quotes `'...'` for PowerShell, inner double quotes around the bat path.

### Build engine only

```
cmd /c '"z:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x86 -host_arch=amd64 && msbuild engine\engine.vcxproj /p:Configuration=Debug /p:Platform=Win32'
```

### Build entire solution

```
cmd /c '"z:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x86 -host_arch=amd64 && msbuild raketic.sln /p:Configuration=Debug /p:Platform=x86'
```

### Build modelgen only

```
cmd /c '"z:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x86 -host_arch=amd64 && msbuild raketic.modelgen\raketic.modelgen.csproj /p:Configuration=Debug'
```

## Configurations

| Configuration | Purpose | Platform backends | Defines | Notes |
|---|---|---|---|---|
| **Debug** | Development | Win32/GDI + SDL2/OpenGL | `_DEBUG`, `TRACY_ENABLE` | Full debug overlay, Tracy profiler, console subsystem |
| **ReleaseSDL** | Release with SDL | SDL2/OpenGL | `NDEBUG`, `TRACY_ENABLE` | Optimized (`/Os`), AVX2, Tracy, no debug overlay |
| **ReleaseMin** | Minimal binary | Win32/GDI only | `NDEBUG` | Aggressive size optimization, no CRT, no debug, fastcall, LTO |
| **Tests** | Unit tests | None (console only) | `_DEBUG`, `UNIT_TESTS` | Unity test framework, `main.c` excluded, test files included |

All engine configurations target **Win32** platform. The modelgen (C#) uses **Any CPU**.

## Output

- Engine executables: `engine\Debug\`, `engine\ReleaseSDL\`, `engine\ReleaseMin\`, `engine\Tests\`
- Modelgen: `raketic.modelgen\bin\Debug\` or `\Release\`

## Dependencies

Managed by **vcpkg** (manifest mode via `engine\vcpkg.json`):

- SDL2
- Tracy (>= 0.11.1)

vcpkg dependencies are installed automatically during the first MSBuild build.

## Common Workflows

### Full rebuild

```
msbuild raketic.sln /p:Configuration=Debug /p:Platform=x86 /t:Rebuild
```

### Clean

```
msbuild raketic.sln /p:Configuration=Debug /p:Platform=x86 /t:Clean
```

### Build and run tests

```
msbuild engine\engine.vcxproj /p:Configuration=Tests /p:Platform=Win32
engine\Tests\engine.exe
```

## Important Notes

- Engine is compiled as **C** (not C++), C17 standard
- Warnings treated as errors (`/WX`) at Warning Level 4 (`/W4`)
- Exception handling is disabled (`/EHsc` off)
- Generated code in `engine\generated\` must exist before building (produced by modelgen)
