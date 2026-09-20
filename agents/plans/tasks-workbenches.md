---
status: partial
topic: tasks-workbenches
depends_on: null
blocks: [assembly-inspection-mode]
github_issue: null
---

# Tasks / workbenches (toolbar filter)

**Load only when** the prompt is about Sketch / Design / Workbench task buttons, the tools-row filter, or Workbench vs Design Shape List. Skip otherwise ([token-lean](../conventions/token-lean.md)). Index: [plans/README.md](README.md).

Shipped: three tasks; tools row shows only the current task (option 1: hide unrelated modes). Workbench idle is `Mode::Workbench_inspection`. Move / Rotate / Align shafts are Workbench; Scale stays Design. Hotkeys auto-switch task (G / R / J -> Workbench, S -> Design).

## Workbench vs Design lists

Same `Occt_view::m_shps`, **different views**. Do not add a second shape store.

| Task      | List shows                                               | Duplicate / array                                        |
| --------- | -------------------------------------------------------- | -------------------------------------------------------- |
| Design    | Bodies / features / groups (Part Origin / planes locked) | Clone or feature-array of geometry                       |
| Workbench | Parts (groups / root bodies until wrapped); hide inners  | New placement of the same Part (instances after phase 3) |

Polar array of **parts** belongs on Workbench after placement. Polar array of a body can stay Design.

## Related

- Placement / Parts: [shape-list-hierarchy-phase3.md](shape-list-hierarchy-phase3.md)
- Arrange idle: [assembly-inspection-mode.md](assembly-inspection-mode.md)
- Modes / toolbar: [src/doc/gui.md](../../src/doc/gui.md)
