#pragma once

#include "shp_operation.h"

class Shp_fuse : private Shp_operation_base
{
 public:

   Shp_fuse(Occt_view& view);

   [[nodiscard]] Status selected_fuse();
};