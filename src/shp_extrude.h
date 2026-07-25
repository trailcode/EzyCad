#pragma once

#include <PrsDim_LengthDimension.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <optional>

#include "utl_geom.h"
#include "shp_operation.h"

class AIS_Shape;
class V3d_View;
enum class Plane_side;

class Shp_extrude : private Shp_operation_base
{
public:
  Shp_extrude(Occt_view& view);

  void sketch_face_extrude(const ScreenCoords& screen_coords, bool is_mouse_move);
  /// Start extruding a known sketch face (list / script); returns false if `shp` is not a valid face.
  bool begin_face_extrude(const AIS_Shape_ptr& shp);
  void finalize();
  bool cancel();
  bool has_active_extrusion() const;
  bool get_both_sides() const;
  void set_both_sides(bool both_sides);
  bool get_shaded_preview() const;
  /// Show the drag preview shaded instead of wireframe (slower on dense faces).
  void set_shaded_preview(bool shaded);
  void refresh_tmp_dimension_style(const Length_dimension_style& style);

private:
  friend class Shp_extrude_access;

  void _update_extrude(const ScreenCoords& screen_coords);
  void update_extrude_preview_(double extrude_dist, Plane_side side);
  // Build a height-1 prism from the picked face for (side, both-sides). It is cached
  // and stretched along the plane normal each move (affinity) so the live preview
  // avoids a full BRepPrimAPI_MakePrism + shaded remesh per frame.
  void build_unit_prism_(Plane_side side);
  void invalidate_unit_prism_();
  // Face extrude related
  AIS_Shape_ptr              m_to_extrude;
  gp_Pln                     m_to_extrude_pln;
  std::optional<gp_Pnt>      m_to_extrude_pt;
  Shp_ptr                    m_extruded;
  gp_Pln                     m_curr_view_pln;
  PrsDim_LengthDimension_ptr m_tmp_dim;
  Plane_side                 m_extrude_side;
  bool                       m_extrude_both_sides{false};
  bool                       m_shaded_preview{false};
  // Cached height-1 prism and the parameters it was built for.
  TopoDS_Shape          m_unit_prism;
  Plane_side            m_unit_prism_side;
  bool                  m_unit_prism_both_sides{false};
  std::optional<double> m_last_preview_dist;
};
