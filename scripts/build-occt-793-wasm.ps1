# Build OCCT V7_9_3 (7.9.3) for WebAssembly.
# Recommended for EzyCad wasm until OCCT 8.x GLES shading regressions are resolved.
# Wrapper around scripts/build-occt-wasm.ps1 with versioned default paths.
#
# Usage (from repo root, after emsdk_env):
#   .\scripts\build-occt-793-wasm.ps1
#   .\scripts\build-occt-793-wasm.ps1 -RootDir C:\bin\occt-wasm-build\V7_9_3

#Requires -Version 5.1
[CmdletBinding()]
param(
    [string] $RootDir = (Join-Path $env:USERPROFILE "occt-wasm-build\V7_9_3"),
    [string] $FreeTypeVersion = "2.13.3",
    [ValidateSet("Release", "Debug", "RelWithDebInfo")]
    [string] $BuildType = "Release",
    [int] $Jobs = 0,
    [switch] $SkipDownload,
    [switch] $SkipFreeType,
    [switch] $SkipOcct,
    [switch] $ReconfigureOnly
)

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
& "$scriptDir\build-occt-wasm.ps1" `
    -RootDir $RootDir `
    -OcctTag "V7_9_3" `
    -VersionDir "." `
    -FreeTypeVersion $FreeTypeVersion `
    -BuildType $BuildType `
    -Jobs $Jobs `
    -SkipDownload:$SkipDownload `
    -SkipFreeType:$SkipFreeType `
    -SkipOcct:$SkipOcct `
    -ReconfigureOnly:$ReconfigureOnly
