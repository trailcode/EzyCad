#!/usr/bin/env python3
"""CLI wrapper for the ezycad remote client.

Prefer::

    pip install -e .
    python -m ezycad
    # or: import ezycad; app = ezycad.connect()

This script adds ``scripts/`` to sys.path so it works without install.
"""

from __future__ import annotations

import sys
from pathlib import Path

_scripts_dir = Path(__file__).resolve().parent
if str(_scripts_dir) not in sys.path:
    sys.path.insert(0, str(_scripts_dir))

from ezycad.__main__ import main

if __name__ == "__main__":
    raise SystemExit(main())
