# Windows build

Install CMake 3.20+ and Visual Studio 2022 with the x64 C++ workload. From a Developer PowerShell in this directory:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

No external libraries are required for the initial WAV renderer. Windows device playback and GUI are not implemented yet.
