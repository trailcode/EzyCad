#!/usr/bin/env python3
"""
Rasterize a PDF to PNG. Many tools mislabel PDFs as ".pbf"; this script opens the
file with PyMuPDF, which recognizes PDF by content.

Requires: pip install pymupdf

Usage:
  python pbf-to-png.py input.pbf -o out.png --dpi 300
  python pbf-to-png.py doc.pdf --all-pages --dpi 150
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path


def _is_pdf_file(path: Path) -> bool:
    try:
        with path.open("rb") as f:
            head = f.read(5)
    except OSError as e:
        print(f"Error reading {path}: {e}", file=sys.stderr)
        return False
    return head.startswith(b"%PDF")


def main() -> int:
    p = argparse.ArgumentParser(
        description="Convert a PDF (often saved as .pbf) to PNG with configurable DPI."
    )
    p.add_argument(
        "input",
        type=Path,
        help="Input file (.pdf or PDF content with another extension such as .pbf)",
    )
    p.add_argument(
        "-o",
        "--output",
        type=Path,
        default=None,
        help="Output PNG path (single-page). For multiple pages with --all-pages, "
        "this is treated as a filename pattern: stem-001.png, stem-002.png, ...",
    )
    p.add_argument(
        "--dpi",
        type=float,
        default=150.0,
        metavar="N",
        help="Rasterization resolution in dots per inch (default: 150)",
    )
    p.add_argument(
        "--all-pages",
        action="store_true",
        help="Export every page; filenames use stem-001.png style unless -o is omitted "
        "(then uses input basename next to the input file)",
    )
    p.add_argument(
        "--page",
        type=int,
        default=None,
        metavar="N",
        help="1-based page index to export (default with --all-pages: all pages; "
        "otherwise: page 1 only)",
    )
    args = p.parse_args()

    src = args.input
    if not src.is_file():
        print(f"Not a file: {src}", file=sys.stderr)
        return 1

    if not _is_pdf_file(src):
        print(
            f"{src} does not look like a PDF (missing %PDF header). "
            "If this is a Mapbox/OSM vector tile or other binary PBF, use a different tool.",
            file=sys.stderr,
        )
        return 1

    try:
        import fitz  # PyMuPDF
    except ImportError:
        print(
            "Missing dependency: install with  pip install pymupdf",
            file=sys.stderr,
        )
        return 1

    dpi = float(args.dpi)
    if dpi <= 0:
        print("--dpi must be positive", file=sys.stderr)
        return 1

    zoom = dpi / 72.0
    mat = fitz.Matrix(zoom, zoom)

    try:
        doc = fitz.open(src)
    except Exception as e:
        print(f"Failed to open as PDF: {e}", file=sys.stderr)
        return 1

    try:
        n = doc.page_count
        if args.page is not None:
            if args.page < 1 or args.page > n:
                print(f"--page must be between 1 and {n}", file=sys.stderr)
                return 1
            indices = [args.page - 1]
        elif args.all_pages:
            indices = list(range(n))
        else:
            indices = [0]

        if len(indices) == 1:
            out = args.output
            if out is None:
                out = src.with_suffix(".png")
            else:
                out = Path(out)
            parent = out.parent
            if parent and str(parent) != ".":
                parent.mkdir(parents=True, exist_ok=True)
            page = doc.load_page(indices[0])
            pix = page.get_pixmap(matrix=mat, alpha=False)
            pix.save(str(out))
            print(out)
        else:
            base = args.output
            if base is None:
                stem = src.stem
                out_dir = src.parent
            else:
                base = Path(base)
                stem = base.stem
                out_dir = base.parent
                if not str(out_dir) or out_dir == Path("."):
                    out_dir = src.parent
            out_dir.mkdir(parents=True, exist_ok=True)
            width = max(3, len(str(len(indices))))
            for i, pi in enumerate(indices):
                page = doc.load_page(pi)
                pix = page.get_pixmap(matrix=mat, alpha=False)
                name = f"{stem}-{i + 1:0{width}d}.png"
                path = out_dir / name
                pix.save(str(path))
                print(path)
    finally:
        doc.close()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
