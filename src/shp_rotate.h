#pragma once

#include "shp_operation.h"

class Shp_rotate : private Shp_operation_base
{
 public:
  Shp_rotate(Occt_view& view);

  [[nodiscard]] Status rotate_selected(const ScreenCoords& screen_coords);
  [[nodiscard]] Status show_angle_edit(const ScreenCoords& screen_coords);
  void                 finalize();
  void                 cancel();

 private:
  [[nodiscard]] Status ensure_start_state_(); 
  void preview_rotate_();
  void post_opts_();

  std::optional<gp_Pln> m_rotate_pln;
  std::optional<gp_Pnt> m_initial_mouse_pos;
  gp_Pnt                m_center;
  double                m_angle {0};
};