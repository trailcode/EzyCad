#include "mode.h"

Mode mode_from_string(std::string_view name)
{
  for (int i = 0; i < static_cast<int>(Mode::_count); ++i)
    if (c_mode_strs[i] == name)
      return static_cast<Mode>(i);

  // Retired names kept so scripts / older docs still resolve.
  if (name == "Normal")
    return Mode::Design_inspection;

  if (name == "Sketch_inspection_mode")
    return Mode::Sketch_inspection;

  if (name == "Move")
    return Mode::Workbench_move;

  if (name == "Rotate")
    return Mode::Workbench_rotate;

  if (name == "Workbench_scale")
    return Mode::Scale;

  if (name == "Shape_shaft_align" || name == "Shape_cyl_align")
    return Mode::Workbench_shaft_align;

  return Mode::Design_inspection;
}

bool is_sketch_mode(const Mode mode)
{
  switch (mode)
  {
  case Mode::Sketch_inspection:
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
  case Mode::Sketch_add_bone:
  case Mode::Sketch_operation_axis:
  case Mode::Sketch_dim_anno:
  case Mode::Sketch_face_extrude:
    return true;

  default:
    return false;
  }
}

bool is_workbench_transform_mode(const Mode mode)
{
  switch (mode)
  {
  case Mode::Workbench_move:
  case Mode::Workbench_rotate:
  case Mode::Workbench_shaft_align:
    return true;

  default:
    return false;
  }
}

bool is_workbench_mode(const Mode mode)
{
  return mode == Mode::Workbench_inspection || is_workbench_transform_mode(mode);
}

bool is_shape_browse_mode(const Mode mode)
{
  return mode == Mode::Design_inspection || mode == Mode::Workbench_inspection;
}

Task task_of(Mode mode)
{
  if (is_workbench_mode(mode))
    return Task::Workbench;

  switch (mode)
  {
  case Mode::Sketch_inspection:
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
  case Mode::Sketch_add_bone:
  case Mode::Sketch_operation_axis:
  case Mode::Sketch_dim_anno:
    return Task::Sketch;

  default:
    return Task::Design;
  }
}

Mode idle_mode_of(Task task)
{
  switch (task)
  {
  case Task::Sketch:
    return Mode::Sketch_inspection;

  case Task::Workbench:
    return Mode::Workbench_inspection;

  case Task::Design:
  default:
    return Mode::Design_inspection;
  }
}
