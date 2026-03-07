#pragma once

class TopoDS_Shape;

namespace shp_create
{
// Unit cube (side length in display units), centered at origin.
TopoDS_Shape create_cube(double side);

// Unit pyramid: base side and height = side (display units), centered at origin.
TopoDS_Shape create_pyramid(double side);
}  // namespace shp_create
