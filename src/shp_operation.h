#pragma once

#include "shp.h"
#include "types.h"

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
  GUI&                    gui();
  Occt_view&              view();
  AIS_InteractiveContext& ctx();

  std::vector<Shp_ptr> get_selected_shps_() const;
  [[nodiscard]] Status ensure_operation_shps_();
  [[nodiscard]] Status ensure_operation_multi_shps_();
  void                 delete_operation_shps_();
  void                 operation_shps_finalize_();
  void                 operation_shps_cancel_();

  AIS_Shape_ptr      get_shape_(const ScreenCoords& screen_coords);
  const TopoDS_Face* get_face_(const ScreenCoords& screen_coords) const;
  const TopoDS_Wire* get_wire_(const ScreenCoords& screen_coords) const;
  const TopoDS_Edge* get_edge_(const ScreenCoords& screen_coords) const;

  void add_shp_(Shp_ptr& shp);

  /// After transform-only changes on `m_shps`, refresh presentations once (avoids N x viewer updates per frame).
  void redisplay_operation_shps_after_transform_();

  std::vector<Shp_ptr> m_shps;

 private:
  Occt_view& m_view;
};