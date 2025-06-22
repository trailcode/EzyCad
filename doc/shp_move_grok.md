# Shp_move Class Documentation

## Overview
The `Shp_move` class is part of a CAD-like application built with Open CASCADE Technology (OCCT). It handles the interactive movement of selected 3D shapes in a 3D view, supporting features like axis constraints, direct distance input via a GUI, and operations to finalize or cancel the move. The class integrates with an `Occt_view` for rendering and a GUI for user interaction.

## Class Details
- **Header File**: `shp_move.h`
- **Source File**: `shp_move.cpp`
- **Base Class**: `Shp_operation_base` (private inheritance)
- **Namespace**: Not explicitly defined in the provided code
- **Dependencies**: OCCT (`gp_Pln`, `gp_Pnt`, `gp_Trsf`, `gp_Vec`, `gp_XYZ`), `Occt_view`, `GUI`, `geom.h`

### Key Structures
- **`Move_options`**:
  - Fields: `constr_axis_x`, `constr_axis_y`, `constr_axis_z` (boolean flags to constrain movement along X, Y, or Z axes).
  - Purpose: Configures axis-specific movement restrictions.
- **`Deltas`** (private struct):
  - Fields: `override_x`, `override_y`, `override_z` (optional doubles for user-entered values), `delta` (`gp_XYZ` for computed translation).
  - Purpose: Tracks user overrides and the resulting translation vector.

### Private Members
- `m_move_pln`: Optional `gp_Pln` defining the movement plane (based on the view).
- `m_center`: Optional `gp_Pnt` representing the shape’s center (computed from the first shape’s bounding box).
- `m_opts`: `Move_options` instance for configuration.
- `m_delta`: `Deltas` instance for translation data.

## Public Methods
| Method | Description | Return Type |
|--------|-------------|-------------|
| `Shp_move(Occt_view& view)` | Constructor; initializes with a reference to the 3D view. | - |
| `Status move_selected(const ScreenCoords& screen_coords)` | Moves selected shapes based on screen coordinates, respecting axis constraints and user overrides. Updates the display. | `Status` (e.g., `ok`, `user_error`) |
| `void show_dist_edit(const ScreenCoords& screen_coords)` | Enables GUI-based distance input for unconstrained axes, setting up callbacks to override X, Y, or Z deltas. | - |
| `void finalize()` | Completes the move operation for non-empty shape selections and resets the state. | - |
| `void cancel()` | Cancels the operation and resets the state. | - |
| `void reset()` | Clears options, deltas, plane, center, shapes, and sets GUI mode to `Normal`. | - |
| `Move_options& get_opts()` | Returns a reference to the `Move_options` struct for configuration. | `Move_options&` |

### Private Methods
- `void check_finalize_()`: Checks if all required axes have overrides or are unconstrained, calling `finalize()` when conditions are met.

## Functionality
- **Interactive Movement**: Translates shapes based on mouse input, projected onto a view-defined plane (computed from `Occt_view`).
- **Axis Constraints**: Optionally restricts movement to X, Y, or Z axes via `Move_options`.
- **Direct Input**: Supports GUI-based distance entry for precise control, with scaling via `view().get_dimension_scale()`.
- **State Management**: Tracks the movement plane, shape center, and deltas; handles finalization or cancellation.
- **Error Handling**: Returns `Status` to indicate success or issues (e.g., failure to project a point onto the plane).
- **OCCT Integration**: Uses OCCT for transformations (`gp_Trsf`), geometry, and rendering (`ctx().Redisplay`).

## Usage Example
```cpp
#include "shp_move.h"
#include "occt_view.h"

// Initialize view and move operation
Occt_view view; // Assume configured
Shp_move move_op(view);

// Configure options (e.g., constrain to X-axis)
move_op.get_opts().constr_axis_x = true;

// Move shapes based on mouse input
ScreenCoords mouse_pos = {100, 200}; // Example screen coordinates
Status status = move_op.move_selected(mouse_pos);
if (!status.is_ok()) {
    // Handle error (e.g., display message)
}

// Show distance editor for precise input
move_op.show_dist_edit(mouse_pos);

// Finalize or cancel
move_op.finalize(); // Or move_op.cancel();