#pragma once

#include <AIS_Shape.hxx>
#include <gp_Pnt.hxx>
#include <string>
#include <vector>

class Sketch;
class TopoDS_Shape;

/// Sketch edge wireframe annotation in the 3D viewer.
struct Sketch_AIS_edge : public AIS_Shape
{
  Sketch_AIS_edge(Sketch& owner, const TopoDS_Shape& shp);
  Sketch& owner_sketch;
};

/// Selectable "+" marker for user-placed permanent sketch nodes (add-node tool).
struct Sketch_AIS_node_mark : public AIS_Shape
{
  Sketch_AIS_node_mark(Sketch& owner, size_t node_idx, const TopoDS_Shape& shp);
  Sketch& owner_sketch;
  size_t  node_idx{};
};

/// Shaded sketch face used for extrusion and face selection.
struct Sketch_face_shp : public AIS_Shape
{
  Sketch_face_shp(Sketch& owner, const TopoDS_Shape& face);

  Sketch&             owner_sketch;
  std::vector<gp_Pnt> verts_3d; // Used for extruding the face
  std::vector<size_t> vert_idxs;
  std::string         name;
};

/// If `shp` is a permanent sketch node mark, removes it from its owner sketch.
bool try_remove_sketch_permanent_node_mark(AIS_Shape* shp);
