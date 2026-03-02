@echo off
setlocal

cmake --fresh -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=C:\src\vcpkg\scripts\buildsystems\vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows || exit /b 1
cmake --build build --config Debug || exit /b 1
start "" "%~dp0build\Debug\PlaneFight.exe"
