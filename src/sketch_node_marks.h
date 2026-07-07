#pragma once

#include <vector>

#include "sketch_ais.h"
#include "utl_types.h"

class Sketch;

/// Permanent sketch-node '+' markers (display, sync, and removal).
class Sketch_node_marks
{
public:
  explicit Sketch_node_marks(Sketch& sketch);

  void sync();
  void remove_at(size_t node_idx);
  void remove_all();
  void erase_all();
  void trim_trailing();

private:
  friend class Sketch_delta;

  void apply_style_(AIS_Shape_ptr& shp, bool origin) const;

  Sketch&                               m_sketch;
  std::vector<Sketch_AIS_node_mark_ptr> m_marks;
};
