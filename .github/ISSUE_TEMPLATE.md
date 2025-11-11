# Auto-rotate view when sketch and view planes are parallel during extrude

## Problem
When extruding a sketch face, if the view plane is parallel (or nearly parallel) to the sketch plane, users cannot clearly see the extrusion direction, making it difficult to specify the extrusion distance by clicking and dragging.

## Solution
Automatically rotate the view by 45 degrees when the sketch plane and view plane are detected to be parallel (within 5 degrees tolerance).

## Changes

### Auto-rotation for parallel planes
**File**: `src/occt_view.cpp`  
**Function**: `sketch_face_extrude()` (lines 693-709)

Added logic to detect parallel planes and auto-rotate the view:

```cpp
const gp_Ax1& a = m_to_extrude_pln.Axis();
const gp_Ax1& b = m_curr_view_pln.Axis();

if (a.IsParallel(b, to_radians(5.0)))
{
  // Rotate view by 45 degrees
  const double angle45 = 3.14159265358979323846 / 4.0;
  auto aa = gp_Vec(m_to_extrude_pln.XAxis().Direction()) + gp_Vec(m_to_extrude_pln.YAxis().Direction());
  aa.Normalize();
  aa *= angle45;
  m_view->Rotate(aa.X(), aa.Y(), aa.Z(), m_to_extrude_pt->X(), m_to_extrude_pt->Y(), m_to_extrude_pt->Z());
  m_view->Redraw();
  m_ctx->UpdateCurrentViewer();
  m_to_extrude_pt = std::nullopt;
}
```

### Simplified view plane calculation
**File**: `src/occt_view.cpp`  
**Function**: `get_view_plane()` (lines 371-375)

Refactored to use `camera->Direction()` directly:

```cpp
gp_Pln Occt_view::get_view_plane(const gp_Pnt& point_on_plane) const
{
  Graphic3d_Camera_ptr camera = m_view->Camera();
  return gp_Pln(point_on_plane, camera->Direction());
}
```

## Notes
- Tolerance set to 5 degrees for detecting parallel planes
- Rotation axis calculated from sketch plane's X and Y axes
- After rotation, `m_to_extrude_pt` is reset to trigger recalculation

## Cleanup Needed
There's leftover debug code on line 708 that should be removed:
```cpp
int hi = 0;  // TODO: Remove this debug code
```

## Related
- Related to: "Extrude should be in projection mode" (bug #5 in bugs.md)

