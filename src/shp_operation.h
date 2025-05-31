#pragma once

#include "types.h"
#include "shapes.h"
#include "result.h"

class GUI;
class Occt_view;
class AIS_InteractiveContext;
class TopoDS_Face;
class TopoDS_Wire;
class TopoDS_Edge;

class Shp_operation_base
{
 protected:
  Shp_operation_base(Occt_view& view);
  GUI& gui();
  Occt_view& view();
  AIS_InteractiveContext& ctx();

  std::vector<AIS_Shape_ptr> get_selected() const;
  void delete_selected_();

  AIS_Shape_ptr              get_shape(const ScreenCoords& screen_coords);
  const TopoDS_Face*         get_face_(const ScreenCoords& screen_coords) const;
  const TopoDS_Wire*         get_wire_(const ScreenCoords& screen_coords) const;
  const TopoDS_Edge*         get_edge_(const ScreenCoords& screen_coords) const;

  void add_shp_(ShapeBase_ptr& shp);

 private:
  Occt_view& m_view;
};