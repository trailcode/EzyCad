# Report any non-ASCII characters (codepoint > U+007F) in C/C++ sources under src/.
# Exits 0 if clean, 1 if any non-ASCII is found (suitable for CI).

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$src = Join-Path $root "src"

if (-not (Test-Path -LiteralPath $src)) {
  Write-Error "Source directory not found: $src"
  exit 2
}

$extensions = @("*.cpp", "*.h", "*.inl")
$files = @()
foreach ($pat in $extensions) {
  $files += Get-ChildItem -LiteralPath $src -Filter $pat -Recurse -File -ErrorAction SilentlyContinue
}
$files = $files | Sort-Object FullName -Unique

$found = 0

foreach ($file in $files) {
  $text = [System.IO.File]::ReadAllText($file.FullName)
  $lineNum = 1
  $lines = $text -split "`r`n|`n|`r", [System.StringSplitOptions]::None
  foreach ($line in $lines) {
    for ($col = 0; $col -lt $line.Length; $col++) {
      $cp = [int][char]$line[$col]
      if ($cp -gt 0x7F) {
        $ch = $line[$col]
        $u = "U+{0:X4}" -f $cp
        $full = $file.FullName
        if ($full.StartsWith($root, [System.StringComparison]::OrdinalIgnoreCase)) {
          $rel = $full.Substring($root.Length).TrimStart([char]'\', [char]'/')
        } else {
          $rel = $full
        }
        Write-Host ("{0}:{1}:{2}: non-ASCII {3} ({4})" -f $rel, $lineNum, ($col + 1), $u, $ch)
        $found++
      }
    }
    $lineNum++
  }
}

if ($found -gt 0) {
  Write-Host ""
  Write-Host "Total non-ASCII character occurrences: $found"
  exit 1
}

Write-Host "No non-ASCII characters in src ($($files.Count) files checked)."
exit 0
