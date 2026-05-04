# Convert PDF content (often mislabeled as .pbf) to PNG with configurable DPI.
# Requires Python 3 and: pip install pymupdf
#
# Examples:
#   .\pbf-to-png.ps1 -Input drawing.pbf -Dpi 300
#   .\pbf-to-png.ps1 -Input doc.pdf -Output out.png -Dpi 150
#   .\pbf-to-png.ps1 -Input manual.pdf -AllPages -Dpi 200

param(
  [Parameter(Mandatory = $true, Position = 0)]
  [string] $Input,

  [string] $Output,

  [double] $Dpi = 150,

  [switch] $AllPages,

  [int] $Page
)

$here = $PSScriptRoot
$py = Join-Path $here "pbf-to-png.py"
if (-not (Test-Path $py)) {
  Write-Error "Missing script: $py"
  exit 1
}

$argsList = @($py, $Input, "--dpi", $Dpi)
if ($Output) {
  $argsList += @("-o", $Output)
}
if ($AllPages) {
  $argsList += "--all-pages"
}
if ($PSBoundParameters.ContainsKey("Page")) {
  $argsList += @("--page", $Page)
}

& python @argsList
exit $LASTEXITCODE
