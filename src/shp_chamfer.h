#pragma once

#include "shp_operation.h"

enum class Chamfer_mode;

class Shp_chamfer : private Shp_operation_base
{
 public:
  Shp_chamfer(Occt_view& view);

  [[nodiscard]] Status add_chamfer(const ScreenCoords& screen_coords, const Chamfer_mode chamfer_mode);

  void   set_chamfer_dist(const double dist);
  double get_chamfer_dist() const;

 private:

   double m_chamfer_dist {1.0};
};