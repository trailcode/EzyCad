#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

/// Remappable GUI keyboard actions (stable string ids in settings JSON).
enum class Gui_action
{
  Mode_move,
  Mode_rotate,
  Mode_scale,
  Mode_extrude,
  Mode_chamfer,
  Mode_fillet,
  Mode_dimension,
  Mode_sketch_inspection,
  Mode_sketch_from_face,
  Mode_operation_axis,
  Mode_add_node,
  Mode_add_edge,
  Mode_add_multi_edges,
  Mode_add_arc,
  Mode_add_square,
  Mode_add_rectangle,
  Mode_add_rectangle_center,
  Mode_add_circle,
  Mode_add_circle_3_pts,
  Mode_add_slot,
  Mode_polar_duplicate,
  Mode_cross_section,
  Cmd_shape_cut,
  Cmd_shape_fuse,
  Cmd_shape_common,
  Edit_delete,
  File_new,
  File_open,
  File_save,
  Edit_undo,
  Edit_redo,
  _count
};

struct Key_chord
{
  int key{0};
  int mods{0}; // GLFW_MOD_SHIFT | CONTROL | ALT | SUPER only

  [[nodiscard]] bool operator==(const Key_chord& o) const;
  [[nodiscard]] bool operator!=(const Key_chord& o) const { return !(*this == o); }
};

/// Action -> chord map with conflict checks and human-readable chord strings.
class Gui_hotkeys
{
public:
  Gui_hotkeys();

  void reset_defaults();
  void reset_action(Gui_action action);

  [[nodiscard]] Key_chord                  chord_for(Gui_action action) const;
  [[nodiscard]] std::optional<Gui_action>  action_for(int key, int mods) const;
  /// Returns false if \a chord is reserved, invalid, or already bound to a different action.
  [[nodiscard]] bool                       set_chord(Gui_action action, Key_chord chord);

  [[nodiscard]] static const char*         action_id(Gui_action action);
  [[nodiscard]] static const char*         action_label(Gui_action action);
  [[nodiscard]] static std::optional<Gui_action> action_from_id(std::string_view id);

  [[nodiscard]] static int                 normalize_mods(int mods);
  /// Fixed chords handled outside the remappable map (Esc, Tab, digits, view nav, Ctrl+Shift+Z, ...).
  [[nodiscard]] static bool                is_reserved_chord(Key_chord chord);
  [[nodiscard]] static std::string         format_chord(Key_chord chord);
  [[nodiscard]] static std::optional<Key_chord> parse_chord(std::string_view text);

  [[nodiscard]] nlohmann::json             to_json() const;
  /// Overlay known action ids from \a obj; missing / invalid keys keep current values.
  void                                     merge_from_json(const nlohmann::json& obj);

  static constexpr int k_count = static_cast<int>(Gui_action::_count);

private:
  std::array<Key_chord, k_count> m_chords{};
};
