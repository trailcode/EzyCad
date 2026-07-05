#include "sketch_edge.h"

#include "utl.h"

bool Sketch_edge::reversed(size_t idx_a, size_t idx_b) const
{
  if (node_idx_a == idx_a && node_idx_b == idx_b)
    return false;

  if (node_idx_a == idx_b && node_idx_b == idx_a)
    return true;

  EZY_ASSERT(false);
  return false;
}

bool sketch_edge_is_linear(const Sketch_edge& e) { return !e.circle_arc && e.node_idx_b.has_value(); }
