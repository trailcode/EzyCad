# Sphinx configuration for EzyCad user documentation (Read the Docs).
# Everything lives under the single top-level docs/ directory for clarity.
# Markdown content (usage*.md etc.) lives as siblings to this conf.py.

from __future__ import annotations

import shutil
from pathlib import Path

DOCS_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = DOCS_DIR.parent


def _copy_file_resilient(src: Path, dst: Path) -> None:
    """Copy one file; if dst exists but is locked (Windows), keep the existing copy."""
    dst.parent.mkdir(parents=True, exist_ok=True)
    try:
        shutil.copy2(src, dst)
    except OSError:
        if dst.is_file():
            return
        raise


def _sync_tree_resilient(src: Path, dst: Path) -> None:
    """Mirror src into dst file-by-file (avoids copytree failing on locked targets)."""
    if not src.is_dir():
        return
    for item in src.rglob("*"):
        rel = item.relative_to(src)
        target = dst / rel
        if item.is_dir():
            target.mkdir(parents=True, exist_ok=True)
        else:
            _copy_file_resilient(item, target)


def _prepare_assets() -> None:
    """Copy supporting assets (icons + splash) so relative paths in the guides work."""
    # Icons referenced from the style guide and some usage pages as res/icons/...
    res_dst = DOCS_DIR / "res"
    res_dst.mkdir(exist_ok=True)
    icons_src = PROJECT_ROOT / "res" / "icons"
    if icons_src.is_dir():
        _sync_tree_resilient(icons_src, res_dst / "icons")

    # Splash screen (used in index and README)
    splash_src = PROJECT_ROOT / "res" / "AI-gen-splashscreen_05_01_2026_512.png"
    if splash_src.is_file():
        _copy_file_resilient(splash_src, res_dst / splash_src.name)


def _verify_doc_assets() -> None:
    """Fail fast on CI/RTD if required assets are missing."""
    required = [
        DOCS_DIR / "res" / "icons" / "Draft_Rotate.png",
        DOCS_DIR / "images" / "rotate_constrain_axis.png",
        DOCS_DIR / "images" / "AI-gen-splashscreen_05_01_2026_512.png",
    ]
    missing = [p.relative_to(DOCS_DIR) for p in required if not p.is_file()]
    if missing:
        raise RuntimeError(
            "Documentation assets missing: " + ", ".join(str(p) for p in missing) +
            ". Make sure res/icons/ and docs/images/ are committed."
        )


_prepare_assets()
_verify_doc_assets()

project = "EzyCad"
copyright = "2026, trailcode"
author = "trailcode"

# Search and social previews (Read the Docs, Google, link unfurlers).
html_title = "EzyCad — open-source hobbyist CAD (OCCT, ImGui, OpenGL)"
html_short_title = "EzyCad"
html_meta = {
    "description": (
        "EzyCad (Easy CAD) is open-source hobbyist CAD for machining: sketch, "
        "extrude, and export STEP/STL using Open CASCADE, Dear ImGui, and OpenGL. "
        "Not EZCAD laser marking software."
    ),
    "keywords": (
        "EzyCad, CAD, Open CASCADE, OCCT, ImGui, OpenGL, CNC, machining, "
        "STEP, STL, WebAssembly, Emscripten, 3D modeling"
    ),
}
html_theme_options = {
    "navigation_depth": 4,
    "collapse_navigation": False,
}

extensions = [
    "myst_parser",
]

myst_heading_anchors = 5
myst_enable_extensions = [
    "colon_fence",
    "linkify",
]

templates_path = ["_templates"]
exclude_patterns = [
    "_build",
    "Thumbs.db",
    ".DS_Store",
    "requirements.txt",
    "readthedocs.md",
]

# Same-page #anchors in usage.md are not myst cross-refs; do not fail the build on them.
suppress_warnings = [
    "myst.xref_missing",
]

html_theme = "sphinx_rtd_theme"
html_static_path = ["_static"]
html_css_files = ["custom.css"]

# usage.md and related pages are the main entry; index.rst wires the toctree.
master_doc = "index"
