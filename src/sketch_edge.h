#pragma once

#include <gp_Pln.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Vec2d.hxx>
#include <optional>
#include <string>

#include "utl_types.h"

/// One sketch edge segment (linear segment or circle arc).
struct Sketch_edge
{
  size_t                node_idx_a;
  std::optional<size_t> node_idx_b;      // End node (lines and arcs).
  std::optional<size_t> node_idx_arc_pt; // Arc curve midpoint snap node (when add-midpoint option on); not a graph vertex.
  std::optional<size_t> node_idx_mid;    // Midpoint snap node for lines (when add-midpoint option on).

  Sketch_AIS_edge_ptr shp; // Edge annotation and OCCT geometry.
  std::string         name;

  bool reversed(size_t idx_a, size_t idx_b) const;
};

[[nodiscard]] bool sketch_edge_is_linear(const Sketch_edge& e);
[[nodiscard]] bool sketch_edge_is_arc(const Sketch_edge& e);

/// Unit direction at \a from_pt toward \a to_pt along \a e (chord for lines, curve tangent for arcs).
[[nodiscard]] gp_Vec2d sketch_edge_outgoing_dir_2d(const Sketch_edge& e, const gp_Pnt2d& from_pt, const gp_Pnt2d& to_pt,
                                                   const gp_Pln& pln);

/// Unit arrival direction at \a to_pt when traveling from \a from_pt along \a e.
[[nodiscard]] gp_Vec2d sketch_edge_incoming_dir_2d(const Sketch_edge& e, const gp_Pnt2d& from_pt, const gp_Pnt2d& to_pt,
                                                   const gp_Pln& pln);

/// Read-only linear edge (node indices only).
struct Sketch_edge_linear
{
  size_t                node_a;
  size_t                node_b;
  std::optional<size_t> node_mid;
};

/// Read-only circle-arc edge (start, arc point, end).
struct Sketch_edge_arc
{
  size_t              node_start;
  size_t              node_arc;
  size_t              node_end;
  Sketch_AIS_edge_ptr shp;
};
