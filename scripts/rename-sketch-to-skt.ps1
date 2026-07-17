# Rename src/sketch_* (and coordinator sketch.h/cpp) to src/skt_*.
# Script lives at <repo>/scripts/rename-sketch-to-skt.ps1; repo root is one level above scripts/

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$src = Join-Path $root "src"

$renamed = 0

# Coordinator: sketch.h / sketch.cpp -> skt.h / skt.cpp
foreach ($name in @("sketch.h", "sketch.cpp")) {
  $path = Join-Path $src $name
  if (-not (Test-Path -LiteralPath $path)) {
    continue
  }
  $newName = $name -replace "^sketch", "skt"
  $dest = Join-Path $src $newName
  if (Test-Path -LiteralPath $dest) {
    throw "Destination already exists: $dest"
  }
  Rename-Item -LiteralPath $path -NewName $newName
  Write-Host "$name -> $newName"
  $renamed++
}

$files = @(Get-ChildItem -Path $src -File -Filter "sketch_*")
foreach ($file in $files) {
  $newName = $file.Name -replace "^sketch_", "skt_"
  $dest = Join-Path $file.DirectoryName $newName
  if (Test-Path -LiteralPath $dest) {
    throw "Destination already exists: $dest"
  }
  Rename-Item -LiteralPath $file.FullName -NewName $newName
  Write-Host "$($file.Name) -> $newName"
  $renamed++
}

if ($renamed -eq 0) {
  Write-Host "No sketch.h/cpp or sketch_* files found under $src"
  exit 0
}

Write-Host "Renamed $renamed file(s)."
