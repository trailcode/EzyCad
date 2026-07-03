---
github_issue: 154
github_pr: 155
status: merged
paired_draft: ../../issues/archive/gh-154-sketch-bounds-shader-grid.md
---

# PR — Sketch-bounds OCCT shader grid with padding and tools/ dev helpers

**Issue:** https://github.com/trailcode/EzyCad/issues/154  
**PR:** https://github.com/trailcode/EzyCad/pull/155

## Summary

- Sketch-bounds shader grid with `grid_padding` (replaces extent X/Y).
- OCCT `Aspect_GridParams` on sketch plane; grid behind coplanar geometry.
- `tools/pimpl_gen.py`; `pbf-to-png` moved to `tools/`.
- `utl_geom_boost.inl` extracted from `utl_geom.h`.

## Test plan

- [ ] Release build
- [ ] Grid follows sketch + padding in Settings
- [ ] Legacy settings JSON loads
