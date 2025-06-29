# EzyCad Usage Guide

## Table of Contents
1.  [Introduction](#introduction)
2.  [Getting Started](#getting-started)
3.  [User Interface](#user-interface)
4.  [File Operations](#file-operations)
5.  [Modeling Tools](#modeling-tools)
6.  [Keyboard Shortcuts](#keyboard-shortcuts)
7.  [View Controls](#view-controls)
8.  [Tips and Tricks](#tips-and-tricks)
9.  [Support](#support)
10. [Tool Icons](#tool-icons)

## Introduction

EzyCad (Easy CAD) is a CAD application for hobbyist machinists to design and edit 2D and 3D models for machining projects. It supports creating precise parts with tools for sketching, extruding, and applying geometric operations, using OpenGL, ImGui, and Open CASCADE Technology (OCCT). Export models to formats like STEP or STL for CNC machines or 3D printers, and try it in your browser with the WebAssembly version.

## Getting Started

### System Requirements
- Windows or, WebAssembly
   - Not Tested: Linux, or macOS operating system
- OpenGL-compatible graphics card

### Installation
1. Download the latest release for your operating system(TODO)
2. Extract the archive to your preferred location
3. Run the executable file

## User Interface

### Main Components
1. **Menu Bar**
   - File operations (New, Open, Save, Import)
   - Edit functions
   - View controls
   - Help and settings

2. **Toolbar**
   - Quick access to commonly used tools
   - Mode selection buttons
   - Operation tools

3. **Sketch List**
   - View and manage 2D sketches
   - Select and edit sketch elements
   - Toggle sketch visibility

4. **Shape List**
   - View and manage 3D shapes
   - Select and edit shape properties
   - Toggle shape visibility

5. **Options Panel**
   - Adjust tool parameters
   - Set operation properties
   - Configure view settings

6. **Log Window**
   - View operation history
   - Check for errors and warnings
   - Monitor system status

## File Operations

### Supported Formats
- Native format: `.ezy` files
- Import formats: STEP, IGES, STL
- Export formats: STEP, IGES, STL

### Basic Operations
1. **New Project**
   - Start with a clean workspace
   - Reset all views and settings

2. **Open Project**
   - Load existing `.ezy` files
   - Restore previous work

3. **Save Project**
   - Save current work to `.ezy` file
   - Auto-save feature available

4. **Import/Export**
   - Import external CAD files
   - Export to standard formats

## Modeling Tools

### 2D Sketching
1. **Basic Tools**
   - Add nodes
   - [Create line edges](#line-edge-creation-tools) ![Line Edge Tool](icons/Sketcher_Element_Line_Edge.png)
   - Draw multi-line edges
   - Add arc segments
   - [Create circles](#circle-creation-tools) ![Circle Tool](icons/Sketcher_CreateCircle.png)
   - Draw rectangles and squares
   - Add slots

2. **Sketch Operations**
   - Define operation axis
   - Toggle edge dimensions
   - Mirror sketches
   - Create from face

#### Line Edge Creation Tools
![Line Edge Tool](icons/Sketcher_Element_Line_Edge.png)
EzyCad provides tools for creating individual line edges in sketch mode, allowing you to build complex geometries one edge at a time.

##### Single Line Edge Tool

The single line edge tool allows you to create straight line segments between two points.

**Features:**
- **Two-point creation**: Click to set the start point, then click to set the end point
- **Real-time preview**: See the line shape while moving the mouse
- **Precise length control**: Use the distance input dialog (Tab key) for exact line lengths
- **Snap support**: Automatically snaps to existing nodes and geometry
- **Dimension annotations**: Optional length dimensions can be displayed

**How to use:**
1. Select the **Line Edge** tool from the toolbar (line icon)
2. Click to set the start point of the line
3. Move the mouse to see a preview of the line
4. Click to set the end point, or press **Tab** to enter an exact length value
5. The line edge will be created and added to your sketch

**Keyboard shortcuts:**
- **Tab**: Open distance input dialog for precise length control
- **Escape**: Cancel the current line creation
- **Enter**: Finalize the line (if using distance input)
- **Right-click**: Complete the current line and start a new one

**Tips:**
- Use the snap feature to create lines that connect precisely to existing geometry
- Lines can be used as construction geometry or as part of your final design
- The line tool works in any sketch plane
- Multiple line edges can be created in sequence by right-clicking after each line

#### Circle Creation Tools

EzyCad provides a method for creating circles in sketch mode using the **center-radius approach**.

![Circle Tool](icons/Sketcher_CreateCircle.png)

##### Center-Radius Circle Tool

The center-radius circle tool allows you to create circles by defining a center point and a radius point.

**Features:**
- **Two-point creation**: Click to set the center, then click to set the radius
- **Real-time preview**: See the circle shape while dragging the radius point
- **Precise radius control**: Use the distance input dialog (Tab key) for exact radius values
- **Snap support**: Automatically snaps to existing nodes and geometry

**How to use:**
1. Select the **Circle** tool from the toolbar (circle icon)
2. Click to set the center point of the circle
3. Move the mouse to see a preview of the circle
4. Click to set the radius point, or press **Tab** to enter an exact radius value
5. The circle will be created and added to your sketch

**Keyboard shortcuts:**
- **Tab**: Open distance input dialog for precise radius control
- **Escape**: Cancel the current circle creation
- **Enter**: Finalize the circle (if using distance input)

**Tips:**
- Use the snap feature to create circles that are precisely positioned relative to existing geometry
- The circle tool works in any sketch plane
- Circles can be used as construction geometry or as part of your final design

##### Three-Point Circle Tool (Planned Feature)

**Status**: Not yet implemented

The three-point circle tool is planned for future development. This feature would allow creating circles by defining three points that lie on the circle's circumference.

**Planned Features:**
- **Three-point creation**: Click three points that lie on the circle's circumference
- **Automatic center and radius calculation**: The system would compute the center and radius from the three points
- **Geometric validation**: Ensure the three points are not collinear

**Note**: The toolbar icon ![Sketcher_Create3PointCircle](icons/Sketcher_Create3PointCircle.png) exists but the functionality is not yet implemented.

#### Circle Creation Workflow

The circle tool follows this workflow:

1. **Activate Tool**: Select the circle creation mode
2. **Point Placement**: Click to place the center point, then click to place the radius point
3. **Preview**: See real-time preview of the circle as you move the mouse
4. **Finalization**: Click to complete the circle creation
5. **Integration**: The circle becomes part of the sketch and can be used for further operations

**Common Operations with Circles:**
- **Extrusion**: Select the circle face and extrude to create cylindrical shapes
- **Boolean Operations**: Use circles in cut, fuse, or common operations
- **Pattern Creation**: Use circles as the basis for polar arrays or other patterns
- **Dimensioning**: Add radius or diameter dimensions to circles

**Error Handling:**
- **Coincident Points**: The system prevents creation of circles with zero radius
- **Invalid Geometry**: Circles that would be too small are rejected
- **Snap Integration**: Use existing snap points for precise circle placement

### 3D Modeling
1. **Transform Operations**
   - Move shapes (G)
   - Rotate objects (R)
   - Scale elements
   - Polar duplicate

#### Shape Move Tool (G)

The shape move tool allows you to reposition selected shapes in the 3D viewer with precision and flexibility.

**Features:**
- **Axis Constraints:** Restrict movement to the X, Y, or Z axis by toggling axis constraints in the options panel or using keyboard shortcuts.
- **Interactive Distance Editing:** Enter or adjust the distance moved along each axis for precise control. Real-time feedback is provided in the viewer and options panel.
- **Improved Plane Handling:** The move plane is automatically estimated based on the center of the selected shapes, making movement more intuitive.
- **Finalization Logic:** The move operation completes when you confirm the action (e.g., `left mouse button`).
- **Reset and Cancel:** Press `Esc` to cancel and revert to the original position at any time during the move operation.

**How to Use:**
1. **Activate Move Tool:** Select one or more shapes and press `G` or click the ![Assembly_AxialMove](icons/Assembly_AxialMove.png) icon.
2. **Constrain Movement (Optional):** Use the options panel to lock movement to a specific axis, or use keyboard shortcuts (e.g., `X`, `Y`, `Z`).

   ![Move constrain axis example](doc/gen/move_constrain_axis.png)
   
   *Example: Movement constrained on the Y and Z axes.*
3. **Edit Distance (Optional):**  
While moving a shape, you can press `Tab` to activate a floating distance input box for the current axis. If no axis constraints are set, you can edit distances for X, Y, and Z in sequence. If axis constraints are enabled, only the allowed axes are available for editing. After entering a distance, that axis is locked to the specified value. Pressing `Tab` again advances to the next available axis. After the distances for all participating axises are defined, the more will be finalized.

4. **Finalize or Cancel:** Press the `left mouse button` to confirm and apply the move, or `Esc` to cancel and revert.

**Tips:**
- Use axis constraints for straight-line moves.
- Use interactive distance editing for precise adjustments.
- You can always cancel and try again if the move isn't as expected.

#### Shape Rotate Tool (R)

The shape rotate tool enables precise rotation of selected shapes around a specified axis in the 3D viewer.

**Features:**
- **Rotation Axis Options:** Choose between view-to-object rotation or constrain rotation to X, Y, or Z axis.
- **Interactive Angle Editing:** Enter or adjust the rotation angle for precise control with real-time preview.
- **Visual Feedback:** The rotation axis is displayed with color-coded indicators (Red for X, Green for Y, Blue for Z).

**How to Use:**
1. **Activate Rotate Tool:** Select one or more shapes and press `R` or click the ![Draft_Rotate](icons/Draft_Rotate.png) icon. You can also activate the tool and select the shape(s) to rotate afterwards.
2. **Select Rotation Axis: (Optional)**
   
   ![Rotate constrain axis example](doc/gen/rotate_constrain_axis.png)

   *Example: Rotation around on the X axis.*
   - Press `X` to rotate around the X-axis (Red)
   - Press `Y` to rotate around the Y-axis (Green)
   - Press `Z` to rotate around the Z-axis (Blue)
   - Press the same axis key again to switch to view-to-object rotation

3. **Edit Angle (Optional):**
   - Press `Tab` to activate the angle input box
   - Enter the desired rotation angle in degrees
   - The preview updates in real-time as you adjust the angle
   - Pressing enter finializes the rotation

4. **Finalize or Cancel:**
   - Press the `left mouse button` to confirm and apply the rotation
   - Press `Esc` to cancel and revert to the original position

**Tips:**
- Use view-to-object rotation for intuitive free-form rotation
- Use axis constraints for precise rotations around specific axes
- The rotation center point is displayed as a red dot for reference
   - Visiable in wirefame rendering of the shape(s)
- You can combine rotation with other operations for complex transformations

## Feature Operations

- Extrude sketches (E)
- Create chamfers
- Add fillets
- Boolean operations:
  - Cut
  - Fuse
  - Common

## Keyboard Shortcuts

### General Operations
- `Ctrl+O`: Open file
- `Ctrl+S`: Save file
- `Ctrl+Shift+S`: Save as
- `Esc`: Cancel current operation
- `Enter`: Confirm current operation
- `Tab`: Dimension input
- `Delete`: Remove selected elements

### Modeling Shortcuts
- `G`: Move mode
- `E`: Extrude mode
- `D`: Delete selected
- `L`: (Reserved for future use)

## View Controls

### Mouse Controls
- **Left Click**: Select object
- **Left drag**: Orbit view
- **Middle drag**: Pan view
- **Right drag**: Zoom
- **Scroll Wheel**: Zoom in/out

### View Options
- Reset view
- Fit to screen
- Toggle wireframe
- Change material appearance
- Adjust lighting

## Tips and Tricks

### Efficient Modeling
1. Use keyboard shortcuts for common operations
2. Utilize the toolbar for quick access to tools
3. Take advantage of the dimension input feature
4. Use the log window to track operations

### Troubleshooting
1. Check the log window for error messages
2. Verify file permissions for save operations
3. Ensure sufficient system resources
4. Update graphics drivers if experiencing display issues

### Performance Optimization
1. Close unused sketches and shapes
2. Use wireframe mode for complex models
3. Optimize view settings for your hardware
4. Regular save operations to prevent data loss

## Support

### Documentation
- This usage guide
- Online documentation
- Video tutorials

### Community
- User forums
- GitHub repository
- Issue tracking

### Updates
- Regular feature updates
- Bug fixes
- Performance improvements

## Tool Icons

### Basic Operations
- ![User](icons/User.png) - Inspection mode
- ![Assembly_AxialMove](icons/Assembly_AxialMove.png) - Shape move (G)
- ![Draft_Rotate](icons/Draft_Rotate.png) - Shape rotate
- ![Part_Scale](icons/Part_Scale.png) - Shape scale

### Sketch Tools
- ![Workbench_Sketcher_none](icons/Workbench_Sketcher_none.png) - Sketch inspection mode
- ![Macro_FaceToSketch_48](icons/Macro_FaceToSketch_48.png) - Create sketch from face
- ![Sketcher_MirrorSketch](icons/Sketcher_MirrorSketch.png) - Define operation axis
- ![Sketcher_CreatePoint](icons/Sketcher_CreatePoint.png) - Add node
- ![Sketcher_Element_Line_Edge](icons/Sketcher_Element_Line_Edge.png) - Add line edge
- ![ls](icons/ls.png) - Add multi-line edge
- ![Sketcher_Element_Arc_Edge](icons/Sketcher_Element_Arc_Edge.png) - Add arc circle
- ![Sketcher_CreateSquare](icons/Sketcher_CreateSquare.png) - Add square
- ![Sketcher_CreateRectangle](icons/Sketcher_CreateRectangle.png) - Add rectangle from two points
- ![Sketcher_CreateRectangle_Center](icons/Sketcher_CreateRectangle_Center.png) - Add rectangle with center point
- ![Sketcher_CreateCircle](icons/Sketcher_CreateCircle.png) - Add circle (center and radius)
- ![Sketcher_Create3PointCircle](icons/Sketcher_Create3PointCircle.png) - Add circle from three points *(planned feature)*
- ![Sketcher_CreateSlot](icons/Sketcher_CreateSlot.png) - Add slot
- ![TechDraw_LengthDimension](icons/TechDraw_LengthDimension.png) - Toggle edge dimension annotation

### 3D Operations
- ![Design456_Extrude](icons/Design456_Extrude.png) - Extrude sketch face (E)
- ![PartDesign_Chamfer](icons/PartDesign_Chamfer.png) - Chamfer
- ![PartDesign_Fillet](icons/PartDesign_Fillet.png) - Fillet
- ![Draft_PolarArray](icons/Draft_PolarArray.png) - Shape polar duplicate

### Boolean Operations
- ![Part_Cut](icons/Part_Cut.png) - Shape cut
- ![Part_Fuse](icons/Part_Fuse.png) - Shape fuse
- ![Part_Common](icons/Part_Common.png) - Shape common

---

For more information, visit the official EzyCad website or GitHub repository.
