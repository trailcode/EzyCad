# [Draft] Polar duplicate: sketch-based reference when a sketch is available

**Suggested labels:** `enhancement`, `sketch`, `ux`

**Repo:** `trailcode/EzyCad`

---

## Title (GitHub)

Polar duplicate: use sketch-based points/plane when a sketch is available

## Body (GitHub)

### Summary

Polar duplicate today builds its polar arm and rotation using the **current sketch plane** and `Sketch_nodes::snap` for the first pick (see `Shp_polar_dup::move_point` / `add_point` in `src/shp_polar_dup.cpp`). There is a product gap: when a sketch exists, users expect polar array placement and reference geometry to feel **explicitly tied to sketch points, edges, or the sketch datum**, not only a bbox center plus plane snap.

### Current behavior (for context)

- Rotation axis uses `view().curr_sketch().get_plane()` and edge endpoints derived from the polar arm edge on that plane.
- First drag uses `get_shape_bbox_center(m_shps[0]->Shape())` as one end of the arm (see TODO in code: "consider all shapes").
- Successful `Dup` removes originals via `delete_operation_shps_()` (documented elsewhere; separate from sketch reference).

### Desired direction (open for design)

- When a **sketch is available** (e.g. current sketch has geometry or user is in a sketch-centric workflow), prefer:
  - Snapping the polar **origin** or **arm endpoints** to sketch nodes / edges / meaningful sketch references where possible.
  - Clear UX when no sketch is active (fallback to today or a world pick path).
- Align with **Reader-first** / pragmatic DRY in `ezycad_code_style.md`: keep polar-dup interaction discoverable in one place (Options + mouse flow) without scattering sketch rules across unrelated tools.

### Acceptance criteria (draft)

- [ ] With an active sketch, polar duplicate picks can use sketch-backed snaps (exact rules TBD with design).
- [ ] Without a sketch (or when explicitly in pure solid mode), behavior remains defined and documented (no silent failure).
- [ ] Update in-app hints / tooltips if needed so users know plane and snap source.

### Links / notes

- Code entry points: `src/shp_polar_dup.cpp`, `Occt_view::on_mode` sketch visibility for `Mode::Shape_polar_duplicate` in `src/occt_view.cpp`.

---

## Metadata (do not paste into GitHub)

- Draft path: `agents/drafts/issues/active/gh-102-polar-dup-sketch-based-reference.md`
