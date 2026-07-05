#include "sketch_ais.h"

#include "sketch.h"

Sketch_AIS_edge::Sketch_AIS_edge(Sketch& owner, const TopoDS_Shape& shp)
    : AIS_Shape(shp)
    , owner_sketch(owner)
{
}

Sketch_AIS_node_mark::Sketch_AIS_node_mark(Sketch& owner, size_t node_idx, const TopoDS_Shape& shp)
    : AIS_Shape(shp)
    , owner_sketch(owner)
    , node_idx(node_idx)
{
}

Sketch_face_shp::Sketch_face_shp(Sketch& owner, const TopoDS_Shape& face)
    : owner_sketch(owner)
    , AIS_Shape(face)
{
}

bool try_remove_sketch_permanent_node_mark(AIS_Shape* shp)
{
  if (!shp)
    return false;

  if (auto* mark = dynamic_cast<Sketch_AIS_node_mark*>(shp))
  {
    mark->owner_sketch.remove_permanent_node_mark(*mark);
    return true;
  }

  return false;
}
