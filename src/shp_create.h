#pragma once

class TopoDS_Shape;

namespace shp_create
{
// Unit cube (side length in display units), centered at origin.
TopoDS_Shape create_cube(double side);

// Unit pyramid: base side and height = side (display units), centered at origin.
TopoDS_Shape create_pyramid(double side);

// Sphere with given radius (display units), centered at origin.
TopoDS_Shape create_sphere(double radius);

// Cylinder: radius and height (display units), Z-axis, centered at origin.
TopoDS_Shape create_cylinder(double radius, double height);

// Cone: base radius R1, top radius R2, height (display units), Z-axis, centered at origin.
TopoDS_Shape create_cone(double R1, double R2, double height);

// Torus: major radius R1, minor radius R2 (display units), centered at origin.
TopoDS_Shape create_torus(double R1, double R2);
}  // namespace shp_create
