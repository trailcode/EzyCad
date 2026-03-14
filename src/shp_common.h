#pragma once

#include "shp_operation.h"

class Shp_common : private Shp_operation_base
{
 public:
  Shp_common(Occt_view& view);

  [[nodiscard]] Status selected_common();
};