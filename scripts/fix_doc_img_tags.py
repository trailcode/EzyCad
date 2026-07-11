#!/usr/bin/env python3
"""Convert inline HTML <img> tags to Markdown images for Sphinx/RTD."""

from __future__ import annotations

import re
import sys
from pathlib import Path

_IMG = re.compile(
    r'<img\s+src="([^"]+)"\s+alt="([^"]*)"(?:\s+width="\d+"\s+height="\d+")?\s*/?>',
    re.IGNORECASE,
)


def fix(text: str) -> str:
    return _IMG.sub(lambda m: f"![{m.group(2)}]({m.group(1)})", text)


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    for name in ("usage.md", "usage-sketch.md"):
        path = root / name
        orig = path.read_text(encoding="utf-8")
        new = fix(orig)
        n = len(_IMG.findall(orig))
        if new != orig:
            path.write_text(new, encoding="utf-8", newline="\n")
        print(f"{name}: converted {n} inline <img> tags")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
