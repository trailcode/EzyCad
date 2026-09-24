---
status: partial
topic: tasks-workbenches
depends_on: null
blocks: [assembly-inspection-mode]
github_issue: 268
---

# Tasks / workbenches (toolbar filter)

**Load only when** the prompt is about Sketch / Design / Workbench task buttons, the tools-row filter, or Workbench vs Design Shape List. Skip otherwise ([token-lean](../conventions/token-lean.md)). Index: [plans/README.md](README.md).

Shipped: three tasks; tools row shows only the current task. Design has `Design_move` / `Design_rotate` / `Scale` / `Design_shaft_align` (bake for CSG). Workbench has `Workbench_move` / `Workbench_rotate` / `Workbench_shaft_align` (instance pose). G / R / J follow the current task; S enters Design Scale.

## Workbench vs Design lists

Two stores, one document: Design `Occt_view::m_shps` and Workbench `Occt_view::m_wbk_shps`. Workbench leaves start as **geometry links** (`source_id`). **Unlink** deep-copies local geom and clears `source_id`.

| Task      | Store / pane                  | What a row is                                 |
| --------- | ----------------------------- | --------------------------------------------- |
| Design    | `m_shps` / Shape List         | Bodies / groups you model                     |
| Workbench | `m_wbk_shps` / Workbench List | Instances with their own frame (link or copy) |

Copy Shape List solids or groups onto the Workbench (**Add to Workbench**, context menu, or drag). Design fillet/chamfer (same `Shape_id`) refreshes linked local geom. Design move/rotate bake does not move instances. Workbench Move/Rotate/Align update the instance frame only. **Unlink** (Workbench List button or row menu, `unlink_workbench`) freezes the instance; undo restores the link. Deleting a Design source removes its Workbench links and leaves unlinked copies. `.ezy` writes `workbench[]` (ids, `sourceId`, frame; `geom` only when unlinked). Selecting or right-clicking a list row switches to that pane's task (`GUI::ensure_task_`) so Reset / Show axes / set-from-face run against the visible store.

Polar array of **parts** can follow on Workbench after instances exist. Polar array of a body stays Design.

## Related

- Placement / Parts: [shape-list-hierarchy-phase3.md](shape-list-hierarchy-phase3.md)
- Arrange idle: [assembly-inspection-mode.md](assembly-inspection-mode.md)
- Modes / toolbar: [src/doc/gui.md](../../src/doc/gui.md)
