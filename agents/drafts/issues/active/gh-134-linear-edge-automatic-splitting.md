# Document automatic splitting of linear edges on intersections

**Suggested labels:** `docs`, `sketch`, `enhancement`

---

## Title (GitHub)

Document how linear edges are automatically split if they intersect other edges in sketches

## Body (GitHub)

### Summary

Added clear, user-facing documentation explaining the automatic splitting behavior for straight (linear) edges when they intersect existing straight edges or snap their endpoints to existing midpoints. This is the key mechanism that produces T-junctions, crossings, and multiple separate faces from a single sketch (e.g. adding a vertical mid-line through a square or rectangle cleanly divides it into two rectangular faces suitable for separate extrusion or other operations).

### Problem

- The user guide previously only mentioned splitting in the context of the "Add Node" tool's mid-point snap.
- There was no explanation of the general case that happens when using the Line Edge tool or Multi-Line Edge tool to draw a new straight segment that crosses or touches existing straight geometry.
- Users adding internal edges (splitters, dividers, etc.) had no guidance on the expected automatic topology updates and resulting faces.
- The behavior is important for face creation, extrusion targets, etc., but was undocumented from the end-user perspective.

### Implemented scope (for drafts after work begins)

**Documentation:**

- `docs/usage-sketch.md`:
  - Added a new row to the "Sketch snapping" table: "**Automatic splitting on edge intersections**" describing the observable behavior for the Line/Multi-line tools (existing edges split at interior intersections; new edge subdivided as needed; also triggered by endpoint midpoint snaps).
  - Added a matching practical Tips bullet under the Sketch snapping section.
  - Added a parallel bullet under the single Line Edge Creation Tools "Tips" section.
- `CHANGELOG.md`: Added concise user-focused entry under [Unreleased] / Sketch / 2D Topology (emphasizing the documented behavior; previous dev-heavy text about internal rewrite was condensed/removed per review).

**Context / related work (no behavior change in this PR):**
- Unit tests already exercise the behavior extensively (`AddSquareThenMidEdge_*` tests for both orders and full GUI draw-direction paths for the splitter; `UpdateFaces_BridgeEzy_*` for bridge+hole case from user-provided .ezy; plus earlier crossing/T-junction/hole tests).
- This docs change is the public-facing counterpart to the ongoing 2D topology / face extraction work on the branch (the internal rewrite attempt that was reverted because it hung is not mentioned in user docs).

### Acceptance criteria

- [ ] User guide section accurately and concisely describes only the observable user behavior (no implementation details such as walkers, filters, adjacency sorting, etc.).
- [ ] Changelog entry is present and user-focused.
- [ ] Docs build succeeds and rendered page looks good.
- [ ] Matches real behavior confirmed by existing unit tests and manual GUI use (Line tool crossings, midpoint snaps on existing edges, square/rect + mid splitter, etc.).
- [ ] No user-visible behavior changes; purely documentation.

### Files touched

- `docs/usage-sketch.md`
- `CHANGELOG.md`
- `agents/drafts/issues/active/gh-134-linear-edge-automatic-splitting.md` (this draft)

### Related

- GitHub issue: https://github.com/trailcode/EzyCad/issues/134 (created earlier in the 2D-topology thread; covers the broader face extraction, splitter direction sensitivity, bridge cases, and the reverted internal rewrite).
- PR for this docs work will reference the above.
- Branch: `Trailcode/2D-topology`
- Earlier agents issues on sketch snapping, topology, and docs (e.g. 006-sketch-snap-unification-and-docs.md, 009-sketch-tool-precise-input-docs-and-options-help.md).
- `agents/drafts/issues/_template.md` and `agents/drafts/prs/_template.md` templates.
- `agents/conventions/user-docs-sync.md` (not strictly required as this is clarification of long-standing behavior rather than a brand-new user-visible feature).

### Test plan (for implementation drafts)

- [ ] Manual review of the added text against the actual code behavior (via the unit tests that cover square+mid in both orders, GUI line path in both draw directions, add-node mid snap splits, crossings, T-junctions).
- [ ] `sphinx-build -b html -W docs docs/_build` and spot-check the rendered usage-sketch.md page (snapping table and tips).
- [ ] Cross-check that the description does not promise anything the current stable implementation doesn't deliver (the internal rewrite that would have removed the orientation filter was reverted).
- [ ] Verify the language is consistent with the rest of the guide (tables, bold, practical tone, links to related extrusion/boolean sections).
- [ ] No new code/tests in this specific change (the supporting tests were added in prior steps on the branch).

---

**Post-creation notes (for the draft maintainer):**
- Once the corresponding PR is opened, update this file with the PR number and link.
- After merge, consider whether any "See also" cross-links in other sections (e.g. Rectangle/Square/Slot tool descriptions) would benefit from a one-line pointer to the new snapping row.
- The goal was strictly user documentation of the splitting behavior that has existed for the automatic topology maintenance. Dev-only details (the `!is_face_clockwise_ break`, the recent rewrite attempt, exact internal algorithms) were deliberately excluded.