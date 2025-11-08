#pragma once

#include "shp_operation.h"
#include <PrsDim_LengthDimension.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <optional>

class AIS_Shape;
class V3d_View;

class Shp_extrude : private Shp_operation_base
{
 public:
  Shp_extrude(Occt_view& view);

  void sketch_face_extrude(const ScreenCoords& screen_coords, bool is_mouse_move);
  void finalize();
  bool cancel();
  bool has_active_extrusion() const;
  
  // For testing
  void set_curr_view_pln(const gp_Pln& pln);

 private:
  void                       _update_extrude(const ScreenCoords& screen_coords);
  // Face extrude related
  AIS_Shape_ptr              m_to_extrude;
  gp_Pln                     m_to_extrude_pln;
  std::optional<gp_Pnt>      m_to_extrude_pt;
  ExtrudedShp_ptr            m_extruded;
  gp_Pln                     m_curr_view_pln;
  PrsDim_LengthDimension_ptr m_tmp_dim;
};

