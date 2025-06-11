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

   std::optional<ScreenCoords> m_start;
};