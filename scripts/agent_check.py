#!/usr/bin/env python3
"""One post-edit quality check for agents and the IDE.

Runs in a single Python process (no PowerShell):
  - 7-bit ASCII in C/C++ under src/ and tests/ (or given paths)
  - Markdown table alignment --check when .md paths are given
  - Optional: code_style_check.py (vertical rhythm) with --style; not CI

Usage (from repo root):
  python scripts/agent_check.py
  python scripts/agent_check.py src/gui.cpp src/gui.h
  python scripts/agent_check.py docs/usage.md
  python scripts/code_style_check.py src/gui.cpp

Default paths: src/ and tests/. Pass the files you just edited so the report
stays small. Exit 1 if any check fails.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

# Same directory as this file (scripts/).
_SCRIPTS = Path(__file__).resolve().parent
if str(_SCRIPTS) not in sys.path:
    sys.path.insert(0, str(_SCRIPTS))

import align_md_tables  # noqa: E402
import code_style_check  # noqa: E402


def repo_root() -> Path:
    return _SCRIPTS.parent


def rel_path(path: Path, root: Path) -> str:
    try:
        return path.resolve().relative_to(root.resolve()).as_posix()
    except ValueError:
        return path.as_posix()


def check_ascii(files: list[Path], root: Path) -> int:
    found = 0
    for path in files:
        try:
            text = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            text = path.read_text(encoding="utf-8", errors="replace")
        for line_num, line in enumerate(text.splitlines(), start=1):
            for col, ch in enumerate(line, start=1):
                cp = ord(ch)
                if cp > 0x7F:
                    print(f"{rel_path(path, root)}:{line_num}:{col}: non-ASCII U+{cp:04X} ({ch!r})")
                    found += 1
    if found:
        print(f"Total non-ASCII character occurrences: {found}")
        return 1
    print(f"ASCII: ok ({len(files)} file(s)).")
    return 0


def check_style(files: list[Path], root: Path) -> int:
    findings = code_style_check.collect_findings(files)
    for f in findings:
        print(f.format(root))
    if findings:
        print(f"{len(findings)} code-style finding(s) in {len({f.path for f in findings})} file(s).")
        return 1
    print(f"Code style: ok ({len(files)} file(s)).")
    return 0


def check_md_tables(md_paths: list[Path], root: Path) -> int:
    files = align_md_tables.iter_md_files(md_paths)
    if not files:
        return 0
    bad: list[Path] = []
    for path in files:
        original = path.read_text(encoding="utf-8")
        updated = align_md_tables.process_text(original)
        if original.endswith("\n") and not updated.endswith("\n"):
            updated += "\n"
        if updated != original:
            bad.append(path)
    if bad:
        print("Markdown tables need alignment (python scripts/align_md_tables.py):")
        for path in bad:
            print(f"  {rel_path(path, root)}")
        return 1
    print(f"Markdown tables: ok ({len(files)} file(s)).")
    return 0


def main() -> int:
    root = repo_root()
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "paths",
        nargs="*",
        type=Path,
        default=[root / "src", root / "tests"],
        help="Files or directories you edited (default: src/ and tests/)",
    )
    ap.add_argument(
        "--style",
        action="store_true",
        help="Also run code_style_check.py (optional; not used by CI)",
    )
    args = ap.parse_args()
    paths = [p if p.is_absolute() else (Path.cwd() / p) for p in args.paths]

    cpp_files = code_style_check.iter_cpp_files(paths)
    cpp_roots = {(root / "src").resolve(), (root / "tests").resolve()}
    md_paths: list[Path] = []
    for p in paths:
        if p.is_file() and p.suffix.lower() == ".md":
            md_paths.append(p)
        elif p.is_dir() and p.resolve() not in cpp_roots:
            md_paths.append(p)

    if not cpp_files and not md_paths:
        print("No C/C++ or Markdown files to check.", file=sys.stderr)
        return 2

    rc = 0
    if cpp_files:
        rc |= check_ascii(cpp_files, root)
        if args.style:
            rc |= check_style(cpp_files, root)
    if md_paths:
        rc |= check_md_tables(md_paths, root)
    return 1 if rc else 0


if __name__ == "__main__":
    sys.exit(main())
