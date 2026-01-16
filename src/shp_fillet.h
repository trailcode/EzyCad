#pragma once

#include "shp_operation.h"

enum class Fillet_mode;

class Shp_fillet : private Shp_operation_base
{
 public:
  Shp_fillet(Occt_view& view);

  [[nodiscard]] Status add_fillet(const ScreenCoords& screen_coords, const Fillet_mode fillet_mode);

  void   set_fillet_radius(const double radius);
  double get_fillet_radius() const;

 private:

   double m_fillet_radius {1.0};
};

