#pragma once

#include "shp_operation.h"

class Shp_cut : private Shp_operation_base
{
public:
  Shp_cut(Occt_view& view);

  /// Cut tools from the first shape; deletes inputs and adds the result. Returns the cut shape.
  /// Argument order matches selection: first shape is the body, the rest are tools.
  [[nodiscard]] Shp_rslt cut(std::vector<Shp_ptr> shps);

  /// Cut using the current multi-shape selection (GUI).
  [[nodiscard]] Status selected_cut();
};
