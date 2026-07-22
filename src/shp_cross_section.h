#pragma once

#include "shp_operation.h"

#include <gp_Ax3.hxx>
#include <TopoDS_Shape.hxx>

#include <cstddef>
#include <vector>

enum class Cross_section_plane
{
  XY,
  XZ,
  YZ
};

struct Cross_section_geometry
{
  TopoDS_Shape shape;
  std::size_t  edge_count{0};
  std::size_t  line_count{0};
  std::size_t  circle_count{0};
  std::size_t  ellipse_count{0};
  std::size_t  bspline_count{0};
  std::size_t  other_curve_count{0};
};

/// Compute a cross-section with a plane from \a frame local coordinates. Offset is in
/// model units along the selected local plane normal. Interactive preview uses one
/// shared plane for the whole selection (first-shape axes, selection bbox center).
Result<Cross_section_geometry> cross_section_shape(const TopoDS_Shape& shape, const gp_Ax3& frame, Cross_section_plane plane,
                                                   double offset);

class Shp_cross_section : private Shp_operation_base
{
public:
  explicit Shp_cross_section(Occt_view& view);

  [[nodiscard]] Status preview_selected();
  /// Same as preview_selected, but uses \a shapes instead of the current AIS selection.
  [[nodiscard]] Status preview(const std::vector<Shp_ptr>& shapes);
  void                 clear();

  Cross_section_plane get_plane() const { return m_plane; }
  void                set_plane(Cross_section_plane plane) { m_plane = plane; }
  double              get_offset_display() const { return m_offset_display; }
  void                set_offset_display(double offset) { m_offset_display = offset; }
  bool                has_preview() const { return !m_preview.IsNull(); }
  /// True when the current AIS selection ids differ from the last acknowledged set.
  [[nodiscard]] bool selection_stale() const;
  /// True when selection, section plane, or offset differ from the last preview/acknowledge.
  [[nodiscard]] bool preview_inputs_stale() const;
  /// Record the current AIS selection, plane, and offset as acknowledged without rebuilding.
  void acknowledge_current_selection();
  /// Projected extent of the current selection along the shared cross-section plane normal,
  /// in display units. False when nothing solid is selected or bounds are empty.
  [[nodiscard]] bool try_get_offset_range_display(double& out_min, double& out_max);

private:
  static std::vector<Shape_id> selection_ids_(const std::vector<Shp_ptr>& shapes);
  void                         acknowledge_inputs_(const std::vector<Shp_ptr>& shapes);

  Cross_section_plane   m_plane{Cross_section_plane::XY};
  double                m_offset_display{0.0};
  Cross_section_plane   m_acked_plane{Cross_section_plane::XY};
  double                m_acked_offset_display{0.0};
  std::vector<Shape_id> m_acked_selection_ids;
  AIS_Shape_ptr         m_preview;
  AIS_Shape_ptr         m_plane_fill;
  AIS_Shape_ptr         m_plane_lines;
};
