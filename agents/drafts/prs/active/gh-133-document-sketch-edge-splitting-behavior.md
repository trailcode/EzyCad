# PR - Document automatic splitting of linear edges on intersections (user guide + changelog)

## Title

Docs: document how linear edges are automatically split when they intersect other edges in sketches

## Summary

- Added user-facing documentation for the automatic splitting of straight (linear) edges on interior intersections (and when endpoints snap to existing midpoints). This is the mechanism behind T-junctions, crossings, and multiple faces from one sketch (e.g. a mid vertical drawn through a square/rectangle produces two proper rectangular faces).
- Updated the Sketch snapping table and added practical Tips bullets in the appropriate sections.
- Added a concise user-focused entry to `CHANGELOG.md` under [Unreleased] (the previous longer dev-experiment text was condensed).
- This change is purely documentation; no behavior or code changes. It complements the unit tests and the broader 2D topology work already on the branch.

Why the change was needed: The splitting behavior has been present (and exercised by tests for square+mid in both orders, GUI draw directions, bridge cases, etc.), but it was never clearly explained to end users in the guide. The only prior mention was the Add Node mid-point snap case.

Notable design decisions:
- Kept the text strictly user-observable (no mention of the internal left-most walker, orientation filter, the recent rewrite attempt that hung and was reverted, adjacency sorting, etc.).
- Followed the existing table + Tips style used elsewhere in usage-sketch.md.
- Focused the changelog entry on the documented behavior rather than internals.

## Files Changed

- `docs/usage-sketch.md` (new table row "Automatic splitting on edge intersections" + Tips bullets under Sketch snapping and the single Line Edge tool)
- `CHANGELOG.md` (user-focused entry under Sketch / 2D Topology)
- `agents/drafts/issues/active/gh-134-linear-edge-automatic-splitting.md` (this issue draft)
- `agents/drafts/prs/active/gh-133-document-sketch-edge-splitting-behavior.md` (this PR draft)

## Related

- Issue: https://github.com/trailcode/EzyCad/issues/134 (the broader 2D topology / face extraction thread that includes square+mid direction sensitivity, the bridge.ezy case, the reverted internal rewrite of update_faces_, and the GH artifacts created earlier)
- Branch: `Trailcode/2D-topology`
- Existing PR on the branch (#133) — this docs work can be folded in or referenced there.
- Other drafts: `agents/drafts/issues/009-...`, `agents/drafts/prs/004-...` (prior sketch docs work)
- Templates: `agents/drafts/issues/_template.md` and `agents/drafts/prs/_template.md`
- `agents/conventions/user-docs-sync.md` — reviewed; this is clarification of existing long-standing behavior (no brand-new feature), so no additional sync steps required beyond the changelog + guide update.

## Test Plan

- [ ] Build `EzyCad_lib` (Debug + Release) and `EzyCad_tests` (no code changes, but verify nothing is broken).
- [ ] Run relevant tests (`EzyCad_tests --gtest_filter="*AddSquareThenMidEdge*|*BridgeEzy*|*FaceWithHole*"`) — these already cover the documented splitting behavior (they passed before this docs-only change).
- [ ] Manual verification steps for the feature / fix:
  - Open the rendered user guide (or `sphinx-build -b html -W docs docs/_build`).
  - Confirm the new table row and Tips bullets appear under "Sketch snapping" and under "Line Edge Creation Tools".
  - Spot-check that the description matches real behavior (draw crossing lines, snap to mids, add mid splitter to square/rect in different orders — faces divide as described).
- [ ] Docs build (`sphinx-build -b html -W docs docs/_build`) and review rendered pages — no warnings, formatting consistent with surrounding content.
- [ ] Check in-app behavior (no changes; the documented splitting is already active in the Line/Multi-line tools and when tools internally create edges).
- [ ] Verify no regressions in related areas (snapping, face creation for extrusion/boolean, rectangle/square/slot tools, Add Node mid splits).
- [ ] (User-visible docs change) cross-check `agents/conventions/user-docs-sync.md` — satisfied by the changelog entry and the fact that this documents pre-existing behavior.

## Notes

- The heavy developer-oriented section that was previously added to usage-sketch.md (detailing the internal rewrite, the `!is_face_clockwise_ break`, the hang, etc.) was reverted by the author because "good for devs, but users might not care." This PR contains only the clean user-facing description requested.
- The internal rewrite attempt (angular sorting so the early break would no longer be required, always-normalize, geometric sort of m_faces, fan successor logic with straight-turn bias) is still present under `#if 0` in `src/sketch.cpp` for future reference, but is not active and is not mentioned in the user guide.
- Current stable behavior (with the orientation filter for stability in some addition/draw orders) is what the new docs describe.
- Unrelated files or workspace state: The broader 2D-topology work on the branch includes the new unit tests for the splitter and bridge cases; those landed earlier.
- CI / platform specifics: None (pure docs + changelog).

## Post-merge (checklist for the author)

- [ ] Update any open issue drafts or related PRs (e.g. comment on #133 and/or #134).
- [ ] Close linked GitHub issue(s) if fully addressed (the new issue #134 tracks the larger effort; this specific docs task can be closed once merged).
- [ ] Consider adding to `CHANGELOG.md` `[Unreleased]` if not already done (it is).
- [ ] Optionally add one-line "See also" pointers in the Rectangle/Square/Slot tool sections if reviewers feel it would help discoverability.