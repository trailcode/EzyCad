# PR - Fix sketch face walk arrival tangent on arcs

---
github_pr: 185
status: active
paired_draft: ../../issues/active/gh-184-sketch-face-walk-incoming-arc-tangent.md
---

**Opened on GitHub:** https://github.com/trailcode/EzyCad/pull/185

## Title

Fix sketch face walk arrival tangent on arcs (circle + chord)

## Summary

- Face walk used `sketch_edge_outgoing_dir_2d` (tangent at edge **start**) as the arrival direction at the current node. On arcs that is wrong; leftmost-turn selection can drop faces (circle + chord).
- Add `sketch_edge_incoming_dir_2d` and use it for `incoming_dir` in `Sketch_topo::update_faces_`.
- Regression test `UpdateFaces_CircleWithChord_IncomingArcTangent`.
- Graphical Debugging: `to_boost_ls`, `segment_2d` / `ring_2d` as Ring, `tools/ezycad_graphical_debugging.xml` (https://github.com/awulkiew/graphical-debugging).

## Files Changed

- `src/sketch_edge.cpp`, `src/sketch_edge.h`, `src/sketch_topo.cpp`
- `src/utl_geom.cpp`, `src/utl_geom.h`, `src/utl_geom_boost.inl`
- `src/doc/sketch.md`, `src/doc/utility.md`
- `tests/sketch_tests.cpp`
- `tools/ezycad_graphical_debugging.xml`
- `agents/drafts/issues/active/gh-184-sketch-face-walk-incoming-arc-tangent.md`
- `agents/drafts/prs/active/gh-185-sketch-face-walk-incoming-arc-tangent.md`

## Related

- Issue: https://github.com/trailcode/EzyCad/issues/184
- Follow-up to #166; does not address #165

## Test Plan

- [x] Build `EzyCad_tests` (Debug)
- [x] `EzyCad_tests --gtest_filter=Sketch_test.UpdateFaces_CircleWithChord_IncomingArcTangent`
- [ ] Manual: circle + chord sketch confirms both faces
- [ ] Optional: Geometry Watch with `tools/ezycad_graphical_debugging.xml`

## Post-merge

- [ ] Archive issue/PR drafts
- [ ] Consider `CHANGELOG.md` `[Unreleased]` entry if not already present
