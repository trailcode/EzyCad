#!/usr/bin/env python3
"""Align GFM pipe tables in Markdown files (source-readable + preview-safe).

Skips fenced code blocks. Excludes third_party / build / vendor trees.
Usage: python scripts/align_md_tables.py [--check] [paths...]
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

SKIP_DIR_NAMES = {
    ".git",
    "third_party",
    "node_modules",
    "_build",
    "_deps",
    "build",
    "out",
    ".venv",
    "venv",
}

# Also skip CMake / local build trees (build-vs18, build-wasm, etc.).
SKIP_DIR_PREFIXES = ("build-", "cmake-build")

ROW_RE = re.compile(r"^\s*\|.*\|\s*$")
SEP_CELL_RE = re.compile(r"^:?-+:?$")


def is_separator_row(cells: list[str]) -> bool:
    if not cells:
        return False
    return all(SEP_CELL_RE.match(c.replace(" ", "")) for c in cells)


def split_row(line: str) -> list[str]:
    s = line.strip()
    if s.startswith("|"):
        s = s[1:]
    if s.endswith("|"):
        s = s[:-1]
    return [c.strip() for c in s.split("|")]


def format_separator(width: int, cell: str) -> str:
    raw = cell.replace(" ", "")
    left = raw.startswith(":")
    right = raw.endswith(":") and raw != ":"
    # Minimum three dashes for GFM.
    inner = max(3, width - (1 if left else 0) - (1 if right else 0))
    body = "-" * inner
    if left and right:
        return f":{body}:"
    if left:
        return f":{body}"
    if right:
        return f"{body}:"
    return body


def align_table(lines: list[str]) -> list[str]:
    rows = [split_row(line) for line in lines]
    col_count = max(len(r) for r in rows)
    for r in rows:
        while len(r) < col_count:
            r.append("")

    widths = [0] * col_count
    for r in rows:
        if is_separator_row(r):
            continue
        for i, cell in enumerate(r):
            widths[i] = max(widths[i], len(cell))
    # Separators need at least 3 dashes of content width.
    for i in range(col_count):
        widths[i] = max(widths[i], 3)

    out: list[str] = []
    for r in rows:
        if is_separator_row(r):
            cells = [format_separator(widths[i], r[i]) for i in range(col_count)]
            # Pad separator visual width to column width.
            padded = []
            for i, cell in enumerate(cells):
                pad = widths[i] - len(cell)
                if pad > 0:
                    # Insert extra dashes before any trailing colon.
                    if cell.endswith(":"):
                        cell = cell[:-1] + ("-" * pad) + ":"
                    else:
                        cell = cell + ("-" * pad)
                padded.append(cell)
            out.append("| " + " | ".join(padded) + " |")
        else:
            cells = [r[i].ljust(widths[i]) for i in range(col_count)]
            out.append("| " + " | ".join(cells) + " |")
    return out


def process_text(text: str) -> str:
    lines = text.splitlines(keepends=True)
    out: list[str] = []
    i = 0
    in_fence = False
    fence_marker = ""

    while i < len(lines):
        line = lines[i]
        stripped = line.lstrip()

        if not in_fence and stripped.startswith("```"):
            in_fence = True
            fence_marker = stripped[:3]
            out.append(line)
            i += 1
            continue
        if in_fence:
            out.append(line)
            if stripped.startswith(fence_marker):
                in_fence = False
                fence_marker = ""
            i += 1
            continue

        if ROW_RE.match(line):
            block: list[str] = []
            raw_block: list[str] = []
            while i < len(lines) and ROW_RE.match(lines[i]):
                raw_block.append(lines[i])
                # Preserve original newline style via keepends content without NL for align.
                block.append(lines[i].rstrip("\r\n"))
                i += 1
            # Only treat as a table if there is a separator row.
            cells_rows = [split_row(b) for b in block]
            if any(is_separator_row(r) for r in cells_rows) and len(block) >= 2:
                aligned = align_table(block)
                nl = "\n"
                if raw_block and raw_block[0].endswith("\r\n"):
                    nl = "\r\n"
                elif raw_block and raw_block[0].endswith("\n"):
                    nl = "\n"
                for a in aligned:
                    out.append(a + nl)
            else:
                out.extend(raw_block)
            continue

        out.append(line)
        i += 1

    return "".join(out)


def should_skip(path: Path) -> bool:
    for part in path.parts:
        if part in SKIP_DIR_NAMES:
            return True
        if part.startswith(SKIP_DIR_PREFIXES):
            return True
    return False


def iter_md_files(roots: list[Path]) -> list[Path]:
    files: list[Path] = []
    for root in roots:
        if root.is_file() and root.suffix.lower() == ".md":
            if not should_skip(root):
                files.append(root)
            continue
        if not root.is_dir():
            continue
        for path in root.rglob("*.md"):
            if should_skip(path):
                continue
            files.append(path)
    return sorted(set(files))


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "paths",
        nargs="*",
        type=Path,
        default=[Path(".")],
        help="Files or directories (default: repo root)",
    )
    ap.add_argument(
        "--check",
        action="store_true",
        help="Exit 1 if any file would change; do not write",
    )
    args = ap.parse_args()

    changed: list[Path] = []
    for path in iter_md_files(args.paths):
        original = path.read_text(encoding="utf-8")
        updated = process_text(original)
        # Preserve final newline presence.
        if original.endswith("\n") and not updated.endswith("\n"):
            updated += "\n"
        if updated != original:
            changed.append(path)
            if not args.check:
                path.write_text(updated, encoding="utf-8", newline="")

    if args.check:
        if changed:
            print("Would align tables in:")
            for p in changed:
                print(f"  {p}")
            return 1
        print("All Markdown tables already aligned.")
        return 0

    if changed:
        print(f"Aligned tables in {len(changed)} file(s):")
        for p in changed:
            print(f"  {p}")
    else:
        print("No Markdown table changes needed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
