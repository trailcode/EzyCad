# Download and build Open CASCADE Technology (OCCT) for WebAssembly (Emscripten).
#
# Builds FreeType static libs, then OCCT static libs with GLES2 (TKOpenGles) for EzyCad's wasm target.
#
# Prerequisites: Git, CMake 3.16+, Emscripten SDK on PATH (run emsdk_env first).
#
# Prefer the version-specific wrappers:
#   .\scripts\build-occt-793-wasm.ps1
#   .\scripts\build-occt-v8-wasm.ps1
#
# Install layout (default VersionDir = OcctTag under RootDir):
#   <RootDir>\<VersionDir>\install\lib\cmake\opencascade  -> OpenCASCADE_DIR
#
# Flat layout (VersionDir = "."): src/build/install directly under RootDir.
# Version wrappers default RootDir to %USERPROFILE%\occt-wasm-build\<OcctTag>.

#Requires -Version 5.1
[CmdletBinding()]
param(
    [string] $RootDir = (Join-Path $env:USERPROFILE "occt-wasm-build"),
    [Parameter(Mandatory = $true)]
    [string] $OcctTag,
    # Subdirectory under RootDir for src/build/install. Default: OcctTag. Use "." for flat layout.
    [string] $VersionDir = "",
    [string] $FreeTypeVersion = "2.13.3",
    [ValidateSet("Release", "Debug", "RelWithDebInfo")]
    [string] $BuildType = "Release",
    [int] $Jobs = 0,
    [switch] $SkipDownload,
    [switch] $SkipFreeType,
    [switch] $SkipOcct,
    [switch] $ReconfigureOnly
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Require-Command([string]$Name) {
    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "Required command not found on PATH: $Name (is emsdk_env active?)"
    }
}

function Invoke-EmCMake {
    param([string[]]$CmakeArguments)
    & emcmake cmake @CmakeArguments
    if ($LASTEXITCODE -ne 0) { throw "emcmake cmake failed (exit $LASTEXITCODE)" }
}

function Invoke-EmInstall {
    param([string]$BuildDir, [int]$Parallel)
    Push-Location $BuildDir
    try {
        if ($Parallel -gt 0) {
            & emmake cmake --build . --target install --parallel $Parallel
        }
        else {
            & emmake cmake --build . --target install
        }
        if ($LASTEXITCODE -ne 0) { throw "install failed (exit $LASTEXITCODE)" }
    }
    finally {
        Pop-Location
    }
}

Require-Command git
Require-Command cmake
Require-Command emcc
Require-Command emcmake
Require-Command emmake

if ($Jobs -le 0) {
    $Jobs = [Environment]::ProcessorCount
    if ($Jobs -lt 2) { $Jobs = 2 }
}

if ([string]::IsNullOrWhiteSpace($VersionDir)) {
    $VersionDir = $OcctTag
}

$RootDir = [System.IO.Path]::GetFullPath($RootDir)
if ($VersionDir -eq ".") {
    $VersionRoot = $RootDir
}
else {
    $VersionRoot = Join-Path $RootDir $VersionDir
}
$SrcDir = Join-Path $VersionRoot "src"
$BuildDir = Join-Path $VersionRoot "build"
$InstallDir = Join-Path $VersionRoot "install"
$OcctSrc = Join-Path $SrcDir "OCCT"
$FtSrc = Join-Path $SrcDir "freetype-$FreeTypeVersion"
$FtBuild = Join-Path $BuildDir "freetype"
$FtInstall = Join-Path $InstallDir "freetype"
$OcctBuild = Join-Path $BuildDir "occt"
$OcctInstall = $InstallDir

$ExceptionFlags = "-fexceptions"

Write-Host "=== OCCT $OcctTag WebAssembly build ===" -ForegroundColor Cyan
Write-Host "Root:       $RootDir"
Write-Host "VersionDir: $VersionDir"
Write-Host "Tree:       $VersionRoot"
Write-Host "Install:    $OcctInstall"
Write-Host "Jobs:       $Jobs"
Write-Host "Type:       $BuildType"
Write-Host ""

New-Item -ItemType Directory -Force -Path $SrcDir, $BuildDir, $InstallDir | Out-Null

# --- FreeType ---
if (-not $SkipFreeType) {
    if (-not $SkipDownload -and -not (Test-Path $FtSrc)) {
        Write-Host ">>> Download FreeType $FreeTypeVersion" -ForegroundColor Yellow
        # Use .tar.gz: Windows built-in tar cannot extract .tar.xz without a separate xz tool.
        $ftArchive = Join-Path $SrcDir "freetype-$FreeTypeVersion.tar.gz"
        $ftUrl = "https://download.savannah.gnu.org/releases/freetype/freetype-$FreeTypeVersion.tar.gz"
        Invoke-WebRequest -Uri $ftUrl -OutFile $ftArchive
        & tar -xzf $ftArchive -C $SrcDir
        if (-not (Test-Path $FtSrc)) { throw "FreeType extract failed: $FtSrc" }
    }

    if (-not $ReconfigureOnly -or -not (Test-Path (Join-Path $FtBuild "CMakeCache.txt"))) {
        Write-Host ">>> Configure FreeType (wasm)" -ForegroundColor Yellow
        New-Item -ItemType Directory -Force -Path $FtBuild | Out-Null
        Invoke-EmCMake -CmakeArguments @(
            "-S", $FtSrc,
            "-B", $FtBuild,
            "-DCMAKE_BUILD_TYPE=$BuildType",
            "-DCMAKE_INSTALL_PREFIX=$FtInstall",
            "-DCMAKE_CXX_FLAGS=$ExceptionFlags",
            "-DCMAKE_C_FLAGS=$ExceptionFlags",
            "-DCMAKE_EXE_LINKER_FLAGS=$ExceptionFlags",
            "-DBUILD_SHARED_LIBS=OFF",
            "-DCMAKE_DISABLE_FIND_PACKAGE_ZLIB=ON",
            "-DCMAKE_DISABLE_FIND_PACKAGE_BZip2=ON",
            "-DCMAKE_DISABLE_FIND_PACKAGE_PNG=ON",
            "-DCMAKE_DISABLE_FIND_PACKAGE_HarfBuzz=ON",
            "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"
        )
    }

    Write-Host ">>> Build & install FreeType" -ForegroundColor Yellow
    Invoke-EmInstall -BuildDir $FtBuild -Parallel $Jobs
}

$FreeTypeDir = Join-Path $FtInstall "lib\cmake\freetype"
if (-not (Test-Path $FreeTypeDir)) {
    throw "FreeType CMake package not found: $FreeTypeDir"
}

# --- OCCT source ---
if (-not $SkipDownload) {
    if (-not (Test-Path $OcctSrc)) {
        Write-Host ">>> Clone OCCT ($OcctTag)" -ForegroundColor Yellow
        & git clone --depth 1 --branch $OcctTag `
            https://github.com/Open-Cascade-SAS/OCCT.git $OcctSrc
        if ($LASTEXITCODE -ne 0) { throw "git clone failed" }
    }
    else {
        Write-Host ">>> OCCT source exists; checkout $OcctTag" -ForegroundColor Yellow
        Push-Location $OcctSrc
        try {
            & git fetch --depth 1 origin tag $OcctTag
            if ($LASTEXITCODE -ne 0) { throw "git fetch failed" }
            & git checkout $OcctTag
            if ($LASTEXITCODE -ne 0) { throw "git checkout failed" }
        }
        finally {
            Pop-Location
        }
    }
}
elseif (-not (Test-Path $OcctSrc)) {
    throw "OCCT source missing: $OcctSrc (omit -SkipDownload)"
}

# --- OCCT build ---
if (-not $SkipOcct) {
    if (-not $ReconfigureOnly -or -not (Test-Path (Join-Path $OcctBuild "CMakeCache.txt"))) {
        Write-Host ">>> Configure OCCT (wasm static + GLES2)" -ForegroundColor Yellow
        New-Item -ItemType Directory -Force -Path $OcctBuild | Out-Null

        # EzyCad links TKOpenGles, not TKOpenGl.
        $OcctCmakeArgs = @(
            "-S", $OcctSrc,
            "-B", $OcctBuild,
            "-DCMAKE_BUILD_TYPE=$BuildType",
            "-DCMAKE_INSTALL_PREFIX=$OcctInstall",
            "-DBUILD_LIBRARY_TYPE=Static",
            "-DBUILD_MODULE_Draw=OFF",
            "-DBUILD_SAMPLES=OFF",
            "-DBUILD_DOC_Overview=OFF",
            "-DUSE_OPENGL=OFF",
            "-DUSE_GLES2=ON",
            "-DUSE_FREETYPE=ON",
            "-DUSE_TBB=OFF",
            "-DUSE_FFMPEG=OFF",
            "-DUSE_FREEIMAGE=OFF",
            "-DUSE_OPENVR=OFF",
            "-DUSE_TK=OFF",
            "-Dfreetype_DIR=$FreeTypeDir",
            "-DCMAKE_CXX_FLAGS=$ExceptionFlags",
            "-DCMAKE_C_FLAGS=$ExceptionFlags",
            "-DCMAKE_EXE_LINKER_FLAGS=$ExceptionFlags",
            "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"
        )
        # USE_VTK option exists from OCCT 7.x; default OFF in 8.0.
        $OcctCmakeArgs += "-DUSE_VTK=OFF"
        Invoke-EmCMake -CmakeArguments $OcctCmakeArgs
    }

    Write-Host ">>> Build & install OCCT (may take 1-3+ hours)" -ForegroundColor Yellow
    Invoke-EmInstall -BuildDir $OcctBuild -Parallel $Jobs
}

$OcctConfig = Join-Path $OcctInstall "lib\cmake\opencascade\OpenCASCADEConfig.cmake"
if (-not (Test-Path $OcctConfig)) {
    throw "OCCT install incomplete; missing: $OcctConfig"
}

$OpenCascadeDir = Join-Path $OcctInstall "lib\cmake\opencascade"
$tagSuffix = ($OcctTag -replace '^V', '' -replace '_', '-').ToLower()
$EzyCadBuildDir = "build-em-$tagSuffix"
Write-Host ""
Write-Host "=== Done ===" -ForegroundColor Green
Write-Host "OcctTag=$OcctTag"
Write-Host "VersionDir=$VersionDir"
Write-Host "OpenCASCADE_DIR=$OpenCascadeDir"
Write-Host ""
Write-Host "EzyCad wasm configure example:"
Write-Host "  emcmake cmake $((Split-Path -Parent $PSScriptRoot)) -Wno-dev -G Ninja -B $EzyCadBuildDir -DOpenCASCADE_DIR=$OpenCascadeDir -DCMAKE_BUILD_TYPE=Release"
Write-Host "  ninja -C $EzyCadBuildDir"
