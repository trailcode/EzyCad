#pragma once

#include <string>
#include <vector>

#include <TopoDS_Shape.hxx>

namespace shp_info
{
struct Display_meta
{
  std::string name;
  std::string material;
  std::string display_mode;
  bool        visible = true;
};

struct Line
{
  std::string label;
  std::string value;
};

/// Collect OCCT topology and property lines for the shape info dialog.
std::vector<Line> collect(const TopoDS_Shape& shape, const Display_meta* display = nullptr);
} // namespace shp_info
