# Shp_rotate Class Documentation

## Overview
The `Shp_rotate` class is part of a CAD-like application built with Open CASCADE Technology (OCCT). It enables interactive rotation of selected 3D shapes in a 3D view, supporting rotation around a view-defined axis or global X, Y, or Z axes. The class provides features like angle input via a GUI, visual feedback for the rotation axis and center, and operations to finalize or cancel the rotation. It integrates with an `Occt_view` for rendering and a GUI for user interaction.

## Class Details
- **Header File**: `shp_rotate.h`
- **Source File**: `shp_rotate.cpp`
- **Base Class**: `Shp_operation_base` (private inheritance)
- **Namespace**: Not explicitly defined in the provided code
- **Dependencies**: OCCT (`gp_Pln`, `gp_Pnt`, `gp_Vec`, `gp_Dir`, `gp_Trsf`, `gp_Ax1`, `BRepBuilderAPI_MakeEdge`, `BRepBuilderAPI_MakeVertex`, `TopoDS_Edge`, `TopoDS_Vertex`, `AIS_Shape`), `Occt_view`, `GUI`, `geom.h`, `imgui.h`

### Key Structures
- **`Rotation_axis`** (enum class):
  - Values: `View_to_object` (rotate around view axis through object center), `X_axis`, `Y_axis`, `Z_axis` (rotate around global axes).
  - Purpose: Defines the rotation axis for the operation.

### Private Members
- `m_rotate_pln`: Optional `gp_Pln` defining the plane for mouse projection (based on the view).
- `m_initial_mouse_pos`: Optional `gp_Pnt` storing the initial mouse position for angle calculation.
- `m_center`: Optional `gp_Pnt` representing the shape’s center (computed from the first shape’s bounding box).
- `m_angle`: Double storing the current rotation angle (in radians).
- `m_rotation_axis`: `Rotation_axis` enum specifying the current rotation axis (default: `View_to_object`).
- `m_rotation_axis_vis`: `AIS_Shape_ptr` for visualizing the rotation axis as a colored line (red for X, green for Y, blue for Z).
- `m_rotation_center_vis`: `AIS_Shape_ptr` for visualizing the rotation center as a red point.

## Public Methods
| Method | Description | Return Type |
|--------|-------------|-------------|
| `Shp_rotate(Occt_view& view)` | Constructor; initializes with a reference to the 3D view. | - |
| `Status rotate_selected(const ScreenCoords& screen_coords)` | Rotates selected shapes based on screen coordinates, computing the angle from mouse movement. Updates the display. | `Status` (e.g., `ok`, `user_error`) |
| `Status show_angle_edit(const ScreenCoords& screen_coords)` | Enables GUI-based angle input, setting up a callback to update the rotation angle. | `Status` |
| `void finalize()` | Completes the rotation operation for non-empty shape selections and resets the state. | - |
| `void cancel()` | Cancels the operation and resets the state. | - |
| `void set_rotation_axis(Rotation_axis axis)` | Sets the rotation axis and updates its visualization. | - |
| `Rotation_axis get_rotation_axis() const` | Returns the current rotation axis. | `Rotation_axis` |

### Private Methods
| Method | Description |
|--------|-------------|
| `Status ensure_start_state_()` | Ensures shapes, center, and plane are initialized; updates axis and center visualizations. |
| `void preview_rotate_()` | Applies the current rotation to shapes for preview, using the specified axis and angle. |
| `void reset()` | Clears state, removes visualizations, and sets GUI mode to `Normal`. |
| `void update_rotation_axis_()` | Updates or creates a visual representation of the rotation axis (line) based on the current axis. |
| `void update_rotation_center_()` | Updates or creates a visual representation of the rotation center (point). |

## Functionality
- **Interactive Rotation**: Rotates shapes based on mouse movement, calculating the angle between initial and current mouse positions projected onto a plane.
- **Axis Selection**: Supports rotation around the view axis (`View_to_object`) or global X, Y, Z axes, configurable via `set_rotation_axis`.
- **Visual Feedback**: Displays the rotation axis as a colored line (red for X, green for Y, blue for Z) and the center as a red point (though center visibility may require wireframe rendering).
- **Direct Input**: Allows GUI-based angle entry (in degrees, converted to radians) for precise control.
- **State Management**: Tracks the rotation plane, initial mouse position, center, angle, and visualizations; handles finalization or cancellation.
- **Error Handling**: Returns `Status` to indicate success or issues (e.g., mouse too close to center, invalid plane projection).
- **OCCT Integration**: Uses OCCT for geometry (`gp_Vec`, `gp_Trsf`), shape creation (`BRepBuilderAPI`), and rendering (`ctx().Display`, `ctx().Redisplay`).

## Usage Example
```cpp
#include "shp_rotate.h"
#include "occt_view.h"

// Initialize view and rotate operation
Occt_view view; // Assume configured
Shp_rotate rotate_op(view);

// Set rotation axis (e.g., X-axis)
rotate_op.set_rotation_axis(Rotation_axis::X_axis);

// Rotate shapes based on mouse input
ScreenCoords mouse_pos = {150, 250}; // Example screen coordinates
Status status = rotate_op.rotate_selected(mouse_pos);
if (!status.is_ok()) {
    // Handle error (e.g., display message)
}

// Show angle editor for precise input
rotate_op.show_angle_edit(mouse_pos);

// Finalize or cancel
rotate_op.finalize(); // Or rotate_op.cancel();