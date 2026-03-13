# EzyCad 2D Sketching Guide

This guide covers all 2D sketching tools and operations in EzyCad. For the main usage guide (workflow, 3D modeling, file operations, etc.), see [usage.md](usage.md).

## Table of Contents
1. [2D Sketching](#2d-sketching)
2. [Line Edge Creation Tools](#line-edge-creation-tools)
3. [Multi-Line Edge Tool](#multi-line-edge-tool)
4. [Circle Creation Tools](#circle-creation-tools)
5. [Circle Creation Workflow](#circle-creation-workflow)
6. [Arc Segment Creation Tool](#arc-segment-creation-tool)
7. [Rectangle and Square Creation Tools](#rectangle-and-square-creation-tools)
8. [Slot Creation Tool](#slot-creation-tool)
9. [Operation Axis Tool](#operation-axis-tool)
10. [Toggle Edge Dimensions Tool](#toggle-edge-dimensions-tool)
11. [Create Sketch from Planar Face Tool](#create-sketch-from-planar-face-tool)

---

## 2D Sketching
1. **Basic Tools**
   - Add nodes
   - [Create line edges](#line-edge-creation-tools) <img src="icons/Sketcher_Element_Line_Edge.png" alt="Line Edge Tool" width="20" height="20">
   - [Draw multi-line edges](#multi-line-edge-tool) <img src="icons/ls.png" alt="Multi-line Edge Tool" width="20" height="20">
   - [Add arc segments](#arc-segment-creation-tool) <img src="icons/Sketcher_Element_Arc_Edge.png" alt="Arc Segment Tool" width="20" height="20">
   - [Create circles](#circle-creation-tools) <img src="icons/Sketcher_CreateCircle.png" alt="Circle Tool" width="20" height="20">
   - [Draw rectangles and squares](#rectangle-and-square-creation-tools) <img src="icons/Sketcher_CreateRectangle.png" alt="Rectangle Tool" width="20" height="20"> <img src="icons/Sketcher_CreateSquare.png" alt="Square Tool" width="20" height="20">
   - [Add slots](#slot-creation-tool) <img src="icons/Sketcher_CreateSlot.png" alt="Slot Tool" width="20" height="20">

2. **Sketch Operations**
   - [Define operation axis](#operation-axis-tool) <img src="icons/Sketcher_MirrorSketch.png" alt="Operation Axis Tool" width="20" height="20">
      - Mirror sketches
      - Revolve edges or faces
   - [Toggle edge dimensions](#toggle-edge-dimensions-tool) <img src="icons/TechDraw_LengthDimension.png" alt="Toggle Edge Dimensions Tool" width="20" height="20">
   - [Create sketch from planar face](#create-sketch-from-planar-face-tool) <img src="icons/Macro_FaceToSketch_48.png" alt="Create Sketch from Planar Face Tool" width="20" height="20">

#### Line Edge Creation Tools
![Line Edge Tool](icons/Sketcher_Element_Line_Edge.png)

EzyCad provides tools for creating individual line edges in sketch mode, allowing you to build complex geometries one edge at a time.

##### Single Line Edge Tool

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
1. Select the **Line Edge** tool from the toolbar (line icon)
2. Click to set the start point of the line
3. Move the mouse to see a preview of the line
4. Click to set the end point, or use input dialogs for precise control:
   - Press <kbd>Tab</kbd> to enter an exact length value
   - Press <kbd>Shift</kbd>+<kbd>Tab</kbd> to enter an exact angle value (in degrees)
   - When using both: apply angle (<kbd>Shift</kbd>+<kbd>Tab</kbd>) first, then distance (<kbd>Tab</kbd>).
5. The line edge will be created and added to your sketch

**Keyboard shortcuts:**

| | |
| ---: | --- |
| <kbd>Tab</kbd> | Open distance input dialog for precise length control |
| <kbd>Shift</kbd>+<kbd>Tab</kbd> | Open angle input dialog to constrain the line to a specific angle (after first point is set) |
| <kbd>Escape</kbd> | Cancel the current line creation |
| <kbd>Enter</kbd> | Finalize the line (if using distance or angle input) |
| <kbd>Right-click</kbd> | Complete the current line and start a new one |

**Angle Constraint:**
- After setting the first point, press **Shift+Tab** to open the angle input dialog
- Enter the desired angle in degrees (0Â° = horizontal right, 90Â° = vertical up, counterclockwise)
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

#### Multi-Line Edge Tool

![Multi-line Edge Tool](icons/ls.png)

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
1. Select the **Multi-line Edge** tool from the toolbar <img src="icons/ls.png" alt="ls" width="20" height="20">
2. Click to set the first point (start of the first edge)
3. Move the mouse to see a preview of the first edge
4. Click to set the second point (end of first edge, start of second edge), or use input for precise control:
   - Press <kbd>Tab</kbd> to enter an exact length value
   - Press <kbd>Shift</kbd>+<kbd>Tab</kbd> to enter an exact angle (in degrees) for the current edge
   - When using both angle and distance: apply angle (<kbd>Shift</kbd>+<kbd>Tab</kbd>) first, then distance (<kbd>Tab</kbd>).
5. Continue clicking to add more connected edges, each new click starts a new edge from the previous edge's end point
6. Press <kbd>Right-click</kbd> to finalize the entire multi-line sequence and add all edges to your sketch

**Keyboard shortcuts:**

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

#### Circle Creation Tools

![Circle Tool](icons/Sketcher_CreateCircle.png)

EzyCad provides a method for creating circles in sketch mode using the **center-radius approach**.

##### Center-Radius Circle Tool

The center-radius circle tool allows you to create circles by defining a center point and a radius point.

**Features:**

| | |
| ---: | --- |
| **Two-point creation** | Click to set the center, then click to set the radius |
| **Real-time preview** | See the circle shape while dragging the radius point |
| **Precise radius control** | Use the distance input dialog (<kbd>Tab</kbd> key) for exact radius values |
| **Snap support** | Automatically snaps to existing nodes and geometry |

**How to use:**
1. Select the **Circle** tool from the toolbar (circle icon)
2. Click to set the center point of the circle
3. Move the mouse to see a preview of the circle
4. Click to set the radius point, or press <kbd>Tab</kbd> to enter an exact radius value
5. The circle will be created and added to your sketch

**Keyboard shortcuts:**

| | |
| ---: | --- |
| <kbd>Tab</kbd> | Open distance input dialog for precise radius control |
| <kbd>Escape</kbd> | Cancel the current circle creation |
| <kbd>Enter</kbd> | Finalize the circle (if using distance input) |

**Tips:**
- Use the snap feature to create circles that are precisely positioned relative to existing geometry
- The circle tool works in any sketch plane
- Circles can be used as construction geometry or as part of your final design

##### Three-Point Circle Tool (Planned Feature)

**Status**: Not yet implemented

The three-point circle tool is planned for future development. This feature would allow creating circles by defining three points that lie on the circle's circumference.

**Planned Features:**

| | |
| ---: | --- |
| **Three-point creation** | Click three points that lie on the circle's circumference |
| **Automatic center and radius calculation** | The system would compute the center and radius from the three points |
| **Geometric validation** | Ensure the three points are not collinear |

**Note**: The toolbar icon <img src="icons/Sketcher_Create3PointCircle.png" alt="Sketcher_Create3PointCircle" width="20" height="20"> exists but the functionality is not yet implemented.

#### Circle Creation Workflow

The circle tool follows this workflow:

1. **Activate Tool**: Select the circle creation mode
2. **Point Placement**: Click to place the center point, then click to place the radius point
3. **Preview**: See real-time preview of the circle as you move the mouse
4. **Finalization**: Click to complete the circle creation
5. **Integration**: The circle becomes part of the sketch and can be used for further operations

**Common Operations with Circles:**

| | |
| ---: | --- |
| **Extrusion** | Select the circle face and [extrude](usage.md#extrude-sketch-face-tool-e) to create cylindrical shapes |
| **Boolean Operations** | Use circles in [cut, fuse, or common](usage.md#other-feature-operations) operations |
| **Pattern Creation** | Use circles as the basis for polar arrays or other patterns |
| **Dimensioning** | Add radius or diameter dimensions to circles |

**Error Handling:**

| | |
| ---: | --- |
| **Coincident Points** | The system prevents creation of circles with zero radius |
| **Invalid Geometry** | Circles that would be too small are rejected |
| **Snap Integration** | Use existing snap points for precise circle placement |

#### Arc Segment Creation Tool

![Arc Segment Tool](icons/Sketcher_Element_Arc_Edge.png)

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
1. Select the **Arc Segment** tool from the toolbar <img src="icons/Sketcher_Element_Arc_Edge.png" alt="Sketcher_Element_Arc_Edge" width="20" height="20">
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

**Keyboard shortcuts:**

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

#### Rectangle and Square Creation Tools

EzyCad provides three tools for creating rectangular shapes in sketch mode: square, rectangle from two points, and rectangle with center point.

##### Square Tool

![Square Tool](icons/Sketcher_CreateSquare.png)

The square tool allows you to create perfect squares by defining a center point and an edge midpoint.

**Features:**

| | |
| ---: | --- |
| **Two-point creation** | Click to set the center point, then click to set the midpoint of one edge |
| **Real-time preview** | See the square shape while moving the mouse |
| **Perfect square** | Automatically ensures all sides are equal length |
| **Orientation control** | The square's orientation is determined by the direction from center to edge midpoint |
| **Precise size control** | Use the distance input dialog (<kbd>Tab</kbd> key) for exact side lengths |
| **Snap support** | Automatically snaps to existing nodes and geometry |

**How to use:**
1. Select the **Square** tool from the toolbar <img src="icons/Sketcher_CreateSquare.png" alt="Sketcher_CreateSquare" width="20" height="20">
2. Click to set the center point of the square
3. Move the mouse to see a preview of the square
4. Click to set the midpoint of one edge (this defines both the size and orientation), or press <kbd>Tab</kbd> to enter an exact side length value
5. The square will be created with four edges and added to your sketch

**Keyboard shortcuts:**

| | |
| ---: | --- |
| <kbd>Tab</kbd> | Open distance input dialog for precise side length control |
| <kbd>Escape</kbd> | Cancel the current square creation |
| <kbd>Enter</kbd> | Finalize the square (if using distance input) |

**Tips:**
- The distance from center to edge midpoint determines half the side length
- Use the snap feature to create squares that are precisely positioned relative to existing geometry
- The square tool works in any sketch plane
- Squares automatically form closed faces that can be [extruded](usage.md#extrude-sketch-face-tool-e)

##### Rectangle Tool (Two Points)

![Rectangle Tool](icons/Sketcher_CreateRectangle.png)

The rectangle tool allows you to create rectangles by defining two opposite corners.

**Features:**

| | |
| ---: | --- |
| **Two-point creation** | Click to set the first corner, then click to set the opposite corner |
| **Real-time preview** | See the rectangle shape while moving the mouse |
| **Precise size control** | Use the distance input dialog (<kbd>Tab</kbd> key) for exact dimensions |
| **Snap support** | Automatically snaps to existing nodes and geometry |
| **Automatic corner calculation** | The system automatically calculates the other two corners |

**How to use:**
1. Select the **Rectangle** tool from the toolbar <img src="icons/Sketcher_CreateRectangle.png" alt="Sketcher_CreateRectangle" width="20" height="20">
2. Click to set the first corner point
3. Move the mouse to see a preview of the rectangle
4. Click to set the opposite corner point, or press <kbd>Tab</kbd> to enter exact distance values
5. The rectangle will be created with four edges and added to your sketch

**Keyboard shortcuts:**

| | |
| ---: | --- |
| <kbd>Tab</kbd> | Open distance input dialog for precise dimension control |
| <kbd>Escape</kbd> | Cancel the current rectangle creation |
| <kbd>Enter</kbd> | Finalize the rectangle (if using distance input) |

**Tips:**
- The two points define opposite corners of the rectangle (diagonal)
- The rectangle edges are automatically aligned with the coordinate axes
- Use the snap feature to create rectangles that are precisely positioned relative to existing geometry
- The rectangle tool works in any sketch plane
- Rectangles automatically form closed faces that can be [extruded](usage.md#extrude-sketch-face-tool-e)

##### Rectangle Tool (Center Point)

![Rectangle Center Tool](icons/Sketcher_CreateRectangle_Center.png)

The rectangle with center point tool allows you to create rectangles by defining a center point and a corner point.

**Features:**

| | |
| ---: | --- |
| **Two-point creation** | Click to set the center point, then click to set a corner point |
| **Real-time preview** | See the rectangle shape while moving the mouse |
| **Centered creation** | The rectangle is centered on the first point |
| **Precise size control** | Use the distance input dialog (<kbd>Tab</kbd> key) for exact dimensions |
| **Snap support** | Automatically snaps to existing nodes and geometry |

**How to use:**
1. Select the **Rectangle with Center Point** tool from the toolbar <img src="icons/Sketcher_CreateRectangle_Center.png" alt="Sketcher_CreateRectangle_Center" width="20" height="20">
2. Click to set the center point of the rectangle
3. Move the mouse to see a preview of the rectangle
4. Click to set a corner point (defines both size and orientation), or press <kbd>Tab</kbd> to enter exact distance values
5. The rectangle will be created with four edges and added to your sketch

**Keyboard shortcuts:**

| | |
| ---: | --- |
| <kbd>Tab</kbd> | Open distance input dialog for precise dimension control |
| <kbd>Escape</kbd> | Cancel the current rectangle creation |
| <kbd>Enter</kbd> | Finalize the rectangle (if using distance input) |

**Tips:**
- The rectangle is centered on the first point you click
- The distance from center to corner determines the rectangle's dimensions
- Use the snap feature to create rectangles that are precisely positioned relative to existing geometry
- The rectangle tool works in any sketch plane
- Rectangles automatically form closed faces that can be [extruded](usage.md#extrude-sketch-face-tool-e)

**Comparison of Rectangle Tools:**

| | |
| ---: | --- |
| **Rectangle (Two Points)** | Define opposite corners - useful when you know the corner positions |
| **Rectangle (Center Point)** | Define center and corner - useful when you want the rectangle centered on a specific point |
| **Square** | Always creates a perfect square - use when you need equal sides |

#### Slot Creation Tool

![Slot Tool](icons/Sketcher_CreateSlot.png)

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
| **Snap support** | Automatically snaps to existing nodes and geometry |

**How to use:**
1. Select the **Slot** tool from the toolbar <img src="icons/Sketcher_CreateSlot.png" alt="Sketcher_CreateSlot" width="20" height="20">
2. Click to set the center point of the first arc (start of slot)
3. Move the mouse to see a preview of the first edge
4. Click to set the center point of the second arc (end of slot)
5. Move the mouse to see a preview of the slot shape
6. Click to set a point that defines the radius of the arcs, or press <kbd>Tab</kbd> to enter exact distance values
   - **Important**: The radius is measured from the second arc center (the point you clicked in step 4) to this third point
   - This radius determines the cross section: the slot's cross-section dimension equals 2 Ã— radius
7. The slot will be automatically created with two arcs and two straight edges and added to your sketch

**Point order:**
- **First click**: First arc center - the center of the arc at one end of the slot
- **Second click**: Second arc center - the center of the arc at the other end of the slot
- **Third click**: Radius point - defines the radius of both arcs
  - The distance from the **second arc center** (second click) to this point determines the arc radius
  - The slot's cross section (perpendicular to the slot length) equals twice this radius
  - Both arcs use the same radius, creating a symmetric slot

**Keyboard shortcuts:**

| | |
| ---: | --- |
| <kbd>Tab</kbd> | Open distance input dialog for precise dimension control |
| <kbd>Escape</kbd> | Cancel the current slot creation |
| <kbd>Enter</kbd> | Finalize the slot (if using distance input) |
| **Note** | The slot is automatically finalized after the third point, so no manual finalization is needed |

**Tips:**
- The slot length is determined by the distance between the first and second arc centers
- The radius of both arcs is determined by the distance from the second arc center to the radius point (third click)
- The slot's cross section (the dimension perpendicular to the slot length) equals 2 Ã— radius
  - This is because each arc is a semicircle with the specified radius, extending equally in both perpendicular directions
  - For example, if you click the radius point 3 units away from the second arc center, the slot cross section will be 6 units
- Both arcs have the same radius, creating a symmetric slot shape
- The slot orientation (which dimension is length vs width/height) is determined by the direction from the first to the second arc center
- Use the snap feature to create slots that connect precisely to existing geometry
- The slot tool works in any sketch plane
- Slots automatically form closed faces that can be [extruded](usage.md#extrude-sketch-face-tool-e)
- The slot orientation is determined by the direction from the first to the second arc center

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

#### Operation Axis Tool

![Operation Axis Tool](icons/Sketcher_MirrorSketch.png)

The operation axis tool allows you to define a reference line for mirroring and revolving operations in sketches.

**Features:**

| | |
| ---: | --- |
| **Two-point definition** | Click to set the start point, then click to set the end point of the axis line |
| **Real-time preview** | See the axis line while moving the mouse |
| **Automatic redefinition** | If an axis already exists, clicking again will clear it and start defining a new one |
| **Mirror operations** | Use the defined axis to mirror selected edges |
| **Revolve operations** | Use the defined axis to revolve selected edges or faces |

**How to Use:**
1. Select the **Operation Axis** tool from the toolbar <img src="icons/Sketcher_MirrorSketch.png" alt="Sketcher_MirrorSketch" width="20" height="20">
2. Click to set the start point of the axis line
3. Move the mouse to see a preview of the axis line
4. Click to set the end point to finalize the axis
5. Once defined, the axis can be used for mirror or revolve operations

**Redefining the Axis:**
- If an operation axis already exists and you click again in operation axis mode, the existing axis will be automatically cleared and you can start defining a new one
- Alternatively, use the "Clear axis" button in the options panel to manually clear the axis

**Using the Operation Axis:**
Once an axis is defined, the options panel will show:

| | |
| ---: | --- |
| **Mirror button** | Mirrors selected edges across the operation axis |
| **Revolve button** | Revolves selected edges or faces around the operation axis |
| **Revolve angle input** | Set the angle for revolve operations (default: 360 degrees) |
| **Clear axis button** | Manually clear the current operation axis |

**Keyboard Shortcuts:**

| | |
| ---: | --- |
| <kbd>Escape</kbd> | Cancel the current axis definition |

**Tips:**
- The operation axis is a reference line used for geometric transformations
- Select edges or faces before using the Mirror or Revolve operations
- The axis can be redefined at any time by clicking again in operation axis mode
- Use snap points for precise axis placement relative to existing geometry

#### Toggle Edge Dimensions Tool

![Toggle Edge Dimensions Tool](icons/TechDraw_LengthDimension.png)

The toggle edge dimensions tool allows you to show or hide length dimension annotations on individual sketch edges, making it easier to verify measurements and understand the geometry of your sketches.

**Features:**

| | |
| ---: | --- |
| **Visual dimension display** | Shows length measurements on sketch edges |
| **Per-edge toggle** | Click on individual edges to show or hide their dimensions |
| **Helpful for verification** | Quickly verify that your sketch has the correct dimensions |

**How to Use:**
1. Select the **Toggle Edge Dimensions** tool from the toolbar <img src="icons/TechDraw_LengthDimension.png" alt="TechDraw_LengthDimension" width="20" height="20">
2. Click on any edge in the current sketch to toggle its dimension annotation on or off
3. Repeat for any other edges you want to show or hide dimensions for
4. Click on an edge again to hide its dimension if it's currently visible

**When to Use:**

| | |
| ---: | --- |
| **Design verification** | Check that your sketch dimensions match your design requirements |
| **Quality control** | Verify measurements before extruding or performing operations |
| **Learning and debugging** | Understand how your sketch geometry is sized |
| **Documentation** | Take screenshots with dimensions visible for reference |
| **Selective display** | Show dimensions only for the edges you're interested in |

**Tips:**
- Each edge's dimension can be toggled independently
- The dimensions show the actual length of each edge
- This tool is particularly useful when working with precise measurements
- Use in combination with the distance input feature (<kbd>Tab</kbd> key) when creating edges
- Dimensions are for display only and do not constrain the geometry
- Toggle dimensions on only the edges you need to reduce visual clutter

**Technical Details:**

| | |
| ---: | --- |
| **Dimension source** | Calculated from the actual edge geometry |
| **Unit system** | Displays measurements in the current unit system |
| **Auto-update** | Dimensions update automatically when geometry is modified |
| **View-only** | Does not affect the underlying geometry |

#### Create Sketch from Planar Face Tool

![Create Sketch from Planar Face Tool](icons/Macro_FaceToSketch_48.png)

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
1. **Activate Tool**: Click the **Create Sketch from Planar Face** tool from the toolbar <img src="icons/Macro_FaceToSketch_48.png" alt="Macro_FaceToSketch_48" width="20" height="20">
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

**Keyboard Shortcuts:**
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
| **Model editing** | Extract a face boundary, modify it, and use it in [boolean operations](usage.md#other-feature-operations) |
| **Workflow efficiency** | Quickly create sketches from existing geometry instead of manually recreating profiles |

**Technical Details:**

| | |
| ---: | --- |
| **Boundary extraction** | Uses `BRepTools::OuterWire()` on the selected face |
| **Sketch plane** | Determined from the face's underlying surface geometry |
| **Supported faces** | Only `Geom_Plane` surfaces - other surface types are rejected |
| **Sketch list** | Created sketch is added and can be managed like any other sketch |