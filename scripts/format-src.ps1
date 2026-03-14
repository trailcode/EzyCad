# Format all C/C++ sources under src/ with clang-format.
# Requires LLVM clang-format (e.g. "C:\Program Files\LLVM\bin\clang-format.exe") or clang-format in PATH.

$clang = "C:\Program Files\LLVM\bin\clang-format.exe"
if (-not (Test-Path $clang)) {
  $clang = "clang-format"
}

$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$src = Join-Path $root "src"
Get-ChildItem -Path $src -Include *.cpp,*.h,*.inl -Recurse -File |
  ForEach-Object { & $clang -i $_.FullName; Write-Host $_.Name }
