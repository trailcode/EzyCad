# Rename src/sketch_* files to src/skt_*.
# Script lives at <repo>/scripts/rename-sketch-to-skt.ps1; repo root is one level above scripts/

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$src = Join-Path $root "src"

$files = Get-ChildItem -Path $src -File -Filter "sketch_*"
if ($files.Count -eq 0) {
  Write-Host "No sketch_* files found under $src"
  exit 0
}

foreach ($file in $files) {
  $newName = $file.Name -replace "^sketch_", "skt_"
  $dest = Join-Path $file.DirectoryName $newName
  if (Test-Path $dest) {
    throw "Destination already exists: $dest"
  }
  Rename-Item -LiteralPath $file.FullName -NewName $newName
  Write-Host "$($file.Name) -> $newName"
}

Write-Host "Renamed $($files.Count) file(s)."
