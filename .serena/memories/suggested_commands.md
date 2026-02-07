# Development Commands

## Build
- Open `raketic.sln` in Visual Studio 2022
- Build configurations: Debug, Release, ReleaseMin, ReleaseSDL, Tests
- Build produces executables in respective output folders (e.g. `Debug/`, `ReleaseMin/`)

## Run
- Run from Visual Studio (F5) or execute the built `.exe` directly
- Debug configuration includes assertion checks and Tracy profiler

## System Utils (Windows/PowerShell)
- `git` for version control
- `dir` or `ls` for directory listing
- `rg` (ripgrep) for text search
- `code` for opening files in VS Code/Cursor

## Model Generation
- `raketic.modelgen` project (C# tool) processes SVG models and Lua data -> generates C code
- Generated files: `engine/generated/models.gen.c`, `renderer.gen.c`, `renderer.gen.h`, `slots.gen.h`

## Testing
- Tests configuration builds with `UNIT_TESTS` define
- Uses Unity test framework (engine/test/)
- Test functions embedded in source files with `#ifdef UNIT_TESTS`
