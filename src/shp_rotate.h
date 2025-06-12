#pragma once

#include "shp_operation.h"

class Shp_rotate : private Shp_operation_base
{
 public:
  Shp_rotate(Occt_view& view);

  [[nodiscard]] Status rotate_selected(const ScreenCoords& screen_coords);
  void                 show_dist_edit(const ScreenCoords& screen_coords);
  void                 finalize_rotate_selected();
  void                 cancel_rotate_selected();

 private:
  std::optional<gp_Pln> m_rotate_pln;
  std::optional<gp_Pnt> m_initial_mouse_pos;
};