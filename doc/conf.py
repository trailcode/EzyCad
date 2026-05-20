# Sphinx configuration for EzyCad user documentation (Read the Docs).
# User guides live at the repository root; they are synced into doc/ at build time
# so cross-links (usage.md, usage-sketch.md, ...) and res/icons/ paths keep working.

from __future__ import annotations

import shutil
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent
DOCS_DIR = Path(__file__).resolve().parent

# Markdown guides published on Read the Docs (canonical copies stay at repo root).
_USER_MD = (
    "usage.md",
    "usage-sketch.md",
    "usage-settings.md",
    "usage-occt-view.md",
    "scripting.md",
)


def _sync_user_docs() -> None:
    for name in _USER_MD:
        src = PROJECT_ROOT / name
        if src.is_file():
            shutil.copy2(src, DOCS_DIR / name)

    res_dst = DOCS_DIR / "res"
    res_dst.mkdir(exist_ok=True)

    splash_src = PROJECT_ROOT / "res" / "AI-gen-splashscreen_05_01_2026_512.png"
    if splash_src.is_file():
        shutil.copy2(splash_src, res_dst / splash_src.name)

    icons_src = PROJECT_ROOT / "res" / "icons"
    icons_dst = res_dst / "icons"
    if icons_src.is_dir():
        shutil.copytree(icons_src, icons_dst, dirs_exist_ok=True)

    doc_gen_src = PROJECT_ROOT / "doc" / "gen"
    doc_gen_dst = DOCS_DIR / "doc" / "gen"
    if doc_gen_src.is_dir():
        shutil.copytree(doc_gen_src, doc_gen_dst, dirs_exist_ok=True)


def _verify_doc_assets() -> None:
    """Fail the build early if synced assets are missing (catches RTD/checkout issues)."""
    required = [
        DOCS_DIR / "res" / "icons" / "Draft_Rotate.png",
        DOCS_DIR / "doc" / "gen" / "rotate_constrain_axis.png",
    ]
    missing = [p.relative_to(DOCS_DIR) for p in required if not p.is_file()]
    if missing:
        raise RuntimeError(
            "Documentation assets missing after sync from repo root: "
            + ", ".join(str(p) for p in missing)
            + ". Ensure res/icons/ and doc/gen/ are committed."
        )


_sync_user_docs()
_verify_doc_assets()

project = "EzyCad"
copyright = "2026, trailcode"
author = "trailcode"

extensions = [
    "myst_parser",
]

myst_heading_anchors = 3
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

# usage.md and related pages are the main entry; index.rst wires the toctree.
master_doc = "index"
