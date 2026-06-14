# EzyCad 2D Sketching Guide

This guide covers all 2D sketching tools and operations in EzyCad. For the main usage guide (workflow, 3D modeling, file operations, etc.), see [usage.md](usage.md).

## Table of Contents
1. [2D Sketching](#2d-sketching)
2. [Sketch snapping](#sketch-snapping)
3. [Hotkeys](#hotkeys)
4. [Line Edge Creation Tools](#line-edge-creation-tools)
5. [Multi-Line Edge Tool](#multi-line-edge-tool)
6. [Circle Creation Tools](#circle-creation-tools)
7. [Circle Creation Workflow](#circle-creation-workflow)
8. [Arc Segment Creation Tool](#arc-segment-creation-tool)
9. [Rectangle and Square Creation Tools](#rectangle-and-square-creation-tools)
10. [Slot Creation Tool](#slot-creation-tool)
11. [Operation Axis Tool](#operation-axis-tool)
12. [Dimension Tool](#dimension-tool)
13. [Add Node Tool](#add-node-tool)
14. [Create Sketch from Planar Face Tool](#create-sketch-from-planar-face-tool)
15. [Image underlay](#image-underlay)

---

## 2D Sketching
1. **Basic Tools**
   - ![Line Edge Tool](res/icons/Sketcher_Element_Line_Edge.png) [Create line edges](#line-edge-creation-tools)
   - ![Multi-line Edge Tool](res/icons/ls.png) [Draw multi-line edges](#multi-line-edge-tool)
   - ![Arc Segment Tool](res/icons/Sketcher_Element_Arc_Edge.png) [Add arc segments](#arc-segment-creation-tool)
   - ![Circle Tool](res/icons/Sketcher_CreateCircle.png) [Create circles](#circle-creation-tools)
   - ![Rectangle Tool](res/icons/Sketcher_CreateRectangle.png) ![Square Tool](res/icons/Sketcher_CreateSquare.png) [Draw rectangles and squares](#rectangle-and-square-creation-tools)
   - ![Slot Tool](res/icons/Sketcher_CreateSlot.png) [Add slots](#slot-creation-tool)
   - ![Dimension Tool](res/icons/TechDraw_LengthDimension.png) [Dimension tool](#dimension-tool)
   - ![Add Node Tool](res/icons/Sketcher_CreatePoint.png) [Add nodes](#add-node-tool)

2. **Sketch Operations**
   - ![Operation Axis Tool](res/icons/Sketcher_MirrorSketch.png) [Define operation axis](#operation-axis-tool) - Define a reference axis, then use the **Mirror** and **Revolve** controls (in the Options panel) to mirror selected sketch edges or revolve edges/faces into 3D solids.
   - ![Create Sketch from Planar Face Tool](res/icons/Macro_FaceToSketch_48.png) [Create sketch from planar face](#create-sketch-from-planar-face-tool)

## Sketch snapping

While you draw or place points in sketch mode, EzyCad helps you align to existing geometry. **Snap dist**, **Snap guide mode**, and **All co-axial nodes** are in the Options panel; **Snap guide color**, **Snap guide line width**, and the same mode/co-axial toggles are in **Settings -> Sketch** (see [usage-settings.md](usage-settings.md#sketch-tools)).

| | |
| ---: | --- |
| **Snap distance** | Larger **Snap dist** values let snaps engage from farther away (screen pixels, converted to the sketch plane at the cursor). |
| **Snap guides** | **Snap guide mode**: *Traditional* (local markers at guide intersections), *Fullscreen* (view-spanning axis lines), or *Both*. A separate checkbox **All co-axial nodes** (in the sketch Options panel and in Settings) enables *global* mode: when on, full horizontal and vertical guide lines + markers are shown for *all* nodes in the current sketch and all other visible sketches (the complete set of co-axial alignments). When off (default), only the closest node per active axis is annotated (classic closest-relative behavior). |
| **Axis alignment** | Near a snap target, the pick can align to that point's **X** or **Y** on the sketch plane; guides show which axis is active. When **both** axes align to the **same** point, the cursor **locks to that vertex**. |
| **Mid-point snap (Add node)** | A click near a **straight** edge (not at its ends) snaps onto the segment and **splits** it at commit time (see [Add node tool](#add-node-tool)). Separate from vertex lock. |
| **Automatic splitting on edge intersections** | When you add a new straight (linear) edge using the Line Edge tool or Multi-Line Edge tool, if it crosses or touches the interior of any existing straight edge, the existing edge is automatically split at the intersection point. The new edge is also subdivided into atomic segments where needed. The same splitting occurs when an endpoint of the new edge snaps to the midpoint of an existing edge. This produces correct T-junctions (3 edges), crossings (4 edges), and cleanly divided faces from a single sketch. Arcs have special internal handling and do not trigger the same linear splits. |
| **Other visible sketches** | Nodes from **other visible sketches** are projected onto the current sketch plane and act as snap targets (same distance rules). Useful for multi-sketch layouts and tools such as **polar duplicate** that pick sketch points. |

**Angle constraint:** When a line or add-node rubber band has an active angle constraint, vertex and axis snap may be disabled or relaxed so the typed angle stays exact (see each tool's section).

**Tips:**
- For precise corners, approach a vertex until both horizontal and vertical guides appear, then click.
- Automatic **edge midpoints** are snap targets but do not show **+** markers and are not listed under **Nodes** in the [Sketch List](usage.md#sketch-list) (that list is user-placed points only).
- Adding a new straight edge that intersects existing straight edges (or snaps to their midpoints) automatically splits the intersected edges. This is the main way to divide a closed shape (rectangle, square, slot, or freehand closed profile) into multiple separate faces for individual extrusion or other operations. No manual "split" command is needed. The resulting faces remain valid even if you add the splitting edge before or after the outer shape.

## Hotkeys

Common keyboard shortcuts (hotkeys) while working in 2D sketch mode or with sketch tools. Tool-specific shortcuts are also listed in each tool's section below for quick reference during that workflow.

### Common sketch hotkeys

| Hotkey                        | Action |
| ---                           | --- |
| <kbd>Tab</kbd>                | Open precise distance / length / radius / size input dialog (most creation tools) |
| <kbd>Shift</kbd>+<kbd>Tab</kbd> | Open angle (degrees) input dialog for constrained line / multi-line / add-node placement |
| <kbd>Esc</kbd>                | Cancel the current tool, step, or rubber-band preview |
| <kbd>Enter</kbd>              | Confirm current numeric input or finalize the step |
| <kbd>D</kbd>                  | Activate the Dimension tool |
| <kbd>Shift</kbd>+<kbd>D</kbd> / <kbd>Delete</kbd> / <kbd>Backspace</kbd> | Delete the selected sketch element(s) or dimension |
| <kbd>Right-click</kbd>        | In multi-line / sequences: complete current item and continue, or finish the whole operation |

**Notes:**
- <kbd>Tab</kbd> / <kbd>Shift+Tab</kbd> work even when focus is in the 3D view (they are routed to the active sketch tool for precise entry).
- When an angle constraint is active, node snapping is typically relaxed or disabled to preserve the exact angle.
- Global hotkeys (mode switches like <kbd>G</kbd> Move, undo, view navigation, selection filters) are in the main [Hotkeys](usage.md#hotkeys) section.

### Move / rotate / polar axis constraints (when those options are active)

| Hotkey   | Action |
| ---      | --- |
| <kbd>X</kbd> | Toggle / cycle X axis constraint (move) or rotation axis (rotate / polar) |
| <kbd>Y</kbd> | Toggle / cycle Y |
| <kbd>Z</kbd> | Toggle / cycle Z |

See the individual tool sections (and [usage.md#hotkeys](usage.md#hotkeys)) for full context and additional view / general hotkeys.

### Line Edge Creation Tools
![Line Edge Tool](res/icons/Sketcher_Element_Line_Edge.png)

EzyCad provides tools for creating individual line edges in sketch mode, allowing you to build complex geometries one edge at a time.

#### Single Line Edge Tool

The single line edge tool allows you to create straight line segments between two points.

**Features:**

| | |
| ---: | --- |
| **Two-point creation** | Click to set the start point, then click to set the end point |
| **Real-time preview** | See the line shape while moving the mouse |
| **Precise length control** | Use the distance input dialog (<kbd>Tab</kbd> key) for exact line lengths |
| **Angle constraint** | Use the angle input dialog (<kbd>Shift</kbd>+<kbd>Tab</kbd>) to constrain the line to a specific angle |
| **Snap support** | Automatically snaps to existing nodes and geometry (disabled when angle constraint is active) |
| **Dimension annotations** | Optional length dimensions can be displayed |

**How to use:**
1. ![Line Edge Tool](res/icons/Sketcher_Element_Line_Edge.png) Select the **Line Edge** tool from the toolbar
2. Click to set the start point of the line
3. Move the mouse to see a preview of the line
4. Click to set the end point, or use input dialogs for precise control:
   - Press <kbd>Tab</kbd> to enter an exact length value
   - Press <kbd>Shift</kbd>+<kbd>Tab</kbd> to enter an exact angle value (in degrees)
   - When using both: apply angle (<kbd>Shift</kbd>+<kbd>Tab</kbd>) first, then distance (<kbd>Tab</kbd>).
5. The line edge will be created and added to your sketch

**Shortcuts:**

| | |
| ---: | --- |
| <kbd>Tab</kbd> | Open distance input dialog for precise length control |
| <kbd>Shift</kbd>+<kbd>Tab</kbd> | Open angle input dialog to constrain the line to a specific angle (after first point is set) |
| <kbd>Escape</kbd> | Cancel the current line creation |
| <kbd>Enter</kbd> | Finalize the line (if using distance or angle input) |
| <kbd>Right-click</kbd> | Complete the current line and start a new one |

**Angle Constraint:**
- After setting the first point, press **Shift+Tab** to open the angle input dialog
- Enter the desired angle in degrees (0 deg = horizontal right, 90 deg = vertical up, counterclockwise)
- Once the angle is entered, the line segment is constrained to that angle
- You can still move the mouse to adjust the length while maintaining the angle
- The distance input (<kbd>Tab</kbd>) can still be used in combination with the angle constraint
- **Order when using both**: When requiring both an angle and a distance constraint, apply the angle constraint first (<kbd>Shift</kbd>+<kbd>Tab</kbd>), then the distance constraint (<kbd>Tab</kbd>).
- **Note**: When an angle constraint is active, snapping to nodes is disabled to maintain the angle precision

**Tips:**
- Use the snap feature to create lines that connect precisely to existing geometry
- Lines can be used as construction geometry or as part of your final design
- The line tool works in any sketch plane
- Multiple line edges can be created in sequence by right-clicking after each line
- **New straight edges automatically split existing straight edges** at interior intersection points (or when snapping to an existing midpoint). This is how you divide a closed profile into multiple faces (for separate extrusions, etc.) — just draw the crossing or connecting line; no extra "split" step is required. The same logic applies to multi-line sequences.

### Multi-Line Edge Tool

![Multi-line Edge Tool](res/icons/ls.png)

The multi-line edge tool allows you to create multiple connected line segments in a single operation, making it efficient for drawing continuous paths, polylines, or complex connected geometries.

**Features:**

| | |
| ---: | --- |
| **Continuous edge creation** | Click multiple points to create a chain of connected line segments |
| **Real-time preview** | See each edge shape while moving the mouse before clicking |
| **Precise length control** | Use the distance input dialog (<kbd>Tab</kbd> key) for exact edge lengths |
| **Angle constraint** | Use the angle input dialog (<kbd>Shift</kbd>+<kbd>Tab</kbd>) to constrain the current edge to an angle (in degrees) |
| **Snap support** | Automatically snaps to existing nodes and geometry |
| **Distance annotations** | Real-time distance display for the current edge being drawn |
| **Flexible finalization** | Continue adding edges until you right-click to finalize the entire sequence |

**How to use:**
1. ![ls](res/icons/ls.png) Select the **Multi-line Edge** tool from the toolbar
2. Click to set the first point (start of the first edge)
3. Move the mouse to see a preview of the first edge
4. Click to set the second point (end of first edge, start of second edge), or use input for precise control:
   - Press <kbd>Tab</kbd> to enter an exact length value
   - Press <kbd>Shift</kbd>+<kbd>Tab</kbd> to enter an exact angle (in degrees) for the current edge
   - When using both angle and distance: apply angle (<kbd>Shift</kbd>+<kbd>Tab</kbd>) first, then distance (<kbd>Tab</kbd>).
5. Continue clicking to add more connected edges, each new click starts a new edge from the previous edge's end point
6. Press <kbd>Right-click</kbd> to finalize the entire multi-line sequence and add all edges to your sketch

**Shortcuts:**

| | |
| ---: | --- |
| <kbd>Tab</kbd> | Open distance input dialog for precise length control of the current edge |
| <kbd>Shift</kbd>+<kbd>Tab</kbd> | Open angle input dialog to constrain the current edge to an angle (after first point is set) |
| <kbd>Escape</kbd> | Cancel the entire multi-line creation operation |
| <kbd>Enter</kbd> | Finalize the current edge length (if using distance input) and continue to the next edge |
| <kbd>Right-click</kbd> | Finalize the entire multi-line sequence and complete the operation |

**Workflow details:**
- Each click after the first creates a new edge connected to the previous edge's end point
- After entering a distance with Tab, the tool automatically starts a new edge from the end of the previous one
- The distance annotation shows the length of the edge currently being drawn
- All edges in the sequence are added to the sketch together when you right-click to finalize

**Tips:**
- Use the snap feature to create multi-line edges that connect precisely to existing geometry
- Multi-line edges are ideal for creating continuous paths, outlines, or complex connected shapes
- The tool works in any sketch plane
- Keep clicking to add more edges - there's no limit. For example, you can click 3 times to create 2 edges, or 10 times to create 9 edges. All edges remain temporary until you right-click, which adds all of them to your sketch at once
- If you make a mistake, press <kbd>Escape</kbd> to cancel the entire sequence and start over
- The last edge in the sequence will be removed if it hasn't been finalized (no end point set) when you right-click

**Comparison with Single Line Edge Tool:**
- **Single Line Edge**: Creates one edge at a time, finalizes automatically after two points
- **Multi-Line Edge**: Creates multiple connected edges in sequence, requires right-click to finalize
- Use single line edges when you need individual disconnected segments
- Use multi-line edges when you need a continuous chain of connected segments

### Circle Creation Tools

![Circle Tool](res/icons/Sketcher_CreateCircle.png)

EzyCad provides a method for creating circles in sketch mode using the **center-radius approach**.

#### Center-Radius Circle Tool

The center-radius circle tool allows you to create circles by defining a center point and a radius point.

**Features:**

| | |
| ---: | --- |
| **Two-point creation** | Click to set the center, then click to set the radius point |
| **Real-time preview** | See the circle shape while dragging the radius point |
| **Precise radius control** | Use the distance input dialog (<kbd>Tab</kbd> key) for exact radius values |
| **Angle constraint** | Use the angle input dialog (<kbd>Shift</kbd>+<kbd>Tab</kbd>) to constrain the direction from the center to the radius point |
| **Snap support** | Automatically snaps to existing nodes and geometry (disabled when angle constraint is active) |

**How to use:**
1. ![Circle Tool](res/icons/Sketcher_CreateCircle.png) Select the **Circle** tool from the toolbar
2. Click to set the center point of the circle
3. Move the mouse to see a preview of the circle
4. Click to set the radius point, or use input dialogs for precise control:
   - Press <kbd>Tab</kbd> to enter an exact radius value
   - Press <kbd>Shift</kbd>+<kbd>Tab</kbd> to enter an exact angle value (in degrees, CCW from +X) to constrain the direction from the center
   - When using both: apply angle (<kbd>Shift</kbd>+<kbd>Tab</kbd>) first, then radius (<kbd>Tab</kbd>).
5. The circle will be created and added to your sketch

**Shortcuts:**

| | |
| ---: | --- |
| <kbd>Tab</kbd> | Open distance input dialog for precise radius control |
| <kbd>Shift</kbd>+<kbd>Tab</kbd> | Open angle input dialog to constrain the radius direction from the center (after center point is set) |
| <kbd>Escape</kbd> | Cancel the current circle creation |
| <kbd>Enter</kbd> | Finalize the circle (if using distance input) |

**Angle Constraint:**
- After setting the center point, press **Shift+Tab** to open the angle input dialog
- Enter the desired angle in degrees (0 deg = horizontal right from center, 90 deg = vertical up, counterclockwise)
- Once the angle is entered, the radius point placement is constrained to that direction from the center
- You can still move the mouse to adjust the radius (distance from center) while maintaining the direction, or use <kbd>Tab</kbd> for an exact radius value
- The distance input (<kbd>Tab</kbd>) can still be used in combination with the angle constraint
- **Order when using both**: When requiring both an angle and a radius constraint, apply the angle constraint first (<kbd>Shift</kbd>+<kbd>Tab</kbd>), then the radius constraint (<kbd>Tab</kbd>).
- **Note**: When an angle constraint is active, snapping to nodes is disabled to maintain the angle precision

**Tips:**
- Use the snap feature to create circles that are precisely positioned relative to existing geometry
- The circle tool works in any sketch plane
- Circles can be used as construction geometry or as part of your final design
- Constraining the angle when placing the radius point is useful for precise directional placement (e.g. for future snapping operations involving circle nodes)

#### Three-Point Circle Tool (Planned Feature)

**Status**: Not yet implemented

The three-point circle tool is planned for future development. This feature would allow creating circles by defining three points that lie on the circle's circumference.

**Planned Features:**

| | |
| ---: | --- |
| **Three-point creation** | Click three points that lie on the circle's circumference |
| **Automatic center and radius calculation** | The system would compute the center and radius from the three points |
| **Geometric validation** | Ensure the three points are not collinear |

**Note**: ![Sketcher_Create3PointCircle](res/icons/Sketcher_Create3PointCircle.png) The toolbar icon exists but the functionality is not yet implemented.

### Circle Creation Workflow

The circle tool follows this workflow:

1. **Activate Tool**: Select the circle creation mode
2. **Point Placement**: Click to place the center point, then click to place the radius point (use <kbd>Tab</kbd> for exact radius or <kbd>Shift</kbd>+<kbd>Tab</kbd> for angle constraint on the direction from center)
3. **Preview**: See real-time preview of the circle as you move the mouse
4. **Finalization**: Click to complete the circle creation
5. **Integration**: The circle becomes part of the sketch and can be used for further operations
- Precise control with <kbd>Tab</kbd> / <kbd>Shift</kbd>+<kbd>Tab</kbd> works the same as for line edges (see [Line Edge Creation Tools](#line-edge-creation-tools))

**Common Operations with Circles:**

| | |
| ---: | --- |
| **Extrusion** | Select the circle face and [extrude](usage.md#extrude-sketch-face-tool-e) to create cylindrical shapes |
| **Boolean Operations** | Use circles in [cut, fuse, or common](usage.md#boolean-operations) operations |
| **Pattern Creation** | Use circles as the basis for polar arrays or other patterns |
| **Dimensioning** | Add radius or diameter dimensions to circles |

**Error Handling:**

| | |
| ---: | --- |
| **Coincident Points** | The system prevents creation of circles with zero radius |
| **Invalid Geometry** | Circles that would be too small are rejected |
| **Snap Integration** | Use existing snap points for precise circle placement |

### Arc Segment Creation Tool

![Arc Segment Tool](res/icons/Sketcher_Element_Arc_Edge.png)

The arc segment tool allows you to create circular arc edges by defining three points that lie on the arc: a start point, a middle point, and an end point.

**Features:**

| | |
| ---: | --- |
| **Three-point creation** | Click to set the start point, then the middle point, then the end point |
| **Real-time preview** | See the arc shape while moving the mouse after setting the first two points |
| **Automatic finalization** | The arc is automatically created and added to your sketch after the third point is clicked |
| **Circular arc** | Creates a smooth circular arc that passes through all three points |
| **Snap support** | Automatically snaps to existing nodes and geometry |
| **Unique points** | All three points must be different (cannot be coincident) |

**How to use:**
1. ![Sketcher_Element_Arc_Edge](res/icons/Sketcher_Element_Arc_Edge.png) Select the **Arc Segment** tool from the toolbar
2. Click to set the start point of the arc (first point)
3. Click to set a point on the arc between start and end (middle point)
4. Move the mouse to see a preview of the arc
5. Click to set the end point of the arc (third point)
6. The arc segment will be automatically created and added to your sketch

**Point order:**

| | |
| ---: | --- |
| **First click** | Start point - where the arc begins |
| **Second click** | Middle point - a point that lies on the arc between start and end |
| **Third click** | End point - where the arc ends |

**Shortcuts:**

| | |
| ---: | --- |
| <kbd>Escape</kbd> | Cancel the current arc creation (clears all points) |
| **Note** | The arc is automatically finalized after the third point, so no manual finalization is needed |

**Tips:**
- The three points define a unique circular arc that passes through all of them
- Use the snap feature to create arcs that connect precisely to existing geometry
- The arc tool works in any sketch plane
- Arc segments can be used as part of closed shapes that form faces
- The middle point helps define the arc's curvature and direction
- All three points must be unique - clicking the same point twice will be ignored

**Technical details:**
- The arc is created using the three points to define a circle, then trimming it to the arc segment
- Internally, the arc is represented as two connected edges for proper topology
- Arc segments can be combined with straight edges to create complex closed shapes

**Comparison with Circle Tool:**
- **Circle Tool**: Creates a full circle from center and radius point (2 points)
- **Arc Segment Tool**: Creates a partial arc from three points on the arc (3 points)
- Use circles when you need a complete circular shape
- Use arc segments when you need a curved edge that's part of a larger shape

### Rectangle and Square Creation Tools

EzyCad provides three tools for creating rectangular shapes in sketch mode: square, rectangle from two points, and rectangle with center point.

#### Square Tool

![Square Tool](res/icons/Sketcher_CreateSquare.png)

The square tool allows you to create perfect squares by defining a center point and an edge midpoint.

**Features:**

| | |
| ---: | --- |
| **Two-point creation** | Click to set the center point, then click to set the midpoint of one edge |
| **Real-time preview** | See the square shape while moving the mouse |
| **Perfect square** | Automatically ensures all sides are equal length |
| **Orientation control** | The square's orientation is determined by the direction from center to edge midpoint |
| **Precise size control** | Use the distance input dialog (<kbd>Tab</kbd> key) for exact side lengths |
| **Angle constraint** | Use the angle input dialog (<kbd>Shift</kbd>+<kbd>Tab</kbd>) to constrain the orientation (direction from center to edge midpoint) |
| **Snap support** | Automatically snaps to existing nodes and geometry (disabled when angle constraint is active) |

**How to use:**
1. ![Sketcher_CreateSquare](res/icons/Sketcher_CreateSquare.png) Select the **Square** tool from the toolbar
2. Click to set the center point of the square
3. Move the mouse to see a preview of the square
4. Click to set the midpoint of one edge (this defines both the size and orientation), or use input dialogs for precise control:
   - Press <kbd>Tab</kbd> to enter an exact side length value
   - Press <kbd>Shift</kbd>+<kbd>Tab</kbd> to enter an exact angle value (in degrees, CCW from +X) to constrain the orientation (direction from center)
   - When using both: apply angle (<kbd>Shift</kbd>+<kbd>Tab</kbd>) first, then size (<kbd>Tab</kbd>).
5. The square will be created with four edges and added to your sketch

**Shortcuts:**

| | |
| ---: | --- |
| <kbd>Tab</kbd> | Open distance input dialog for precise side length control |
| <kbd>Shift</kbd>+<kbd>Tab</kbd> | Open angle input dialog to constrain the orientation (direction from center to edge midpoint; after center point is set) |
| <kbd>Escape</kbd> | Cancel the current square creation |
| <kbd>Enter</kbd> | Finalize the square (if using distance input) |

**Angle Constraint:**
- After setting the center point, press **Shift+Tab** to open the angle input dialog
- Enter the desired angle in degrees (0 deg = horizontal right from center, 90 deg = vertical up, counterclockwise)
- Once the angle is entered, the edge midpoint placement is constrained to that direction from the center (fixing the square's orientation)
- You can still move the mouse to adjust the size (distance from center to midpoint) while maintaining the orientation, or use <kbd>Tab</kbd> for an exact side length
- The distance input (<kbd>Tab</kbd>) can still be used in combination with the angle constraint
- **Order when using both**: When requiring both an angle and a size constraint, apply the angle constraint first (<kbd>Shift</kbd>+<kbd>Tab</kbd>), then the size constraint (<kbd>Tab</kbd>).
- **Note**: When an angle constraint is active, snapping to nodes is disabled to maintain the angle precision

**Tips:**
- The distance from center to edge midpoint determines half the side length
- Use the snap feature to create squares that are precisely positioned relative to existing geometry
- The square tool works in any sketch plane
- Squares automatically form closed faces that can be [extruded](usage.md#extrude-sketch-face-tool-e)
- Constraining the angle/orientation when placing the edge midpoint is useful for precise rotated squares (e.g. for future snapping operations)

#### Rectangle Tool (Two Points)

![Rectangle Tool](res/icons/Sketcher_CreateRectangle.png)

The rectangle tool allows you to create rectangles by defining two opposite corners.

**Features:**

| | |
| ---: | --- |
| **Two-point creation** | Click to set the first corner, then click to set the opposite corner |
| **Real-time preview** | See the rectangle shape while moving the mouse |
| **Precise size control** | Use the distance input dialog (<kbd>Tab</kbd> key) for exact dimensions |
| **Angle constraint** | Use the angle input dialog (<kbd>Shift</kbd>+<kbd>Tab</kbd>) to constrain the direction from the first to the opposite corner |
| **Snap support** | Automatically snaps to existing nodes and geometry (disabled when angle constraint is active) |
| **Automatic corner calculation** | The system automatically calculates the other two corners |

**How to use:**
1. ![Sketcher_CreateRectangle](res/icons/Sketcher_CreateRectangle.png) Select the **Rectangle** tool from the toolbar
2. Click to set the first corner point
3. Move the mouse to see a preview of the rectangle
4. Click to set the opposite corner point, or use input dialogs for precise control:
   - Press <kbd>Tab</kbd> to enter exact distance values
   - Press <kbd>Shift</kbd>+<kbd>Tab</kbd> to enter an exact angle value (in degrees, CCW from +X) to constrain the direction from the first corner
   - When using both: apply angle (<kbd>Shift</kbd>+<kbd>Tab</kbd>) first, then distances (<kbd>Tab</kbd>).
5. The rectangle will be created with four edges and added to your sketch

**Shortcuts:**

| | |
| ---: | --- |
| <kbd>Tab</kbd> | Open distance input dialog for precise dimension control |
| <kbd>Shift</kbd>+<kbd>Tab</kbd> | Open angle input dialog to constrain the direction from the first to the opposite corner (after first corner is set) |
| <kbd>Escape</kbd> | Cancel the current rectangle creation |
| <kbd>Enter</kbd> | Finalize the rectangle (if using distance input) |

**Angle Constraint:**
- After setting the first corner point, press **Shift+Tab** to open the angle input dialog
- Enter the desired angle in degrees (0 deg = horizontal right, 90 deg = vertical up, counterclockwise)
- Once the angle is entered, the placement of the opposite corner is constrained to that direction from the first corner (affecting the temporary diagonal/rubber-band during placement)
- You can still move the mouse to adjust the dimensions while maintaining the direction, or use <kbd>Tab</kbd> for exact distance values
- The distance input (<kbd>Tab</kbd>) can still be used in combination with the angle constraint
- **Order when using both**: When requiring both an angle and distance constraints, apply the angle constraint first (<kbd>Shift</kbd>+<kbd>Tab</kbd>), then the distances (<kbd>Tab</kbd>).
- **Note**: When an angle constraint is active, snapping to nodes is disabled to maintain the angle precision

**Tips:**
- The two points define opposite corners of the rectangle (diagonal)
- The rectangle edges are automatically aligned with the coordinate axes
- Use the snap feature to create rectangles that are precisely positioned relative to existing geometry
- The rectangle tool works in any sketch plane
- Rectangles automatically form closed faces that can be [extruded](usage.md#extrude-sketch-face-tool-e)
- Constraining the angle/direction when placing the opposite corner is useful for precise placement during the rubber-band phase (e.g. for future snapping operations)

#### Rectangle Tool (Center Point)

![Rectangle Center Tool](res/icons/Sketcher_CreateRectangle_Center.png)

The rectangle with center point tool allows you to create rectangles by defining a center point and a corner point.

**Features:**

| | |
| ---: | --- |
| **Two-point creation** | Click to set the center point, then click to set a corner point |
| **Real-time preview** | See the rectangle shape while moving the mouse |
| **Centered creation** | The rectangle is centered on the first point |
| **Precise size control** | Use the distance input dialog (<kbd>Tab</kbd> key) for exact dimensions |
| **Angle constraint** | Use the angle input dialog (<kbd>Shift</kbd>+<kbd>Tab</kbd>) to constrain the orientation (direction from center to corner point) |
| **Snap support** | Automatically snaps to existing nodes and geometry (disabled when angle constraint is active) |

**How to use:**
1. ![Sketcher_CreateRectangle_Center](res/icons/Sketcher_CreateRectangle_Center.png) Select the **Rectangle with Center Point** tool from the toolbar
2. Click to set the center point of the rectangle
3. Move the mouse to see a preview of the rectangle
4. Click to set a corner point (defines both size and orientation), or use input dialogs for precise control:
   - Press <kbd>Tab</kbd> to enter exact distance values
   - Press <kbd>Shift</kbd>+<kbd>Tab</kbd> to enter an exact angle value (in degrees, CCW from +X) to constrain the orientation (direction from center)
   - When using both: apply angle (<kbd>Shift</kbd>+<kbd>Tab</kbd>) first, then distances (<kbd>Tab</kbd>).
5. The rectangle will be created with four edges and added to your sketch

**Shortcuts:**

| | |
| ---: | --- |
| <kbd>Tab</kbd> | Open distance input dialog for precise dimension control |
| <kbd>Shift</kbd>+<kbd>Tab</kbd> | Open angle input dialog to constrain the orientation (direction from center to corner point; after center point is set) |
| <kbd>Escape</kbd> | Cancel the current rectangle creation |
| <kbd>Enter</kbd> | Finalize the rectangle (if using distance input) |

**Angle Constraint:**
- After setting the center point, press **Shift+Tab** to open the angle input dialog
- Enter the desired angle in degrees (0 deg = horizontal right from center, 90 deg = vertical up, counterclockwise)
- Once the angle is entered, the corner point placement is constrained to that direction from the center (fixing the rectangle's orientation)
- You can still move the mouse to adjust the size (distance from center to corner) while maintaining the orientation, or use <kbd>Tab</kbd> for exact distance values
- The distance input (<kbd>Tab</kbd>) can still be used in combination with the angle constraint
- **Order when using both**: When requiring both an angle and a size constraint, apply the angle constraint first (<kbd>Shift</kbd>+<kbd>Tab</kbd>), then the size constraint (<kbd>Tab</kbd>).
- **Note**: When an angle constraint is active, snapping to nodes is disabled to maintain the angle precision

**Tips:**
- The rectangle is centered on the first point you click
- The distance from center to corner determines the rectangle's dimensions
- Use the snap feature to create rectangles that are precisely positioned relative to existing geometry
- The rectangle tool works in any sketch plane
- Rectangles automatically form closed faces that can be [extruded](usage.md#extrude-sketch-face-tool-e)
- Constraining the angle/orientation when placing the corner point is useful for precise rotated rectangles (e.g. for future snapping operations)

**Comparison of Rectangle Tools:**

| | |
| ---: | --- |
| **Rectangle (Two Points)** | Define opposite corners - useful when you know the corner positions |
| **Rectangle (Center Point)** | Define center and corner - useful when you want the rectangle centered on a specific point |
| **Square** | Always creates a perfect square - use when you need equal sides |

### Slot Creation Tool

![Slot Tool](res/icons/Sketcher_CreateSlot.png)

The slot tool allows you to create an oblong or oval-shaped slot with rounded ends. A slot consists of two semicircular arcs connected by two straight parallel edges.

**Features:**

| | |
| ---: | --- |
| **Three-point creation** | Click to set the first arc center, then the second arc center, then a point to define the radius |
| **Real-time preview** | See the slot shape while moving the mouse after setting the first two points |
| **Automatic finalization** | The slot is automatically created and added to your sketch after the third point is clicked |
| **Rounded ends** | Creates semicircular arcs at both ends with equal radius |
| **Parallel edges** | The two straight edges connecting the arcs are always parallel |
| **Precise size control** | Use the distance input dialog (<kbd>Tab</kbd> key) for exact dimensions |
| **Angle constraint** | Use the angle input dialog (<kbd>Shift</kbd>+<kbd>Tab</kbd>) to constrain the orientation (direction of the slot length between arc centers, or radius direction) |
| **Snap support** | Automatically snaps to existing nodes and geometry (disabled when angle constraint is active) |

**How to use:**
1. ![Sketcher_CreateSlot](res/icons/Sketcher_CreateSlot.png) Select the **Slot** tool from the toolbar
2. Click to set the center point of the first arc (start of slot)
3. Move the mouse to see a preview of the first edge (this will define the length and orientation of the slot)
4. Click to set the center point of the second arc (end of slot), or use input for precise control on the first edge (length/orientation of slot):
   - Press <kbd>Tab</kbd> to enter exact length value for the slot
   - Press <kbd>Shift</kbd>+<kbd>Tab</kbd> to enter exact angle (in degrees) for the slot direction
5. Move the mouse to see a preview of the slot shape
6. Click to set a point that defines the radius of the arcs, or press <kbd>Tab</kbd> to enter exact distance values (this second dimension defines the radius of the rounded ends)
   - **Important**: The radius is measured from the second arc center (the point you clicked in step 4) to this third point
   - This radius determines the cross section: the slot's cross-section dimension equals 2 * radius
7. The slot will be automatically created with two arcs and two straight edges and added to your sketch
   - When using both angle and distance on a rubber-band step: apply angle (<kbd>Shift</kbd>+<kbd>Tab</kbd>) first, then distance (<kbd>Tab</kbd>).

**Point order:**
- **First click**: First arc center - the center of the arc at one end of the slot
- **Second click**: Second arc center - the center of the arc at the other end of the slot
- **Third click**: Radius point - defines the radius of both arcs
  - The distance from the **second arc center** (second click) to this point determines the arc radius
  - The slot's cross section (perpendicular to the slot length) equals twice this radius
  - Both arcs use the same radius, creating a symmetric slot

**Shortcuts:**

| | |
| ---: | --- |
| <kbd>Tab</kbd> | Open distance input dialog for precise dimension control (length of slot on first edge, or radius on third point) |
| <kbd>Shift</kbd>+<kbd>Tab</kbd> | Open angle input dialog to constrain the orientation (direction of the slot on first edge, or radius direction on third point; after previous point set) |
| <kbd>Escape</kbd> | Cancel the current slot creation |
| <kbd>Enter</kbd> | Finalize the slot (if using distance input) |
| **Note** | The slot is automatically finalized after the third point, so no manual finalization is needed |

**Angle Constraint:**
- The tool has two main rubber-band stages where angle/distance apply:
  - After the first arc center: the rubber-band to the second arc center defines the slot length and orientation (direction of the parallel edges).
  - After the second arc center: the rubber-band (or point) for the radius defines the arc radius (cross-section width = 2 * radius); angle here can constrain the radius direction if useful.
- Press **Shift+Tab** to open the angle input dialog at the relevant rubber-band stage.
- Enter the desired angle in degrees (0 deg = horizontal right, 90 deg = vertical up, counterclockwise). This constrains the direction for that edge/dim.
- You can still adjust the corresponding distance (Tab for length or radius) while the angle is locked.
- The distance input (<kbd>Tab</kbd>) can still be used in combination with the angle constraint.
- **Order when using both**: When requiring both an angle and a distance constraint on a rubber-band step, apply the angle constraint first (<kbd>Shift</kbd>+<kbd>Tab</kbd>), then the distance constraint (<kbd>Tab</kbd>).
- **Note**: When an angle constraint is active, snapping to nodes is disabled to maintain the angle precision. The first edge primarily controls slot length/orientation; the radius point controls the rounded ends' size.

**Tips:**
- The slot length is determined by the distance between the first and second arc centers
- The radius of both arcs is determined by the distance from the second arc center to the radius point (third click)
- The slot's cross section (the dimension perpendicular to the slot length) equals 2 * radius
  - This is because each arc is a semicircle with the specified radius, extending equally in both perpendicular directions
  - For example, if you click the radius point 3 units away from the second arc center, the slot cross section will be 6 units
- Both arcs have the same radius, creating a symmetric slot shape
- The slot orientation (which dimension is length vs width/height) is determined by the direction from the first to the second arc center
- Use the snap feature to create slots that connect precisely to existing geometry
- The slot tool works in any sketch plane
- Slots automatically form closed faces that can be [extruded](usage.md#extrude-sketch-face-tool-e)
- The slot orientation is determined by the direction from the first to the second arc center
- Constraining angle on the first edge (for slot direction) or radius point is useful for precise placement (e.g. for future snapping operations)

**Technical details:**

| | |
| ---: | --- |
| **Slot structure** | Four edges: two semicircular arcs and two straight parallel edges |
| **Arcs** | Created using the arc segment functionality |
| **Straight edges** | Connect the arcs at their endpoints |
| **Closed shape** | Suitable for face creation and extrusion |

**Common use cases:**
- Creating mounting slots for screws or bolts
- Designing elongated holes for adjustment
- Creating rounded-end cutouts in parts
- Designing slots for sliding mechanisms

### Operation Axis Tool

![Operation Axis Tool](res/icons/Sketcher_MirrorSketch.png)

The operation axis tool allows you to define a reference *line* (a direction and a point it passes through) for mirroring and revolving operations in sketches. Only the direction and position matter; the length of the drawn helper segment is visual only and has no effect on the results.

**Features:**

| | |
| ---: | --- |
| **Two-point definition** | Click to set the start point, then click to set the end point of the axis line |
| **Real-time preview** | See the axis line while moving the mouse |
| **Visual length control (Tab)** | Use the distance input dialog (<kbd>Tab</kbd> key) to set the length of the *drawn helper segment* (purely visual/reference; the length has no effect on mirror or revolve results) |
| **Angle constraint** | Use the angle input dialog (<kbd>Shift</kbd>+<kbd>Tab</kbd>) to constrain the direction of the axis line |
| **Mirror operations** | Use the defined axis to mirror selected edges |
| **Revolve operations** | Use the defined axis to revolve selected edges or faces |

**How to Use:**

**Phase 1: Define the operation axis**
1. ![Sketcher_MirrorSketch](res/icons/Sketcher_MirrorSketch.png) Select the **Operation Axis** tool from the toolbar.
2. Click to set the start point of the axis line on the sketch plane.
3. Move the mouse — a live preview of the axis line follows the cursor.
4. Click a second point (or use the input dialogs) to finalize the axis:
   - <kbd>Tab</kbd>: open distance dialog to set the length of the *drawn axis helper segment* (visual only — this length does not affect mirror or revolve operations).
   - <kbd>Shift</kbd>+<kbd>Tab</kbd>: open angle (degrees, CCW from +X) dialog to constrain direction.
   - Apply angle constraint first if using both, then length.
5. The axis is now defined. You remain in the Operation Axis tool (this keeps the Options panel available for the Mirror/Revolve controls).

**Phase 2: Use Mirror or Revolve (the controls shown in the Options panel)**

Once the axis exists, the Options panel (under the **Sketch operation** heading) displays these controls (matching the UI):

- **Mirror** button — Mirrors the currently selected sketch edges across the axis.
- **Revolve** button (with adjacent numeric input, default **360.00**) — Revolves the selected edges or faces around the axis by the given angle (in degrees). 360° produces a full solid of revolution.
- **Clear axis** button — Removes the current operation axis (required before you can define a new one).

**How to Mirror:**
1. With the Operation Axis tool active and an axis defined, select the edges you want to mirror (click them or use a selection box; the mode stays active so normal sketch selection works for the subject geometry).
2. In the Options panel, click the **Mirror** button.
3. Selected straight edges are duplicated and mirrored. Arc/circle edges are handled as pairs to preserve the geometry.
4. The mirrored elements are added to the current sketch (the original selection remains until you change it).

**How to Revolve (create 3D):**
1. With the Operation Axis tool active and an axis defined, select the edges or closed faces you want to revolve.
2. (Optional) Adjust the angle in the input field next to the Revolve button (default 360.00 for a full closed revolution).
3. Click the **Revolve** button.
4. EzyCad creates a 3D solid by revolving the selected sketch geometry around the axis. The new 3D shape is added to the document and you are switched out of sketch mode into Normal (inspection) mode.
5. If the operation fails you will see a message such as "Revolve failed, ensure edges or faces on one side of operation axis."

**Redefining the Axis:**
- While an operation axis exists, clicks in the viewport are used to select edges or faces for Mirror/Revolve (the axis definition mode remains active so the Options panel buttons are available).
- To redefine the axis: click the **Clear axis** button in the options panel first (this removes the current axis), then click in the viewport to place the first point of the new axis, followed by the second point (as in Phase 1 above).

**Using the Operation Axis (UI reference):**
The controls that appear in the Options panel once an axis is defined are:

| Control | Purpose |
| --- | --- |
| **Mirror** button | Mirrors selected edges across the operation axis (stays in 2D sketch). |
| **Revolve** button + numeric field (e.g. 360.00) | Revolves selected edges or faces around the axis by the entered angle to produce a 3D solid. |
| **Clear axis** button | Manually removes the current operation axis reference. |

The Revolve numeric field accepts any float angle (in degrees); 360 is the most common for closed solids.

**Important note on axis length:**
The two points you pick define a *direction* and a point the axis passes through. The length of the drawn segment between those points is purely visual (it determines how long the reference line appears in the sketch for your convenience). It has **no effect** on the results of Mirror or Revolve. The operations always treat the axis as an infinite line in the computed direction.

**Shortcuts:**

| | |
| ---: | --- |
| <kbd>Tab</kbd> | Open distance input dialog to set the length of the drawn axis helper segment (visual only; length has no functional effect on mirror/revolve) |
| <kbd>Shift</kbd>+<kbd>Tab</kbd> | Open angle input dialog to constrain the direction of the axis (after first point is set) |
| <kbd>Escape</kbd> | Cancel the current axis definition |

**Angle Constraint:**
- After setting the first point, press **Shift+Tab** to open the angle input dialog
- Enter the desired angle in degrees (0 deg = horizontal right, 90 deg = vertical up, counterclockwise)
- Once the angle is entered, the axis line is constrained to that direction from the start point
- You can still move the mouse to adjust the length of the *drawn helper segment* while maintaining the direction, or use <kbd>Tab</kbd> to set an exact visual length (note: the length of the operation axis segment has no effect on mirror or revolve results — only the direction and a point on the line matter)
- The distance input (<kbd>Tab</kbd>) can still be used in combination with the angle constraint for the visual helper
- **Order when using both**: When requiring both an angle and a length (for the helper), apply the angle constraint first (<kbd>Shift</kbd>+<kbd>Tab</kbd>), then the length (<kbd>Tab</kbd>).
- **Note**: When an angle constraint is active, snapping to nodes is disabled to maintain the angle precision

**Tips:**
- The operation axis is a reference *line* (direction + any point it passes through). Its drawn length is visual only and has no effect on mirror or revolve results.
- Select edges or faces (while the Operation Axis tool is active) before using the Mirror or Revolve buttons in the options panel
- To redefine the axis, first use the **Clear axis** button, then click to place the new axis points
- Use snap points for precise axis placement relative to existing geometry
- Constraining the angle when placing the axis is useful for precise directional reference lines (e.g. for future snapping operations)
- You can make the drawn axis segment long or short via <kbd>Tab</kbd> purely for visibility/clarity — it does not change the behavior of the operations

### Dimension Tool

![Dimension Tool](res/icons/TechDraw_LengthDimension.png)

Edge dimension tool creates/removes **length dimensions between two sketch nodes**.

**Features:**

| | |
| ---: | --- |
| **Node-pair dimensions** | Dimensions are defined by two sketch nodes |
| **Fast edge workflow** | Click a straight edge to toggle a dimension between its two endpoint nodes |
| **Two-node workflow** | Click one node, then a second node, to toggle a dimension between them; after the first node, a **preview line** follows the cursor (same idea as the line-edge rubber band), so two-node mode is visually distinct from a single edge click |
| **Selectable/deletable** | Dimension objects can be selected in sketch mode and deleted (for example with <kbd>Shift</kbd>+<kbd>D</kbd>, <kbd>Delete</kbd>, or <kbd>Backspace</kbd>) |
| **Helpful for verification** | Quickly verify that your sketch has the correct dimensions |

**Shortcuts:**

| | |
| ---: | --- |
| <kbd>D</kbd> | Activate dimension tool |
| <kbd>Shift</kbd>+<kbd>D</kbd>, <kbd>Delete</kbd>, or <kbd>Backspace</kbd> | Delete selected dimension (or other selection) |

**How to Use:**
1. ![TechDraw_LengthDimension](res/icons/TechDraw_LengthDimension.png) Press <kbd>D</kbd> or select the **Edge Dimensions** tool from the toolbar
2. Use either input style:
   - **Edge click:** Click a straight edge to toggle the dimension between its endpoints
   - **Node pair:** Click node A, then click node B to toggle the dimension between A and B
3. Click the same edge (or same node pair) again to remove that dimension
4. To delete directly, select the dimension object and press <kbd>Shift</kbd>+<kbd>D</kbd>, <kbd>Delete</kbd>, or <kbd>Backspace</kbd>

**When to Use:**

| | |
| ---: | --- |
| **Design verification** | Check that your sketch dimensions match your design requirements |
| **Quality control** | Verify measurements before extruding or performing operations |
| **Learning and debugging** | Understand how your sketch geometry is sized |
| **Documentation** | Take screenshots with dimensions visible for reference |
| **Selective display** | Show dimensions only for the node pairs you care about |

**Tips:**
- Dimensions display the distance between the two referenced nodes
- Node hover/pick uses normal sketch snap distance rules
- This tool is particularly useful when working with precise measurements
- Use in combination with the distance input feature (<kbd>Tab</kbd> key) when creating edges
- Dimensions are for display only and do not constrain the geometry
- Toggle dimensions on only the elements you need to reduce visual clutter

**Technical Details:**

| | |
| ---: | --- |
| **Dimension source** | Calculated from the two referenced nodes |
| **Unit system** | Displays measurements in the current unit system |
| **Auto-update** | Dimensions update automatically when geometry is modified |
| **View-only** | Does not affect the underlying geometry |

### Add Node Tool

![Add Node Tool](res/icons/Sketcher_CreatePoint.png)

The **Add node** tool (toolbar: **Add node**) adds sketch **vertices**. What happens on each placement depends on whether you **start from an existing node**:

- **First click snaps to an existing node** — The tool **acts like [Line edge](#line-edge-creation-tools)** for the next step: a **rubber-band** preview runs from that anchor, and you can enter **length** and/or **angle** (<kbd>Tab</kbd> / <kbd>Shift</kbd>+<kbd>Tab</kbd>, same order as line edge) before you place the **second** point. **No new line edge** is added to the sketch; only the new node is created at the end of that placement.
- **First click does not snap to an existing node** — The new node is placed **at the click** on the sketch plane (usual snap rules still apply, e.g. **mid-point snap** onto a **straight** edge to **split** it).

In both cases, Add node never leaves a **permanent edge** between two clicks the way Line edge does.

#### What else it does

| | |
| ---: | --- |
| **Mid-point snap / split** | If a click lands near a **straight** (non-arc) edge (not at its endpoints), the pick snaps onto the segment and that edge is **replaced by two** meeting at the new vertex. Arc edges are **not** split this way. |
| **No stored edge from anchor** | After a two-step placement from an anchor, **no** sketch edge is created between anchor and new node (unlike Line edge). |
| **Angle constraint** | With an angle constraint active during rubber-band placement, the next click places the new node along that ray from the anchor; snapping may be relaxed to stay on that direction (similar to the line tool). |

#### Permanent “+” markers

Nodes you place with **Add node** are treated as **user-placed** points. When the sketch is visible, eligible points can show a small **+** marker in the 3D view so you can see and pick them. Geometry that exists only as automatic **edge midpoints** for snapping is separate (those nodes are not shown the same way). 

#### How to use

1. Enter sketch mode and select **Add node** ![Add Node Tool](res/icons/Sketcher_CreatePoint.png) on the toolbar.
2. **Direct node:** Click where you are **not** snapping to an existing vertex (empty sketch area, or near a straight edge for **mid-point snap**, etc.). The node is created at the snapped position; **mid-point snap** on a **straight** edge splits that edge.
3. **From an existing node (line-edge–like step):** Click an **existing** vertex first so the first click **snaps** to it. A rubber-band preview runs from that anchor—use <kbd>Tab</kbd> / <kbd>Shift</kbd>+<kbd>Tab</kbd> for length/angle if you want—then click (or confirm) to place the **second** point. Only a **new node** is committed; **no** new edge is stored.
4. Repeat as needed for more nodes.

#### Shortcuts

| | |
| ---: | --- |
| <kbd>Tab</kbd> | Distance input for rubber-band placement (when an anchor is active). |
| <kbd>Shift</kbd>+<kbd>Tab</kbd> | Angle input (degrees) for rubber-band placement. |
| <kbd>Escape</kbd> | Cancel the current placement or rubber-band step. |
| <kbd>Right-click</kbd> | Drop an incomplete rubber-band preview if you started from an anchor and have not placed the second point yet. |

#### Tips

- Prefer **Add node** when you need **extra vertices** or **edge splits** without a new visible segment between the two clicks.
- Use [Line edge](#line-edge-creation-tools) or [Multi-line](#multi-line-edge-tool) when you want a **new edge** between two locations.

#### Add node vs. Line edge

| | |
| ---: | --- |
| **Add node** | Rubber band is for **placement** only; **does not** add a permanent edge between the two clicks. |
| **Line edge** | Two clicks (or numeric input) create a **new line edge** in the sketch. |

### Create Sketch from Planar Face Tool

![Create Sketch from Planar Face Tool](res/icons/Macro_FaceToSketch_48.png)

The create sketch from planar face tool allows you to extract the boundary of a planar face from an existing 3D shape and create a new 2D sketch from it. This is useful for reverse engineering, modifying existing shapes, or creating new features based on existing geometry.

**Features:**

| | |
| ---: | --- |
| **Face selection** | Click directly on a planar face from an existing 3D shape |
| **Automatic boundary extraction** | Extracts the outer wire (boundary) of the selected face |
| **New sketch creation** | Creates a new sketch with the face boundary as the initial geometry |
| **Planar face requirement** | Only works with planar (flat) faces - curved or complex surfaces are not supported |
| **Error handling** | Displays an error message if a non-planar face is selected |

**How to Use:**
1. **Activate Tool**: ![Macro_FaceToSketch_48](res/icons/Macro_FaceToSketch_48.png) Click the **Create Sketch from Planar Face** tool from the toolbar
2. **Select Face**: Click on a planar face from an existing 3D shape
   - The face must be planar (flat) - curved surfaces like cylinders, spheres, or complex surfaces will show an error
   - The system will automatically extract the outer boundary of the face
3. **Sketch Created**: A new sketch is automatically created with:
   - The face boundary as the initial wire
   - The sketch plane aligned with the face plane
   - The sketch name set to "Sketch from face"
4. **Edit Sketch**: You can now edit the sketch using standard sketch tools (add edges, modify geometry, etc.)

**Requirements:**
- **Planar face only**: The selected face must be flat (planar). Common planar faces include:
  - Flat surfaces on boxes or rectangular shapes
  - Flat faces on imported STEP/IGES models
  - Top or bottom faces of [extruded](usage.md#extrude-sketch-face-tool-e) shapes
  - Any face that lies in a single plane

**Error Handling:**
- If you select a non-planar face, you will see an error message: "Error: Selected face is not planar. Please select a planar face."
- The operation will be cancelled and no sketch will be created
- Simply select a different face that is planar

**Shortcuts:**
- <kbd>Escape</kbd>: Cancel the operation (if activated but not yet completed)

**Tips:**
- Use this tool to extract profiles from existing 3D models for modification or reference
- The created sketch can be edited, modified, and then extruded to create new features
- Useful for reverse engineering or working with imported CAD models
- The sketch plane will be automatically aligned with the face plane
- After creating the sketch, you can add additional geometry or modify the extracted boundary
- The original shape remains unchanged - this tool only creates a new sketch based on the face boundary

**Common Use Cases:**

| | |
| ---: | --- |
| **Reverse engineering** | Extract profiles from imported 3D models |
| **Feature modification** | Create a sketch from an existing face, modify it, and [extrude](usage.md#extrude-sketch-face-tool-e) to create a new feature |
| **Reference geometry** | Use existing face boundaries as reference for new sketches |
| **Model editing** | Extract a face boundary, modify it, and use it in [boolean operations](usage.md#boolean-operations) |
| **Workflow efficiency** | Quickly create sketches from existing geometry instead of manually recreating profiles |

**Technical Details:**

| | |
| ---: | --- |
| **Boundary extraction** | Uses `BRepTools::OuterWire()` on the selected face |
| **Sketch plane** | Determined from the face's underlying surface geometry |
| **Supported faces** | Only `Geom_Plane` surfaces - other surface types are rejected |
| **Sketch list** | Created sketch is added and can be managed like any other sketch |

### Image underlay

Import a reference image (PNG, JPEG, or BMP) behind a sketch for tracing or alignment. Open **Sketch properties** from the [Sketch List](usage.md#sketch-list) (**`[P]`** on the sketch row) or use the underlay controls there after import.

**Sketch List shortcuts**

| Control | Action |
| --- | --- |
| Underlay checkbox | Show or hide the underlay for that sketch (only when an image is loaded) |
| **`[P]`** | Open **Sketch properties** for import, calibration, and transform |

**Sketch properties — Image underlay**

| Control | Action |
| --- | --- |
| **Import image...** | Load PNG/JPEG/BMP into the current sketch |
| **Remove underlay** | Clear the image from the sketch |
| **White paper -> transparent** | Treat bright pixels as clear (typical for scanned line drawings; turn off for photos) |
| **Tint visible lines** / **Line color** | Recolor non-transparent pixels after the white key (default yellow works on dark backgrounds) |
| **Opacity** | Overall underlay transparency (**0**–**1**) |

**Calibrate from sketch edges** (sketch must be **current** in the Sketch List):

| Button | Action |
| --- | --- |
| **Set X from edge...** | Two clicks along bitmap width (+U), then enter the real drawing distance (same units as sketch dimensions) |
| **Set Y from edge...** | Two clicks along bitmap height (+V), then enter the drawing distance for Y |
| **Define underlay datum...** | Two picks on the sketch plane: corner **(0,0)**, then direction along +U (keeps current half width and height) |

After edge calibration, bitmap axes may be non-orthogonal (shear). The **Transform** sliders (**Center X/Y**, **Half width/height**, **Rotation**) then stay disabled so they do not overwrite that affine fit; use calibration picks or adjust before shear is introduced.

**Transform** (orthogonal underlay only): sliders and **Rotation value** adjust center, half extents, and rotation on the sketch plane; changes apply in real time and support undo.

Default underlay highlight tint for new imports: **Settings -> Sketch -> Underlay highlight color** (see [usage-settings.md](usage-settings.md#settings-pane)).