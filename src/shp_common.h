#pragma once

#include "shp_operation.h"

class Shp_common : private Shp_operation_base
{
public:
  Shp_common(Occt_view& view);

  /// Intersect two or more shapes; deletes inputs and adds the result. Returns the common shape.
  [[nodiscard]] Shp_rslt common(std::vector<Shp_ptr> shps);

  /// Common using the current multi-shape selection (GUI).
  [[nodiscard]] Status selected_common();
};
