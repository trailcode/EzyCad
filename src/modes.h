#pragma once

#include <array>
#include <string_view>

enum class Mode
{
  Normal,
  Move,
  Scale,
  Rotate,
  Sketch_inspection_mode,          // For inspecting sketch elements
  Sketch_from_planar_face,         // For creating a sketch from a planar face
  Sketch_face_extrude,             // For extruding a sketch face
  Shape_chamfer,                   // For chamfering a shape
  Shape_fillet,                    // For filleting a shape
  Shape_polar_duplicate,           // For polar duplicating a shape
  Sketch_add_node,                 // For adding a node to a sketch
  Sketch_add_edge,                 // For adding an edge to a sketch
  Sketch_add_multi_edges,          // For adding multiple edges to a sketch
  Sketch_add_seg_circle_arc,       // For adding a segmented circle arc to a sketch
  Sketch_operation_axis,           // For mirroring or revolving sketch elements
  Sketch_add_square,               // For adding a square to a sketch
  Sketch_add_rectangle,            // For adding a rectangle to a sketch
  Sketch_add_rectangle_center_pt,  // For adding a rectangle with center point to a sketch
  Sketch_add_circle,               // For adding a circle to a sketch
  Sketch_add_circle_3_pts,         // For adding a circle from three points to a sketch
  Sketch_add_slot,                 // For adding a slot to a sketch
  Sketch_toggle_edge_dim,          // For toggling edge dimension annotation
  _count                           // Must be last
};

// Corresponds to above
constexpr std::array<std::string_view, int(Mode::_count)> c_mode_strs {
    "Normal",
    "Move",
    "Scale",
    "Rotate",
    "Sketch_inspection_mode",
    "Sketch_from_planar_face",
    "Sketch_add_node",
    "Sketch_add_edge",
    "Sketch_add_multi_edges",
    "Sketch_add_seg_circle_arc",
    "Sketch_operation_axis",
    "Sketch_add_square",
    "Sketch_add_rectangle",
    "Sketch_add_rectangle_center_pt",
    "Sketch_add_circle",
    "Sketch_add_circle_3_pts",
    "Sketch_add_slot",
    "Sketch_toggle_edge_dim",
    "Sketch_face_extrude",
    "Shape_chamfer",
    "Shape_fillet",
    "Shape_polar_duplicate"};

enum class Chamfer_mode
{
  Edge,
  Wire,
  Face,
  Shape,
};

// Corresponds to the above
constexpr std::array<const char*, 4> c_chamfer_mode_strs = {"Edge", "Wire", "Face", "Shape"};

enum class Fillet_mode
{
  Edge,
  Wire,
  Face,
  Shape,
};

// Corresponds to the above
constexpr std::array<const char*, 4> c_fillet_mode_strs = {"Edge", "Wire", "Face", "Shape"};

bool is_sketch_mode(const Mode mode);
