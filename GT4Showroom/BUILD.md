# Build and verify

Target: classic SA1.0 US Win32, C++17. Supply an LLVM-MinGW UCRT toolchain
with `i686-w64-mingw32-clang.exe` and `clang++.exe`; the release used
LLVM-MinGW20261006/Clang23. PowerShell example:

```powershell
./build.ps1 -CompilerRoot 'C:/Tools/llvm-mingw' -RunTests
```

The build writes `install/GT4Showroom.asi`. MinHook C source and its license are
included. Windows SDK libraries are supplied by the toolchain. No GTA binaries,
installed runtime, CLEO, MoonLoader or ImGui SDK is required to build.

Nineteen isolated suites plus a separate-DLL MinHook integration suite run when
`-RunTests` is set. They use controlled services and do not start GTA. Font/audio
tests read the packaged asset files. The optional preview driver uses the actual
production renderer and StudioControls with private locally-exported mesh fixtures;
those copyrighted vehicle models/textures are not distributed as source fixtures.
See the driver arguments in tools/preview_studio_v220.cpp.
