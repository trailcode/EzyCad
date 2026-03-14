#pragma once

#include "shp_operation.h"

class Shp_cut : private Shp_operation_base
{
 public:
  Shp_cut(Occt_view& view);

  [[nodiscard]] Status selected_cut();
};