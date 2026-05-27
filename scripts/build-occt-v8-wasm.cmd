@echo off
REM Build OCCT V8.0.0 for WebAssembly (see docs/building-occt.md).
REM Activate Emscripten first, e.g. call C:\src\emsdk\emsdk_env.bat
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build-occt-v8-wasm.ps1" %*
if errorlevel 1 exit /b 1
