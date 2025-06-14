#pragma once

#include "shp_operation.h"

struct Rotate_options
{
  bool constr_axis_x {false};
  bool constr_axis_y {false};
  bool constr_axis_z {false};
  bool custom_center {false};
};

class Shp_rotate : private Shp_operation_base
{
 public:
  Shp_rotate(Occt_view& view);

  [[nodiscard]] Status rotate_selected(const ScreenCoords& screen_coords);
  [[nodiscard]] Status show_angle_edit(const ScreenCoords& screen_coords);
  void                 finalize();
  void                 cancel();
  Rotate_options&      get_opts();

 private:
  [[nodiscard]] Status ensure_start_state_(); 
  void preview_rotate_();
  void post_opts_();
  void update_rotation_axis_();
  void update_rotation_center_();

  std::optional<gp_Pln> m_rotate_pln;
  std::optional<gp_Pnt> m_initial_mouse_pos;
  std::optional<gp_Pnt> m_center;
  double                m_angle {0};
  Rotate_options        m_opts;
  AIS_Shape_ptr         m_rotation_axis_vis;
  AIS_Shape_ptr         m_rotation_center_vis;
};