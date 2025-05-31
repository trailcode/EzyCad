#include "modes.h"

bool is_sketch_mode(const Mode mode)
{
  switch (mode)
  {
    case Mode::Sketch_inspection_mode:
    case Mode::Sketch_add_node:
    case Mode::Sketch_add_edge:
    case Mode::Sketch_add_seg_circle_arc:
    case Mode::Sketch_add_multi_edges:
    case Mode::Sketch_add_square:
    case Mode::Sketch_add_rectangle:
    case Mode::Sketch_add_rectangle_center_pt:
    case Mode::Sketch_add_circle:
    case Mode::Sketch_add_circle_3_pts:
    case Mode::Sketch_add_slot:
    case Mode::Sketch_operation_axis:
    case Mode::Sketch_toggle_edge_dim:
    case Mode::Sketch_face_extrude:
      return true;

    default:
      return false;
  }
}
