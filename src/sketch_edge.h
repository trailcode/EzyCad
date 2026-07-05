#pragma once

#include <optional>
#include <string>

#include "utl_types.h"

/// One sketch edge segment (linear or part of a circle arc).
struct Sketch_edge
{
  size_t                node_idx_a;
  std::optional<size_t> node_idx_b;
  std::optional<size_t> node_idx_arc; // Only valid for circle arc edges.
  std::optional<size_t> node_idx_mid; // Midpoint of edge, used for snapping.

  // Used to identify the two `Sketch_edge`s defining a circle arc.
  Geom_TrimmedCurve_ptr circle_arc;
  Sketch_AIS_edge_ptr   shp; // Current edge annotation.

  std::string name;

  bool reversed(size_t idx_a, size_t idx_b) const;
};

[[nodiscard]] bool sketch_edge_is_linear(const Sketch_edge& e);

/// Read-only linear edge (node indices only).
struct Sketch_edge_linear
{
  size_t                node_a;
  size_t                node_b;
  std::optional<size_t> node_mid;
};

/// Read-only circle-arc edge (start, arc point, end); both arc halves share `shp`.
struct Sketch_edge_arc
{
  size_t              node_start;
  size_t              node_arc;
  size_t              node_end;
  Sketch_AIS_edge_ptr shp;
};
