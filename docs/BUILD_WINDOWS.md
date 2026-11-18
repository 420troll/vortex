# Windows build

The verified machine uses CMake 4.4.4 and Visual Studio 2026 with MSVC 14.51 x64 and Windows SDK 10.0.26100. Generator discovery (`cmake -G`) reported `Visual Studio 18 2026`; the preset selects it explicitly. Use `cmake -G` to check the generator name before adapting the preset on another machine.

From Windows PowerShell in the project directory:

```powershell
cmake --preset windows-vs2026-x64
cmake --build --preset windows-release
ctest --preset windows-release
```

Build outputs go under ignored `build/`. The preset disables automatic vcpkg MSBuild integration for this dependency-free target. The native Windows build should use a Windows filesystem path; building through a WSL UNC path produces Visual Studio path warnings.

The GUI executable is `build/windows-vs2026-x64/Release/TROLLfx-Vortex-Lab.exe`. It uses the Windows subsystem, so double-clicking opens a normal GUI window without a terminal. `vortex-play.exe` remains available as the console/debug player. Both use the Windows SDK's WinMM `waveOut` API and the default output device.
