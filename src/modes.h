#pragma once

#include <array>
#include <cstddef>
#include <string_view>

// --- Mode ------------------------------------------------------------------
// Single source of truth: order here defines enum numeric values and c_mode_strs indices
// (mode_from_string, persistence, etc.). Do not reorder without migrating saved data.
#define EZY_MODE_LIST(X)                                                                               \
  X(Normal)                                                                                            \
  X(Move)                                                                                              \
  X(Scale)                                                                                             \
  X(Rotate)                                                                                            \
  X(Sketch_inspection_mode) /* inspecting sketch elements */                                           \
  X(Sketch_from_planar_face) /* sketch from a planar face */                                           \
  X(Sketch_face_extrude) /* extrude a sketch face */                                                   \
  X(Shape_chamfer)                                                                                     \
  X(Shape_fillet)                                                                                      \
  X(Shape_polar_duplicate)                                                                             \
  X(Sketch_add_node)                                                                                   \
  X(Sketch_add_edge)                                                                                   \
  X(Sketch_add_multi_edges)                                                                            \
  X(Sketch_add_seg_circle_arc)                                                                         \
  X(Sketch_operation_axis) /* mirror / revolve */                                                      \
  X(Sketch_add_square)                                                                                 \
  X(Sketch_add_rectangle)                                                                              \
  X(Sketch_add_rectangle_center_pt)                                                                    \
  X(Sketch_add_circle)                                                                                 \
  X(Sketch_add_circle_3_pts)                                                                           \
  X(Sketch_add_slot)                                                                                   \
  X(Sketch_toggle_edge_dim)

enum class Mode
{
#define EZY_MODE_ENUM(name) name,
  EZY_MODE_LIST(EZY_MODE_ENUM)
#undef EZY_MODE_ENUM
      _count  // must be last; equals number of EZY_MODE_LIST entries
};

constexpr std::array<std::string_view, static_cast<std::size_t>(Mode::_count)> c_mode_strs {
#define EZY_MODE_STR(name) #name,
    EZY_MODE_LIST(EZY_MODE_STR)
#undef EZY_MODE_STR
};

static_assert(c_mode_strs.size() == static_cast<std::size_t>(Mode::_count));

#undef EZY_MODE_LIST

// --- Chamfer_mode / Fillet_mode (shared member set) ------------------------
#define EZY_CHAMFER_FILLET_MODE_LIST(X)                                                                \
  X(Edge)                                                                                              \
  X(Wire)                                                                                              \
  X(Face)                                                                                              \
  X(Shape)

enum class Chamfer_mode
{
#define EZY_CF_ENUM(name) name,
  EZY_CHAMFER_FILLET_MODE_LIST(EZY_CF_ENUM)
#undef EZY_CF_ENUM
      _count
};

constexpr std::array<const char*, static_cast<std::size_t>(Chamfer_mode::_count)> c_chamfer_mode_strs {
#define EZY_CF_STR(name) #name,
    EZY_CHAMFER_FILLET_MODE_LIST(EZY_CF_STR)
#undef EZY_CF_STR
};

static_assert(c_chamfer_mode_strs.size() == static_cast<std::size_t>(Chamfer_mode::_count));

enum class Fillet_mode
{
#define EZY_CF_ENUM2(name) name,
  EZY_CHAMFER_FILLET_MODE_LIST(EZY_CF_ENUM2)
#undef EZY_CF_ENUM2
      _count
};

constexpr std::array<const char*, static_cast<std::size_t>(Fillet_mode::_count)> c_fillet_mode_strs {
#define EZY_CF_STR2(name) #name,
    EZY_CHAMFER_FILLET_MODE_LIST(EZY_CF_STR2)
#undef EZY_CF_STR2
};

static_assert(c_fillet_mode_strs.size() == static_cast<std::size_t>(Fillet_mode::_count));

#undef EZY_CHAMFER_FILLET_MODE_LIST

bool is_sketch_mode(const Mode mode);

/// Return Mode for a name (e.g. "Normal", "Sketch_add_edge"). Returns Normal if not found.
Mode mode_from_string(std::string_view name);
