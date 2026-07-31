#include "gui_hotkeys.h"

#include <cctype>
#include <sstream>
#include <utility>

#include <GLFW/glfw3.h>

namespace
{
constexpr int k_chord_mods_mask = GLFW_MOD_SHIFT | GLFW_MOD_CONTROL | GLFW_MOD_ALT | GLFW_MOD_SUPER;

struct Action_meta
{
  Gui_action  action;
  const char* id;
  const char* label;
  Key_chord   def;
};

// Defaults avoid existing shape letters (G/R/S/E/C/F/D), X/Y/Z (axis toggles), and digits.
// Sketch: AutoCAD/FreeCAD-flavored mnemonics; booleans use Ctrl+Shift chords.
// clang-format off
constexpr Action_meta c_actions[] = {
    {Gui_action::Mode_move,                 "mode.move",                 "Move",                    {GLFW_KEY_G, 0}},
    {Gui_action::Mode_rotate,               "mode.rotate",               "Rotate",                  {GLFW_KEY_R, 0}},
    {Gui_action::Mode_scale,                "mode.scale",                "Scale",                   {GLFW_KEY_S, 0}},
    {Gui_action::Mode_extrude,              "mode.extrude",              "Extrude",                 {GLFW_KEY_E, 0}},
    {Gui_action::Mode_chamfer,              "mode.chamfer",              "Chamfer",                 {GLFW_KEY_C, 0}},
    {Gui_action::Mode_fillet,               "mode.fillet",               "Fillet",                  {GLFW_KEY_F, 0}},
    {Gui_action::Mode_dimension,            "mode.dimension",            "Dimension",               {GLFW_KEY_D, 0}},
    {Gui_action::Mode_sketch_inspection,    "mode.sketch_inspection",    "Sketch inspection",       {GLFW_KEY_I, 0}},
    {Gui_action::Mode_sketch_from_face,     "mode.sketch_from_face",     "Sketch from face",        {GLFW_KEY_P, 0}},
    {Gui_action::Mode_operation_axis,       "mode.operation_axis",       "Operation axis",          {GLFW_KEY_A, GLFW_MOD_SHIFT}},
    {Gui_action::Mode_add_node,             "mode.add_node",             "Add node",                {GLFW_KEY_N, 0}},
    {Gui_action::Mode_add_edge,             "mode.add_edge",             "Add line",                {GLFW_KEY_L, 0}},
    {Gui_action::Mode_add_multi_edges,      "mode.add_multi_edges",      "Add multi-line",          {GLFW_KEY_L, GLFW_MOD_SHIFT}},
    {Gui_action::Mode_add_arc,              "mode.add_arc",              "Add arc",                 {GLFW_KEY_A, 0}},
    {Gui_action::Mode_add_square,           "mode.add_square",           "Add square",              {GLFW_KEY_Q, 0}},
    {Gui_action::Mode_add_rectangle,        "mode.add_rectangle",        "Add rectangle",           {GLFW_KEY_B, 0}},
    {Gui_action::Mode_add_rectangle_center, "mode.add_rectangle_center", "Add rectangle (center)",  {GLFW_KEY_B, GLFW_MOD_SHIFT}},
    {Gui_action::Mode_add_circle,           "mode.add_circle",           "Add circle",              {GLFW_KEY_O, 0}},
    {Gui_action::Mode_add_circle_3_pts,     "mode.add_circle_3_pts",     "Add circle (3 pts)",      {GLFW_KEY_O, GLFW_MOD_SHIFT}},
    {Gui_action::Mode_add_slot,             "mode.add_slot",             "Add slot",                {GLFW_KEY_U, 0}},
    {Gui_action::Mode_polar_duplicate,      "mode.polar_duplicate",      "Polar duplicate",         {GLFW_KEY_P, GLFW_MOD_SHIFT}},
    {Gui_action::Mode_cross_section,        "mode.cross_section",        "Cross-section",           {GLFW_KEY_X, GLFW_MOD_SHIFT}},
    {Gui_action::Cmd_shape_cut,             "cmd.shape_cut",             "Shape cut",               {GLFW_KEY_C, GLFW_MOD_CONTROL | GLFW_MOD_SHIFT}},
    {Gui_action::Cmd_shape_fuse,            "cmd.shape_fuse",            "Shape fuse",              {GLFW_KEY_F, GLFW_MOD_CONTROL | GLFW_MOD_SHIFT}},
    {Gui_action::Cmd_shape_common,          "cmd.shape_common",          "Shape common",            {GLFW_KEY_M, GLFW_MOD_CONTROL | GLFW_MOD_SHIFT}},
    {Gui_action::Edit_delete,               "edit.delete",               "Delete",                  {GLFW_KEY_D, GLFW_MOD_SHIFT}},
    {Gui_action::File_new,                  "file.new",                  "New project",             {GLFW_KEY_N, GLFW_MOD_CONTROL}},
    {Gui_action::File_open,                 "file.open",                 "Open",                    {GLFW_KEY_O, GLFW_MOD_CONTROL}},
    {Gui_action::File_save,                 "file.save",                 "Save",                    {GLFW_KEY_S, GLFW_MOD_CONTROL}},
    {Gui_action::Edit_undo,                 "edit.undo",                 "Undo",                    {GLFW_KEY_Z, GLFW_MOD_CONTROL}},
    {Gui_action::Edit_redo,                 "edit.redo",                 "Redo",                    {GLFW_KEY_Y, GLFW_MOD_CONTROL}},
};
// clang-format on

static_assert(sizeof(c_actions) / sizeof(c_actions[0]) == Gui_hotkeys::k_count);

[[nodiscard]] bool is_pure_modifier_key(int key)
{
  switch (key)
  {
  case GLFW_KEY_LEFT_SHIFT:
  case GLFW_KEY_RIGHT_SHIFT:
  case GLFW_KEY_LEFT_CONTROL:
  case GLFW_KEY_RIGHT_CONTROL:
  case GLFW_KEY_LEFT_ALT:
  case GLFW_KEY_RIGHT_ALT:
  case GLFW_KEY_LEFT_SUPER:
  case GLFW_KEY_RIGHT_SUPER:
    return true;
  default:
    return false;
  }
}

[[nodiscard]] std::string key_token(int key)
{
  if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z)
    return std::string(1, static_cast<char>('A' + (key - GLFW_KEY_A)));

  if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9)
    return std::string(1, static_cast<char>('0' + (key - GLFW_KEY_0)));

  switch (key)
  {
  case GLFW_KEY_DELETE:
    return "Delete";
  case GLFW_KEY_BACKSPACE:
    return "Backspace";
  case GLFW_KEY_TAB:
    return "Tab";
  case GLFW_KEY_ENTER:
    return "Enter";
  case GLFW_KEY_ESCAPE:
    return "Esc";
  case GLFW_KEY_SPACE:
    return "Space";
  default:
  {
    std::ostringstream oss;
    oss << "Key" << key;
    return oss.str();
  }
  }
}

[[nodiscard]] std::optional<int> parse_key_token(std::string_view tok)
{
  if (tok.size() == 1)
  {
    const char c = tok[0];
    if (c >= 'A' && c <= 'Z')
      return GLFW_KEY_A + (c - 'A');
    if (c >= 'a' && c <= 'z')
      return GLFW_KEY_A + (c - 'a');
    if (c >= '0' && c <= '9')
      return GLFW_KEY_0 + (c - '0');
  }

  auto eq = [&](const char* lit)
  {
    const std::string_view l(lit);
    if (tok.size() != l.size())
      return false;
    for (size_t i = 0; i < tok.size(); ++i)
      if (std::tolower(static_cast<unsigned char>(tok[i])) != std::tolower(static_cast<unsigned char>(l[i])))
        return false;
    return true;
  };

  if (eq("Delete"))
    return GLFW_KEY_DELETE;
  if (eq("Backspace"))
    return GLFW_KEY_BACKSPACE;
  if (eq("Tab"))
    return GLFW_KEY_TAB;
  if (eq("Enter") || eq("Return"))
    return GLFW_KEY_ENTER;
  if (eq("Esc") || eq("Escape"))
    return GLFW_KEY_ESCAPE;
  if (eq("Space"))
    return GLFW_KEY_SPACE;
  return std::nullopt;
}
} // namespace

bool Key_chord::operator==(const Key_chord& o) const
{
  return key == o.key && Gui_hotkeys::normalize_mods(mods) == Gui_hotkeys::normalize_mods(o.mods);
}

Gui_hotkeys::Gui_hotkeys() { reset_defaults(); }

void Gui_hotkeys::reset_defaults()
{
  for (const Action_meta& m : c_actions)
    m_chords[static_cast<int>(m.action)] = m.def;
}

bool Gui_hotkeys::reset_action(Gui_action action)
{
  const int i = static_cast<int>(action);
  if (i < 0 || i >= k_count)
    return false;
  // Already factory: succeed without rewriting (avoids a false conflict if a
  // duplicate somehow already exists on another row).
  if (m_chords[i] == c_actions[i].def)
    return true;
  return set_chord(action, c_actions[i].def);
}

Key_chord Gui_hotkeys::chord_for(Gui_action action) const
{
  const int i = static_cast<int>(action);
  if (i < 0 || i >= k_count)
    return {};
  return m_chords[i];
}

std::optional<Gui_action> Gui_hotkeys::action_for(int key, int mods) const
{
  if (is_pure_modifier_key(key))
    return std::nullopt;

  const Key_chord needle{key, normalize_mods(mods)};
  for (int i = 0; i < k_count; ++i)
    if (m_chords[i] == needle)
      return static_cast<Gui_action>(i);
  return std::nullopt;
}

bool Gui_hotkeys::is_reserved_chord(Key_chord chord)
{
  chord.mods     = normalize_mods(chord.mods);
  const int key  = chord.key;
  const int mods = chord.mods;

  switch (key)
  {
  case GLFW_KEY_ESCAPE:
  case GLFW_KEY_ENTER:
  case GLFW_KEY_TAB:
  case GLFW_KEY_DELETE:
  case GLFW_KEY_BACKSPACE:
    return true;
  default:
    break;
  }

  // Fixed redo alias (edit.redo remappable default remains Ctrl+Y).
  if (key == GLFW_KEY_Z && (mods & GLFW_MOD_CONTROL) != 0 && (mods & GLFW_MOD_SHIFT) != 0 &&
      (mods & (GLFW_MOD_ALT | GLFW_MOD_SUPER)) == 0)
    return true;

  const bool no_ctrl_alt_super = (mods & (GLFW_MOD_CONTROL | GLFW_MOD_ALT | GLFW_MOD_SUPER)) == 0;

  // Selection filter digits (Normal mode); also blocks Shift+digit.
  if (no_ctrl_alt_super)
  {
    if ((key >= GLFW_KEY_1 && key <= GLFW_KEY_9) || (key >= GLFW_KEY_KP_1 && key <= GLFW_KEY_KP_9))
      return true;
  }

  // View zoom (+/-) without Ctrl/Alt.
  if ((mods & (GLFW_MOD_CONTROL | GLFW_MOD_ALT)) == 0)
  {
    if (key == GLFW_KEY_KP_ADD || key == GLFW_KEY_KP_SUBTRACT || key == GLFW_KEY_MINUS)
      return true;
    if (key == GLFW_KEY_EQUAL && (mods & GLFW_MOD_SHIFT) != 0)
      return true;
  }

  // View roll: Shift + KP4/6, 4/6, or Left/Right.
  if ((mods & GLFW_MOD_SHIFT) != 0 && (mods & (GLFW_MOD_CONTROL | GLFW_MOD_ALT)) == 0)
  {
    if (key == GLFW_KEY_KP_4 || key == GLFW_KEY_KP_6 || key == GLFW_KEY_4 || key == GLFW_KEY_6 || key == GLFW_KEY_LEFT ||
        key == GLFW_KEY_RIGHT)
      return true;
  }

  // View orbit / snap: unmodified numpad 2/4/5/6/8.
  if (mods == 0)
  {
    if (key == GLFW_KEY_KP_2 || key == GLFW_KEY_KP_4 || key == GLFW_KEY_KP_5 || key == GLFW_KEY_KP_6 || key == GLFW_KEY_KP_8)
      return true;
  }

  return false;
}

bool Gui_hotkeys::set_chord(Gui_action action, Key_chord chord)
{
  const int ai = static_cast<int>(action);
  if (ai < 0 || ai >= k_count)
    return false;
  if (is_pure_modifier_key(chord.key) || chord.key == 0)
    return false;

  chord.mods = normalize_mods(chord.mods);
  if (is_reserved_chord(chord))
    return false;

  for (int i = 0; i < k_count; ++i)
  {
    if (i == ai)
      continue;
    if (m_chords[i] == chord)
      return false;
  }

  m_chords[ai] = chord;
  return true;
}

const char* Gui_hotkeys::action_id(Gui_action action)
{
  const int i = static_cast<int>(action);
  if (i < 0 || i >= k_count)
    return "";
  return c_actions[i].id;
}

const char* Gui_hotkeys::action_label(Gui_action action)
{
  const int i = static_cast<int>(action);
  if (i < 0 || i >= k_count)
    return "";
  return c_actions[i].label;
}

Key_chord Gui_hotkeys::default_chord(Gui_action action)
{
  const int i = static_cast<int>(action);
  if (i < 0 || i >= k_count)
    return {};
  return c_actions[i].def;
}

std::optional<Gui_action> Gui_hotkeys::action_from_id(std::string_view id)
{
  for (const Action_meta& m : c_actions)
    if (id == m.id)
      return m.action;
  return std::nullopt;
}

int Gui_hotkeys::normalize_mods(int mods) { return mods & k_chord_mods_mask; }

std::string Gui_hotkeys::format_chord(Key_chord chord)
{
  chord.mods = normalize_mods(chord.mods);
  std::string out;
  if (chord.mods & GLFW_MOD_CONTROL)
    out += "Ctrl+";
  if (chord.mods & GLFW_MOD_SHIFT)
    out += "Shift+";
  if (chord.mods & GLFW_MOD_ALT)
    out += "Alt+";
  if (chord.mods & GLFW_MOD_SUPER)
    out += "Super+";

  out += key_token(chord.key);
  return out;
}

std::optional<Key_chord> Gui_hotkeys::parse_chord(std::string_view text)
{
  if (text.empty())
    return std::nullopt;

  int         mods = 0;
  std::string key_part;
  size_t      start = 0;
  while (start < text.size())
  {
    size_t           plus = text.find('+', start);
    std::string_view part = plus == std::string_view::npos ? text.substr(start) : text.substr(start, plus - start);
    // Trim spaces
    while (!part.empty() && part.front() == ' ')
      part.remove_prefix(1);
    while (!part.empty() && part.back() == ' ')
      part.remove_suffix(1);

    if (part.empty())
      return std::nullopt;

    auto is_mod = [&](const char* lit)
    {
      const std::string_view l(lit);
      if (part.size() != l.size())
        return false;
      for (size_t i = 0; i < part.size(); ++i)
        if (std::tolower(static_cast<unsigned char>(part[i])) != std::tolower(static_cast<unsigned char>(l[i])))
          return false;
      return true;
    };

    const bool last = (plus == std::string_view::npos);
    if (!last)
    {
      if (is_mod("Ctrl") || is_mod("Control"))
        mods |= GLFW_MOD_CONTROL;
      else if (is_mod("Shift"))
        mods |= GLFW_MOD_SHIFT;
      else if (is_mod("Alt"))
        mods |= GLFW_MOD_ALT;
      else if (is_mod("Super") || is_mod("Cmd") || is_mod("Meta"))
        mods |= GLFW_MOD_SUPER;
      else
        return std::nullopt;
      start = plus + 1;
      continue;
    }

    key_part.assign(part.begin(), part.end());
    break;
  }

  if (key_part.empty())
    return std::nullopt;

  const std::optional<int> key = parse_key_token(key_part);
  if (!key)
    return std::nullopt;

  return Key_chord{*key, normalize_mods(mods)};
}

nlohmann::json Gui_hotkeys::to_json() const
{
  nlohmann::json j = nlohmann::json::object();
  for (int i = 0; i < k_count; ++i)
    j[c_actions[i].id] = format_chord(m_chords[i]);
  return j;
}

void Gui_hotkeys::merge_from_json(const nlohmann::json& obj)
{
  if (!obj.is_object())
    return;

  for (auto it = obj.begin(); it != obj.end(); ++it)
  {
    if (!it.value().is_string())
      continue;
    const std::optional<Gui_action> act = action_from_id(it.key());
    if (!act)
      continue;
    const std::optional<Key_chord> chord = parse_chord(it.value().get<std::string>());
    if (!chord || is_reserved_chord(*chord))
      continue;
    // Apply without conflict reject against other remaps still loading; rebuild unique at end.
    m_chords[static_cast<int>(*act)] = *chord;
  }

  // Resolve conflicts until unique. Prefer keeping the lower Gui_action index when the later
  // row can move to its factory chord. If that factory chord is already held by an earlier
  // remap (e.g. both mode.move and mode.rotate on R), restore the earlier remapped row to
  // its factory chord too. Factory defaults are pairwise unique, so this always terminates
  // without leaving a silent duplicate that would make action_for ignore higher actions.
  bool changed = true;
  while (changed)
  {
    changed = false;
    for (int i = 0; i < k_count; ++i)
    {
      for (int j = 0; j < i; ++j)
      {
        if (m_chords[i] != m_chords[j])
          continue;
        if (m_chords[i] != c_actions[i].def)
        {
          m_chords[i] = c_actions[i].def;
          changed = true;
        }
        else if (m_chords[j] != c_actions[j].def)
        {
          m_chords[j] = c_actions[j].def;
          changed = true;
        }
        break;
      }
    }
  }
}
