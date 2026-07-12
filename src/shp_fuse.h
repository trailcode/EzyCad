#pragma once

#include "shp_operation.h"

class Shp_fuse : private Shp_operation_base
{
public:
  Shp_fuse(Occt_view& view);

  /// Fuse two or more shapes; deletes inputs and adds the result. Returns the fused shape.
  [[nodiscard]] Shp_rslt fuse(std::vector<Shp_ptr> shps);

  /// Fuse the current multi-shape selection (GUI).
  [[nodiscard]] Status selected_fuse();
};
