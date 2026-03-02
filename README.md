# PlaneFight (raylib)

## Prerequisites (Windows x64)

1. Install Visual Studio 2022 with C++ workload.
2. Install `vcpkg` and set `VCPKG_ROOT`.
3. Install raylib in vcpkg:

```powershell
vcpkg install raylib:x64-windows
```

## Configure and build

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\\scripts\\buildsystems\\vcpkg.cmake"
cmake --build build --config Debug
```

## Run

```powershell
.\build\Debug\PlaneFight.exe
```
