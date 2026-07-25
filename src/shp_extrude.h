#pragma once

#include <PrsDim_LengthDimension.hxx>
#include <gp_Ax1.hxx>
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
  /// LMB while a preview is active: lock height and enter twist (if Twist on), else finalize.
  void on_left_click();
  bool cancel();
  bool has_active_extrusion() const;
  bool get_both_sides() const;
  void set_both_sides(bool both_sides);
  bool get_twist() const;
  void set_twist(bool twist);
  bool is_twist_phase() const;
  /// Shift+Tab during twist phase: open angle edit; no-op in height phase.
  void begin_angle_input(const ScreenCoords& screen_coords);
  void refresh_tmp_dimension_style(const Length_dimension_style& style);

private:
  friend class Shp_extrude_access;

  enum class Phase
  {
    Height,
    Twist
  };

  void         _update_extrude(const ScreenCoords& screen_coords);
  void         update_height_(const ScreenCoords& screen_coords);
  void         update_twist_(const ScreenCoords& screen_coords);
  void         lock_height_begin_twist_();
  void         update_extrude_preview_(double extrude_dist, Plane_side side);
  void         update_dim_(double extrude_dist, Plane_side side);
  void         clear_preview_();
  void         clear_lite_other_face_();
  void         clear_session_inputs_();
  bool         use_lite_preview_();
  gp_Ax1       twist_axis_() const;
  TopoDS_Shape make_prism_body_(double extrude_dist, Plane_side side) const;
  TopoDS_Shape make_body_(double extrude_dist, Plane_side side, double twist_rad) const;

  // Face extrude related
  AIS_Shape_ptr         m_to_extrude;
  gp_Pln                m_to_extrude_pln;
  std::optional<gp_Pnt> m_to_extrude_pt;
  Shp_ptr               m_extruded;
  /// Second face copy for lite both-sides preview (near side); not a document shape.
  AIS_Shape_ptr              m_lite_face_other;
  gp_Pln                     m_curr_view_pln;
  PrsDim_LengthDimension_ptr m_tmp_dim;
  Plane_side                 m_extrude_side;
  bool                       m_extrude_both_sides{false};
  bool                       m_twist_enabled{false};
  Phase                      m_phase{Phase::Height};
  double                     m_twist_angle{0.0}; // radians, CCW about face centroid / plane normal
  gp_Pnt                     m_twist_centroid;
  bool                       m_show_angle_input{false};
  std::optional<double>      m_entered_twist_deg;
  // Faces with more than the settings threshold use a translated face-copy preview.
  size_t                m_face_edge_count{0};
  bool                  m_lite_preview_active{false};
  std::optional<double> m_last_preview_dist;
  Plane_side            m_last_preview_side;
  bool                  m_last_preview_both_sides{false};
  double                m_last_preview_twist{0.0};
};
