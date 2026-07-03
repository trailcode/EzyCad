---
github_issue: 154
github_pr: 155
status: merged
paired_draft: ../prs/archive/gh-155-sketch-bounds-shader-grid.md
---

# Sketch-bounds OCCT shader grid with padding and tools/ dev helpers

**GitHub:** https://github.com/trailcode/EzyCad/issues/154  
**PR:** https://github.com/trailcode/EzyCad/pull/155

## Summary

Replace fixed grid extent X/Y with sketch-bounds sizing plus **Grid padding**; draw via OCCT shader grid on the active sketch plane; refresh when sketch geometry changes. Add `tools/pimpl_gen.py` and move asset helpers to `tools/`.

## Key files

- `src/occt_view.cpp`, `src/occt_view.h`, `src/gui_settings.cpp`, `src/sketch.cpp`
- `docs/usage-settings.md`, `res/ezycad_settings.json`
- `tools/pimpl_gen.py`, `CMakeLists.txt`

## Notes

- Legacy `grid_graphic_x_size` loads as `grid_padding`.
- Do not commit `tools/__pycache__/`.
