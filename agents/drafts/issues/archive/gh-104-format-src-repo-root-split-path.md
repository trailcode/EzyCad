# [Draft] Fix `scripts/format-src.ps1` repo root detection

**Suggested GitHub labels:** `bug`, `windows`, `tooling`

**Status:** Resolved in-repo (see **Fix** below). Use this text for a changelog note, a retrospective issue, or copy into GitHub then close.

**GitHub:** https://github.com/trailcode/EzyCad/issues/104

---

## Title

`format-src.ps1` used wrong source tree root (`Split-Path` one level too many)

## Body

### Summary

Running `scripts/format-src.ps1` after bypassing execution policy succeeded in starting the script, but **`Get-ChildItem` failed**: it looked under **`C:\src\src`** instead of **`<repo>\src`** (for a clone such as `C:\src\EzyCad`).

### Root cause

`$PSScriptRoot` is **`…/EzyCad/scripts`**. The script computed the repo root with **two** `Split-Path -Parent` calls:

- First parent → `…/EzyCad` (correct repo root).
- Second parent → `…/src` (wrong).

`Join-Path $root "src"` then pointed at **`C:\src\src`**.

### Fix

Use **one** parent of `$PSScriptRoot` as the repo root, then **`Join-Path`** to `src`. Updated in **`scripts/format-src.ps1`** with an inline comment documenting layout: `<repo>/scripts/format-src.ps1`.

### How to verify

From `scripts/`:

```powershell
powershell -ExecutionPolicy Bypass -File .\format-src.ps1
```

Confirm `clang-format` runs on files under **`(repo)/src`** (requires LLVM `clang-format` on PATH or the path at the top of the script).

### Related (not a repo bug)

**PowerShell:** If `.\format-src.ps1` is blocked, either use `Bypass`/`Process` scope for one run or set **`Set-ExecutionPolicy RemoteSigned -Scope CurrentUser`** (see Microsoft `about_Execution_Policies`).

---

## Metadata (do not paste)

- Draft lives under **`agents/drafts/issues/`** (repo-local companion to GitHub).
- Repo: `trailcode/EzyCad`.
