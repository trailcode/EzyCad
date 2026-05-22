# Copy crawlable HTML from web/ to trailcode.github.io (EzyCad/ subfolder).
# Does not copy .js / .wasm / .data — publish those separately after an Emscripten build.
#
# Usage (from repo root):
#   .\scripts\sync-github-pages-html.ps1 -PagesRepo C:\src\trailcode.github.io
#
# Then commit and push trailcode.github.io.

param(
    [Parameter(Mandatory = $true)]
    [string] $PagesRepo
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$srcWeb = Join-Path $root "web"
$dst = Join-Path $PagesRepo "EzyCad"

if (-not (Test-Path $dst)) {
    throw "Destination not found: $dst (clone trailcode/trailcode.github.io first)"
}

foreach ($name in @("index.html", "EzyCad.html")) {
    $from = Join-Path $srcWeb $name
    if (-not (Test-Path $from)) {
        throw "Missing: $from"
    }
    Copy-Item -Force $from (Join-Path $dst $name)
    Write-Host "Copied $name -> $dst"
}

Write-Host "Done. Commit and push $PagesRepo when ready."
Write-Host "If you also copied a new .js/.wasm/.data build, bump EZYCAD_WEB_CACHE in web/EzyCad.html and re-sync."
