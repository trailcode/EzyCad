# Sketch snap unification, fixes, and documentation

**Opened on GitHub:** https://github.com/trailcode/EzyCad/issues/111

**Suggested labels:** `enhancement`, `sketch`, `docs`

---

## Title (GitHub)

Sketch snap: unify axis guides, fix dimension hover, restore cross-sketch snap, document behavior

## Body (GitHub)

### Summary

Simplify sketch node snapping around a single axis-guide model, fix a dimension-tool hover bug, restore direct snap to nodes on other visible sketches, and document how snap works for users.

### Problem

- `try_get_node_idx_snap` had overlapping paths (direct vertex snap, outside snap, axis snap) and dead annotation code (`update_node_snap_anno_`, `m_snap_anno`).
- `try_pick_existing_node` passed a stored node reference into snap logic, which could mutate sketch geometry during dimension-tool hover.
- The recent snap simplification dropped direct snap to **outside** sketch points (nodes on other visible sketches).
- User guides described snap only generically ("snaps to nodes and geometry") with no explanation of axis alignment vs vertex lock or cross-sketch targets.

### Implemented scope

**Code (`src/sketch_nodes.cpp`, `src/sketch_nodes.h`):**

- Unified snap feedback through axis guides (`show_snap_guides_at_`, `update_axis_snap_anno_`).
- Shared vertex snap threshold helper (`vertex_snap_threshold_sq_`).
- **Fix:** dimension pick hover uses `show_snap_guides_at_` on a copy of the picked position (no mutation of stored nodes).
- **Restore:** direct snap to `m_outside_snap_pts` (other visible sketches, projected onto the current plane).
- **Keep:** in-sketch vertex lock via dual-axis convergence (not proximity snap) so add-node edge-interior placement still works.
- Removed dead `update_node_snap_anno_`, `m_snap_anno`, `m_last_snap_pt`.

**Documentation:**

- `usage-sketch.md` — new **Sketch snapping** section (axis guides, vertex lock, edge interior, cross-sketch).
- `usage-settings.md` — link from Sketch options to snap section.
- `usage.md` — Sketch List **Nodes** clarifies user-placed points only.
- `CHANGELOG.md` — `[Unreleased]` entries.

### Acceptance criteria

- [ ] Dimension tool hover near a node shows snap guides without moving stored node coordinates.
- [ ] Snapping works to nodes on other **visible** sketches (e.g. polar duplicate, multi-sketch alignment).
- [ ] Add-node edge-interior split still works (near-miss onto segment interior).
- [ ] All `Sketch_test` cases pass.
- [ ] User guide **Sketch snapping** section matches in-app behavior.

### Files touched

- `src/sketch_nodes.cpp`
- `src/sketch_nodes.h`
- `usage-sketch.md`
- `usage-settings.md`
- `usage.md`
- `CHANGELOG.md`

### Related

- Prior simplification commits: axis-only snap model in `try_get_node_idx_snap`.
- `agents/drafts/issues/active/gh-102-polar-dup-sketch-based-reference.md` — polar duplicate sketch snap expectations.
