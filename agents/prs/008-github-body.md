Closes https://github.com/trailcode/EzyCad/issues/142

## Summary

- **Operational axis:** toolbar renamed to **Operational axis**; axis line visible only in that mode; after definition, permanent **+** markers and sketch snap suppressed for clearer mirror/revolve selection; axes persist in sketch JSON.
- **Revolve:** best-effort conversion of edge revolutions to solids; contextual **?** help on revolve angle field.
- **Shape info:** right-click shape name or **M** button opens topology/property dialog.
- **Fixes:** shape highlight in sketch operation-axis mode; related GUI/settings refactors on the branch.

Closes operational-axis visibility/snap/persistence work.

## Test plan

- [ ] Build `EzyCad` and `EzyCad_tests` (Release).
- [ ] Run `OperationAxis_JSON_RoundTrip` and mirror/revolve tests.
- [ ] Manual: Operational axis placement, snap suppression after define, Clear axis, mode switch visibility.
- [ ] Manual: Revolve closed edge profile; Shape info shows Solid when conversion succeeds.
- [ ] Docs: review `#operation-axis-tool` and sketch snapping section in `usage-sketch.md`.

## Notes

- Snap suppression applies only when an axis **exists** in Operational axis mode (not during initial two-point placement).
- Axis visibility is mode-gated; stored axis survives tool switches and file reload.
