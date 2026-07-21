#pragma once

#include "shp_operation.h"

#include <gp_Ax3.hxx>
#include <TopoDS_Shape.hxx>

#include <cstddef>

enum class Section_plane
{
  XY,
  XZ,
  YZ
};

struct Section_geometry
{
  TopoDS_Shape shape;
  std::size_t  edge_count{0};
  std::size_t  line_count{0};
  std::size_t  circle_count{0};
  std::size_t  ellipse_count{0};
  std::size_t  bspline_count{0};
  std::size_t  other_curve_count{0};
};

/// Compute a section in \a frame local coordinates. Offset is in model units
/// along the selected local plane normal.
Result<Section_geometry> section_shape(const TopoDS_Shape& shape, const gp_Ax3& frame, Section_plane plane, double offset);

class Shp_section : private Shp_operation_base
{
public:
  explicit Shp_section(Occt_view& view);

  [[nodiscard]] Status preview_selected();
  void                 clear();

  Section_plane get_plane() const { return m_plane; }
  void          set_plane(Section_plane plane) { m_plane = plane; }
  double        get_offset_display() const { return m_offset_display; }
  void          set_offset_display(double offset) { m_offset_display = offset; }
  bool          has_preview() const { return !m_preview.IsNull(); }

private:
  Section_plane m_plane{Section_plane::XY};
  double        m_offset_display{0.0};
  AIS_Shape_ptr m_preview;
  AIS_Shape_ptr m_plane_fill;
  AIS_Shape_ptr m_plane_lines;
};
