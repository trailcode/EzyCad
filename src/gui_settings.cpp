// Settings dialog, JSON load/save, and OCCT view appearance from ezycad_settings.json.

#include <algorithm>
#include <array>
#include <nlohmann/json.hpp>
#include <string>

#include "utl_dbg.h"
#include "gui.h"
#include "imgui.h"
#include "gui_occt_view.h"
#include "utl_settings.h"
#include "skt.h"
#include "skt_nodes.h"

namespace
{
const char* const k_settings_version                  = "1";
const char* const k_gui_key_permanent_node_anno_scale = "permanent_node_anno_scale";

nlohmann::json build_occt_view_settings_object_(const Occt_view& view);

nlohmann::json imgui_style_to_json(const Gui_imgui_style_settings& s)
{
  return nlohmann::json{{"rounding_general", s.rounding_general}, {"rounding_scroll", s.rounding_scroll},
                        {"rounding_tabs", s.rounding_tabs},       {"window_alpha", s.window_alpha},
                        {"window_border", s.window_border},       {"frame_border", s.frame_border},
                        {"window_padding_x", s.window_padding_x}, {"window_padding_y", s.window_padding_y},
                        {"frame_padding_x", s.frame_padding_x},   {"frame_padding_y", s.frame_padding_y},
                        {"item_spacing_x", s.item_spacing_x},     {"item_spacing_y", s.item_spacing_y}};
}

nlohmann::json settings_headers_to_json(const Gui_settings_headers& h)
{
  return nlohmann::json{{"view_nav", h.view_nav},
                        {"ui", h.ui},
                        {"view_presentation", h.view_presentation},
                        {"grid", h.grid},
                        {"sketch", h.sketch},
                        {"sketch_appearance", h.sketch_appearance},
                        {"sketch_dimensions", h.sketch_dimensions},
                        {"sketch_nodes", h.sketch_nodes},
                        {"sketch_snap", h.sketch_snap},
                        {"sketch_underlay", h.sketch_underlay},
                        {"startup", h.startup}};
}

void parse_settings_headers_json(const nlohmann::json& obj, Gui_settings_headers& out)
{
  Gui_settings_headers defaults{};
  auto                 b = [&obj](const char* key, bool fallback) -> bool
  {
    if (obj.contains(key) && obj[key].is_boolean())
      return obj[key].get<bool>();
    return fallback;
  };
  out.view_nav          = b("view_nav", defaults.view_nav);
  out.ui                = b("ui", defaults.ui);
  out.view_presentation = b("view_presentation", defaults.view_presentation);
  out.grid              = b("grid", defaults.grid);
  out.sketch            = b("sketch", defaults.sketch);
  out.sketch_appearance = b("sketch_appearance", defaults.sketch_appearance);
  out.sketch_dimensions = b("sketch_dimensions", defaults.sketch_dimensions);
  out.sketch_nodes      = b("sketch_nodes", defaults.sketch_nodes);
  out.sketch_snap       = b("sketch_snap", defaults.sketch_snap);
  out.sketch_underlay   = b("sketch_underlay", defaults.sketch_underlay);
  out.startup           = b("startup", defaults.startup);
}

void parse_imgui_style_json(const nlohmann::json& obj, const Gui_imgui_style_settings& defaults, Gui_imgui_style_settings& out)
{
  auto f = [&obj](const char* key, float fallback) -> float
  {
    if (obj.contains(key) && obj[key].is_number())
    {
      const float v = obj[key].get<float>();
      if (v >= 0.f && v <= 32.f)
        return v;
    }

    return fallback;
  };

  out.rounding_general = f("rounding_general", defaults.rounding_general);
  out.rounding_scroll  = f("rounding_scroll", defaults.rounding_scroll);
  out.rounding_tabs    = f("rounding_tabs", defaults.rounding_tabs);
  out.window_alpha =
      std::clamp(f("window_alpha", defaults.window_alpha), k_gui_imgui_window_alpha_min, k_gui_imgui_window_alpha_max);
  out.window_border    = std::clamp(f("window_border", defaults.window_border), 0.f, k_gui_imgui_border_slider_max);
  out.frame_border     = std::clamp(f("frame_border", defaults.frame_border), 0.f, k_gui_imgui_border_slider_max);
  out.window_padding_x = std::clamp(f("window_padding_x", defaults.window_padding_x), 0.f, k_gui_imgui_padding_slider_max);
  out.window_padding_y = std::clamp(f("window_padding_y", defaults.window_padding_y), 0.f, k_gui_imgui_padding_slider_max);
  out.frame_padding_x  = std::clamp(f("frame_padding_x", defaults.frame_padding_x), 0.f, k_gui_imgui_padding_slider_max);
  out.frame_padding_y  = std::clamp(f("frame_padding_y", defaults.frame_padding_y), 0.f, k_gui_imgui_padding_slider_max);
  out.item_spacing_x   = std::clamp(f("item_spacing_x", defaults.item_spacing_x), 0.f, k_gui_imgui_spacing_slider_max);
  out.item_spacing_y   = std::clamp(f("item_spacing_y", defaults.item_spacing_y), 0.f, k_gui_imgui_spacing_slider_max);
}

bool settings_imgui_style_controls_(float label_col_w, Gui_imgui_style_settings& style, const char* id_prefix)
{
  bool changed = false;
  if (!ImGui::BeginTable(id_prefix, 2, ImGuiTableFlags_SizingStretchProp))
    return false;

  ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, label_col_w);
  ImGui::TableSetupColumn("control", ImGuiTableColumnFlags_WidthStretch);

  auto section = [&](const char* title)
  {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::SeparatorText(title);
  };

  auto slider = [&](const char* label, const char* id, float* v, float v_min, float v_max, const char* fmt)
  {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    changed |= ImGui::SliderFloat(id, v, v_min, v_max, fmt);
  };

  section("Transparency");
  slider("Window transparency", "##win_alpha", &style.window_alpha, k_gui_imgui_window_alpha_min, k_gui_imgui_window_alpha_max,
         "%.2f");

  section("Rounding");
  slider("Windows, frames, popups", "##round_gen", &style.rounding_general, 0.f, k_gui_imgui_rounding_slider_max, "%.0f");
  slider("Scrollbars and sliders", "##round_scr", &style.rounding_scroll, 0.f, k_gui_imgui_rounding_slider_max, "%.0f");
  slider("Tabs", "##round_tabs", &style.rounding_tabs, 0.f, k_gui_imgui_rounding_slider_max, "%.0f");

  section("Borders");
  slider("Window border", "##win_border", &style.window_border, 0.f, k_gui_imgui_border_slider_max, "%.1f");
  slider("Frame border", "##frame_border", &style.frame_border, 0.f, k_gui_imgui_border_slider_max, "%.1f");

  section("Padding");
  slider("Window padding X", "##win_pad_x", &style.window_padding_x, 0.f, k_gui_imgui_padding_slider_max, "%.0f");
  slider("Window padding Y", "##win_pad_y", &style.window_padding_y, 0.f, k_gui_imgui_padding_slider_max, "%.0f");
  slider("Frame padding X", "##frame_pad_x", &style.frame_padding_x, 0.f, k_gui_imgui_padding_slider_max, "%.0f");
  slider("Frame padding Y", "##frame_pad_y", &style.frame_padding_y, 0.f, k_gui_imgui_padding_slider_max, "%.0f");

  section("Spacing");
  slider("Item spacing X", "##item_sp_x", &style.item_spacing_x, 0.f, k_gui_imgui_spacing_slider_max, "%.0f");
  slider("Item spacing Y", "##item_sp_y", &style.item_spacing_y, 0.f, k_gui_imgui_spacing_slider_max, "%.0f");

  ImGui::EndTable();
  return changed;
}
} // namespace

void GUI::set_ui_verbosity(int v) { m_ui_verbosity = std::max(k_gui_ui_verbosity_min, v); }

std::string GUI::occt_view_settings_json() const
{
  using nlohmann::json;
  EZY_ASSERT(m_view);
  json j;
  j["occt_view"] = build_occt_view_settings_object_(*m_view);
  j["gui"]       = {
      {"edge_dim_label_h", m_edge_dim_label_h},
      {"edge_dim_line_width", m_edge_dim_line_width},
      {"edge_dim_arrow_size", m_edge_dim_arrow_size},
      {"edge_dim_color", {m_edge_dim_color[0], m_edge_dim_color[1], m_edge_dim_color[2]}},
      {"edge_dim_text_scale", m_edge_dim_text_scale},
      {"edge_dim_text_render_mode", m_edge_dim_text_render_mode},
      {"edge_dim_arrow_style", m_edge_dim_arrow_style},
      {"edge_dim_arrow_orientation", m_edge_dim_arrow_orientation},
      {"show_sketch_dimensions", m_show_sketch_dimensions},
      {k_gui_key_permanent_node_anno_scale, m_permanent_node_anno_scale},
      {"origin_marker_color", {m_origin_marker_color[0], m_origin_marker_color[1], m_origin_marker_color[2]}},
      {"sketch_edge_color", {m_sketch_edge_color[0], m_sketch_edge_color[1], m_sketch_edge_color[2], m_sketch_edge_color[3]}},
      {"sketch_edge_selection_color",
             {m_sketch_edge_selection_color[0], m_sketch_edge_selection_color[1], m_sketch_edge_selection_color[2],
              m_sketch_edge_selection_color[3]}},
      {"sketch_edge_highlight_color",
             {m_sketch_edge_highlight_color[0], m_sketch_edge_highlight_color[1], m_sketch_edge_highlight_color[2],
              m_sketch_edge_highlight_color[3]}},
      {"sketch_edge_line_width", m_sketch_edge_line_width},
      {"sketch_face_color", {m_sketch_face_color[0], m_sketch_face_color[1], m_sketch_face_color[2], m_sketch_face_color[3]}},
      {"sketch_face_selection_color",
             {m_sketch_face_selection_color[0], m_sketch_face_selection_color[1], m_sketch_face_selection_color[2],
              m_sketch_face_selection_color[3]}},
      {"sketch_face_highlight_color",
             {m_sketch_face_highlight_color[0], m_sketch_face_highlight_color[1], m_sketch_face_highlight_color[2],
              m_sketch_face_highlight_color[3]}},
      {"add_mid_pt_edges", m_add_mid_pt_line_edges},
      {"add_mid_pt_rect_edges", m_add_mid_pt_rect_edges},
      {"add_mid_pt_slot_edges", m_add_mid_pt_slot_edges},
      {"view_roll_step_deg", m_view_roll_step_deg},
      {"view_zoom_scroll_scale", m_view_zoom_scroll_scale},
      {"default_2d_view_width", m_default_2d_view_width},
      {"default_2d_view_height", m_default_2d_view_height},
      {"inspection_orthographic", m_inspection_orthographic},
      {"snap_guide_color_node",
             [&]()
             {
         float r{}, g{}, b{};
         Sketch_nodes::get_snap_guide_color_node(r, g, b);
         return nlohmann::json::array({r, g, b});
       }()},
      {"snap_guide_color_axis",
             [&]()
             {
         const glm::vec3 c = Sketch_nodes::get_snap_guide_color_axis();
         return nlohmann::json::array({c.x, c.y, c.z});
       }()},
      {"snap_guide_mode", static_cast<int>(Sketch_nodes::get_snap_guide_mode())},
      {"snap_guide_line_width", Sketch_nodes::get_snap_guide_line_width()},
      {"annotate_all_coaxial_nodes", Sketch_nodes::get_annotate_all_coaxial_nodes()},
      {"ui_verbosity", m_ui_verbosity},
      {"elm_list_hover_color",
       {m_elm_list_hover_color[0], m_elm_list_hover_color[1], m_elm_list_hover_color[2], m_elm_list_hover_color[3]}},
      {"shape_selection_color",
       {m_shape_selection_color[0], m_shape_selection_color[1], m_shape_selection_color[2], m_shape_selection_color[3]}},
      {"sketch_shape_faint_style", m_sketch_shape_faint_style},
      {"sketch_shape_faint_opacity", m_sketch_shape_faint_opacity},
      {"sketch_shape_faint_enabled", m_sketch_shape_faint_enabled},
  };
  return j.dump(2);
}

void GUI::save_occt_view_settings()
{
  std::string content = settings::load_with_defaults();
  using namespace nlohmann;
  json j;
  if (!content.empty())
  {
    try
    {
      j = json::parse(content);
    }
    catch (...)
    {
    }
  }

  EZY_ASSERT(m_view);
  j["occt_view"] = build_occt_view_settings_object_(*m_view);
  j["gui"]       = {
      {"show_options", m_show_options},
      {"show_sketch_list", m_show_sketch_list},
      {"show_shape_list", m_show_shape_list},
      {"log_window_visible", m_log_window_visible},
      {"show_settings_dialog", m_show_settings_dialog},
      {"dark_mode", m_dark_mode},
      {"show_lua_console", m_show_lua_console},
      {"show_python_console", m_show_python_console},
      {"ui_verbosity", m_ui_verbosity},
      {"edge_dim_label_h", m_edge_dim_label_h},
      {"edge_dim_line_width", m_edge_dim_line_width},
      {"edge_dim_arrow_size", m_edge_dim_arrow_size},
      {"edge_dim_color", {m_edge_dim_color[0], m_edge_dim_color[1], m_edge_dim_color[2]}},
      {"edge_dim_text_scale", m_edge_dim_text_scale},
      {"edge_dim_text_render_mode", m_edge_dim_text_render_mode},
      {"edge_dim_arrow_style", m_edge_dim_arrow_style},
      {"edge_dim_arrow_orientation", m_edge_dim_arrow_orientation},
      {"show_sketch_dimensions", m_show_sketch_dimensions},
      {k_gui_key_permanent_node_anno_scale, m_permanent_node_anno_scale},
      {"origin_marker_color", {m_origin_marker_color[0], m_origin_marker_color[1], m_origin_marker_color[2]}},
      {"sketch_edge_color", {m_sketch_edge_color[0], m_sketch_edge_color[1], m_sketch_edge_color[2], m_sketch_edge_color[3]}},
      {"sketch_edge_selection_color",
             {m_sketch_edge_selection_color[0], m_sketch_edge_selection_color[1], m_sketch_edge_selection_color[2],
              m_sketch_edge_selection_color[3]}},
      {"sketch_edge_highlight_color",
             {m_sketch_edge_highlight_color[0], m_sketch_edge_highlight_color[1], m_sketch_edge_highlight_color[2],
              m_sketch_edge_highlight_color[3]}},
      {"sketch_edge_line_width", m_sketch_edge_line_width},
      {"sketch_face_color", {m_sketch_face_color[0], m_sketch_face_color[1], m_sketch_face_color[2], m_sketch_face_color[3]}},
      {"sketch_face_selection_color",
             {m_sketch_face_selection_color[0], m_sketch_face_selection_color[1], m_sketch_face_selection_color[2],
              m_sketch_face_selection_color[3]}},
      {"sketch_face_highlight_color",
             {m_sketch_face_highlight_color[0], m_sketch_face_highlight_color[1], m_sketch_face_highlight_color[2],
              m_sketch_face_highlight_color[3]}},
      {"add_mid_pt_edges", m_add_mid_pt_line_edges},
      {"add_mid_pt_rect_edges", m_add_mid_pt_rect_edges},
      {"add_mid_pt_slot_edges", m_add_mid_pt_slot_edges},
      {"load_last_opened_on_startup", m_load_last_opened_on_startup},
      {"last_opened_project_path", m_last_opened_project_path},
      {"imgui_style_dark", imgui_style_to_json(m_imgui_style_dark)},
      {"imgui_style_light", imgui_style_to_json(m_imgui_style_light)},
      {"settings_headers", settings_headers_to_json(m_settings_headers)},
      {"view_roll_step_deg", m_view_roll_step_deg},
      {"view_zoom_scroll_scale", m_view_zoom_scroll_scale},
      {"default_2d_view_width", m_default_2d_view_width},
      {"default_2d_view_height", m_default_2d_view_height},
      {"inspection_orthographic", m_inspection_orthographic},
      {"snap_guide_color_node",
             [&]()
             {
         float r{}, g{}, b{};
         Sketch_nodes::get_snap_guide_color_node(r, g, b);
         return nlohmann::json::array({r, g, b});
       }()},
      {"snap_guide_color_axis",
             [&]()
             {
         const glm::vec3 c = Sketch_nodes::get_snap_guide_color_axis();
         return nlohmann::json::array({c.x, c.y, c.z});
       }()},
      {"snap_guide_mode", static_cast<int>(Sketch_nodes::get_snap_guide_mode())},
      {"snap_guide_line_width", Sketch_nodes::get_snap_guide_line_width()},
      {"annotate_all_coaxial_nodes", Sketch_nodes::get_annotate_all_coaxial_nodes()},
#ifndef NDEBUG
      {"show_dbg", m_show_dbg},
#endif
      {"underlay_highlight_color",
       {m_underlay_highlight_color[0], m_underlay_highlight_color[1], m_underlay_highlight_color[2],
        m_underlay_highlight_color[3]}},
      {"elm_list_hover_color",
       {m_elm_list_hover_color[0], m_elm_list_hover_color[1], m_elm_list_hover_color[2], m_elm_list_hover_color[3]}},
      {"shape_selection_color",
       {m_shape_selection_color[0], m_shape_selection_color[1], m_shape_selection_color[2], m_shape_selection_color[3]}},
      {"sketch_shape_faint_style", m_sketch_shape_faint_style},
      {"sketch_shape_faint_opacity", m_sketch_shape_faint_opacity},
      {"sketch_shape_faint_enabled", m_sketch_shape_faint_enabled},
  };
  j["version"]          = k_settings_version;
  const char* imgui_ini = ImGui::SaveIniSettingsToMemory(nullptr);
  if (imgui_ini && *imgui_ini)
    j["imgui_ini"] = std::string(imgui_ini);

  settings::save(j.dump(2));
  log_message("Settings: saved.");
}

void GUI::parse_occt_view_settings_(const std::string& content)
{
  try
  {
    using namespace nlohmann;
    const json j = json::parse(content);
    if (!j.contains("occt_view") || !j["occt_view"].is_object())
      return;

    const json& ov     = j["occt_view"];
    float       bg1[3] = {0.037552f, 0.040503f, 0.042471f};
    float       bg2[3] = {0.043440f, 0.174068f, 0.239382f};
    int         method = 1;
    float       g1[3]  = {0.112683f, 0.056886f, 0.138996f};
    float       g2[3]  = {0.117917f, 0.117917f, 0.135135f};
    auto        arr3   = [](const json& a, float* out)
    {
      if (a.is_array() && a.size() >= 3)
        for (size_t i = 0; i < 3; ++i)
          if (a[i].is_number())
            out[i] = a[i].get<float>();
    };
    if (ov.contains("bg_color1"))
      arr3(ov["bg_color1"], bg1);

    if (ov.contains("bg_color2"))
      arr3(ov["bg_color2"], bg2);

    if (ov.contains("bg_gradient_method") && ov["bg_gradient_method"].is_number_integer())
      method = ov["bg_gradient_method"].get<int>();

    if (ov.contains("grid_color1"))
      arr3(ov["grid_color1"], g1);

    if (ov.contains("grid_color2"))
      arr3(ov["grid_color2"], g2);

    m_view->set_bg_gradient_colors(bg1[0], bg1[1], bg1[2], bg2[0], bg2[1], bg2[2]);
    m_view->set_bg_gradient_method(method);
    m_view->set_grid_colors(g1[0], g1[1], g1[2], g2[0], g2[1], g2[2]);

    Occt_grid_rect_params grid_rect{};
    m_view->get_occt_grid_rect_params(grid_rect);
    auto apply_num = [&ov](const char* key, double& dst)
    {
      if (!ov.contains(key))
        return;

      const json& v = ov[key];
      if (v.is_number())
        dst = v.get<double>();
    };
    apply_num("grid_step", grid_rect.step);
    if (!ov.contains("grid_step"))
    {
      if (ov.contains("grid_x_step"))
        apply_num("grid_x_step", grid_rect.step);

      else if (ov.contains("grid_y_step"))
        apply_num("grid_y_step", grid_rect.step);
    }
    apply_num("grid_padding", grid_rect.grid_padding);
    if (!ov.contains("grid_padding") && ov.contains("grid_graphic_x_size"))
      // Legacy: treat old half-extent as padding margin around sketch content.
      apply_num("grid_graphic_x_size", grid_rect.grid_padding);

    apply_num("grid_graphic_z_offset", grid_rect.graphic_z_offset);
    m_view->set_occt_grid_rect_params(grid_rect);

    bool grid_visible = m_view->get_grid_visible();
    if (ov.contains("grid_visible") && ov["grid_visible"].is_boolean())
      grid_visible = ov["grid_visible"].get<bool>();

    // Always apply: default m_grid_visible may already match JSON, but the shader grid
    // is not displayed until apply_grid_visibility_() runs.
    m_view->set_grid_visible(grid_visible);
  }
  catch (...)
  {
    // TODO: log error
  }
}

void GUI::parse_gui_panes_settings_(const std::string& content)
{
  try
  {
    using namespace nlohmann;
    const json j = json::parse(content);
    if (!j.contains("gui") || !j["gui"].is_object())
      return;

    const json& g = j["gui"];
    auto        b = [&g](const char* key, bool current)
    {
      if (!g.contains(key))
        return current;

      const json& v = g[key];
      if (v.is_boolean())
        return v.get<bool>();

      if (v.is_number_integer())
        return v.get<int>() != 0;

      return current;
    };

    m_ui_verbosity = k_gui_ui_verbosity_default;
    if (g.contains("ui_verbosity") && g["ui_verbosity"].is_number_integer())
      m_ui_verbosity = std::max(k_gui_ui_verbosity_min, g["ui_verbosity"].get<int>());

    set_show_options(b("show_options", true));
    set_show_sketch_list(b("show_sketch_list", true));
    set_show_shape_list(b("show_shape_list", true));
    set_log_window_visible(b("log_window_visible", true));
    set_show_settings_dialog(b("show_settings_dialog", false));
    m_dark_mode           = b("dark_mode", m_dark_mode);
    m_show_lua_console    = b("show_lua_console", false);
    m_show_python_console = b("show_python_console", false);
    if (g.contains("edge_dim_label_h") && g["edge_dim_label_h"].is_number_integer())
    {
      const int v = g["edge_dim_label_h"].get<int>();
      if (v >= 0 && v <= 3)
        m_edge_dim_label_h = v;
    }

    auto parse_bounded_float = [&g](const char* key, const float min_v, const float max_v, const float default_v) -> float
    {
      float out = default_v;
      if (g.contains(key) && g[key].is_number())
      {
        const float v = g[key].get<float>();
        if (v >= min_v && v <= max_v)
          out = v;
      }

      return out;
    };
    m_edge_dim_line_width = parse_bounded_float("edge_dim_line_width", 0.5f, 8.0f, k_gui_edge_dim_line_width_default);
    m_edge_dim_arrow_size = parse_bounded_float("edge_dim_arrow_size", 1.0f, 24.0f, k_gui_edge_dim_arrow_size_default);
    m_edge_dim_text_scale = parse_bounded_float("edge_dim_text_scale", k_gui_edge_dim_text_scale_min,
                                                k_gui_edge_dim_text_scale_max, k_gui_edge_dim_text_scale_default);
    auto parse_dim_int    = [&g](const char* key, const int min_v, const int max_v, const int default_v) -> int
    {
      if (g.contains(key) && g[key].is_number_integer())
      {
        const int v = g[key].get<int>();
        if (v >= min_v && v <= max_v)
          return v;
      }

      return default_v;
    };
    m_edge_dim_text_render_mode = parse_dim_int("edge_dim_text_render_mode", 0, k_gui_edge_dim_text_render_mode_max,
                                                k_gui_edge_dim_text_render_mode_default);
    if (g.contains("edge_dim_color") && g["edge_dim_color"].is_array() && g["edge_dim_color"].size() >= 3)
    {
      const json& a = g["edge_dim_color"];
      for (int i = 0; i < 3; ++i)
        if (a[i].is_number())
          m_edge_dim_color[i] = std::clamp(a[i].get<float>(), 0.f, 1.f);
    }

    if (g.contains("origin_marker_color") && g["origin_marker_color"].is_array() && g["origin_marker_color"].size() >= 3)
    {
      const json& a = g["origin_marker_color"];
      for (int i = 0; i < 3; ++i)
        if (a[i].is_number())
          m_origin_marker_color[i] = std::clamp(a[i].get<float>(), 0.f, 1.f);
    }

    auto parse_rgba4 = [&g](const char* key, float* out, const float* defaults)
    {
      for (int i = 0; i < 4; ++i)
        out[i] = defaults[i];
      if (!g.contains(key) || !g[key].is_array())
        return;
      const json& a = g[key];
      for (int i = 0; i < 4; ++i)
        if (i < static_cast<int>(a.size()) && a[i].is_number())
          out[i] = std::clamp(a[i].get<float>(), 0.f, 1.f);
    };
    parse_rgba4("sketch_edge_color", m_sketch_edge_color, k_gui_sketch_edge_color_default);
    parse_rgba4("sketch_edge_highlight_color", m_sketch_edge_highlight_color, k_gui_sketch_edge_highlight_color_default);
    parse_rgba4("sketch_face_color", m_sketch_face_color, k_gui_sketch_face_color_default);
    parse_rgba4("sketch_face_highlight_color", m_sketch_face_highlight_color, k_gui_sketch_face_highlight_color_default);
    // Selection colors: fall back to highlight when absent (pre-split settings files).
    parse_rgba4("sketch_edge_selection_color", m_sketch_edge_selection_color,
                g.contains("sketch_edge_selection_color") ? k_gui_sketch_edge_selection_color_default
                                                          : m_sketch_edge_highlight_color);
    parse_rgba4("sketch_face_selection_color", m_sketch_face_selection_color,
                g.contains("sketch_face_selection_color") ? k_gui_sketch_face_selection_color_default
                                                          : m_sketch_face_highlight_color);

    m_sketch_edge_line_width = parse_bounded_float("sketch_edge_line_width", k_gui_sketch_edge_line_width_min,
                                                   k_gui_sketch_edge_line_width_max, k_gui_sketch_edge_line_width_default);

    parse_rgba4("shape_selection_color", m_shape_selection_color, k_gui_shape_selection_color_default);

    if (g.contains("sketch_shape_faint_style") && g["sketch_shape_faint_style"].is_number_integer())
    {
      const int v = g["sketch_shape_faint_style"].get<int>();
      if (v >= k_gui_sketch_shape_faint_style_min && v <= k_gui_sketch_shape_faint_style_max)
        m_sketch_shape_faint_style = v;
    }

    m_sketch_shape_faint_opacity =
        parse_bounded_float("sketch_shape_faint_opacity", k_gui_sketch_shape_faint_opacity_min,
                            k_gui_sketch_shape_faint_opacity_max, k_gui_sketch_shape_faint_opacity_default);
    m_sketch_shape_faint_enabled = b("sketch_shape_faint_enabled", k_gui_sketch_shape_faint_enabled_default);

    if (g.contains("edge_dim_arrow_style") && g["edge_dim_arrow_style"].is_number_integer())
    {
      const int v = g["edge_dim_arrow_style"].get<int>();
      if (v >= k_gui_edge_dim_arrow_style_min && v <= k_gui_edge_dim_arrow_style_max)
        m_edge_dim_arrow_style = v;
    }

    if (g.contains("edge_dim_arrow_orientation") && g["edge_dim_arrow_orientation"].is_number_integer())
    {
      const int v = g["edge_dim_arrow_orientation"].get<int>();
      if (v >= k_gui_edge_dim_arrow_orientation_min && v <= k_gui_edge_dim_arrow_orientation_max)
        m_edge_dim_arrow_orientation = v;
    }

    m_show_sketch_dimensions = b("show_sketch_dimensions", true);
    m_permanent_node_anno_scale =
        parse_bounded_float(k_gui_key_permanent_node_anno_scale, k_gui_permanent_node_anno_scale_min,
                            k_gui_permanent_node_anno_scale_max, k_gui_permanent_node_anno_scale_default);

    m_add_mid_pt_line_edges = b("add_mid_pt_edges", false);
    if (g.contains("add_midpoints_to_linear_edges") && !g.contains("add_mid_pt_edges") &&
        g["add_midpoints_to_linear_edges"].is_boolean())
      m_add_mid_pt_line_edges = g["add_midpoints_to_linear_edges"].get<bool>();

    m_add_mid_pt_rect_edges       = b("add_mid_pt_rect_edges", true);
    m_add_mid_pt_slot_edges       = b("add_mid_pt_slot_edges", false);
    m_load_last_opened_on_startup = b("load_last_opened_on_startup", b("load_last_saved_on_startup", false));

    if (g.contains("last_opened_project_path") && g["last_opened_project_path"].is_string())
      m_last_opened_project_path = g["last_opened_project_path"].get<std::string>();
    else if (g.contains("last_saved_project_path") && g["last_saved_project_path"].is_string())
      m_last_opened_project_path = g["last_saved_project_path"].get<std::string>();

    Gui_imgui_style_settings dark_defaults{};
    Gui_imgui_style_settings light_defaults{};
    imgui_style_defaults_from_theme_(true, dark_defaults);
    imgui_style_defaults_from_theme_(false, light_defaults);
    m_imgui_style_dark  = dark_defaults;
    m_imgui_style_light = light_defaults;

    if (g.contains("imgui_style_dark") && g["imgui_style_dark"].is_object())
      parse_imgui_style_json(g["imgui_style_dark"], dark_defaults, m_imgui_style_dark);

    if (g.contains("imgui_style_light") && g["imgui_style_light"].is_object())
      parse_imgui_style_json(g["imgui_style_light"], light_defaults, m_imgui_style_light);

    m_settings_headers = Gui_settings_headers{};
    if (g.contains("settings_headers") && g["settings_headers"].is_object())
      parse_settings_headers_json(g["settings_headers"], m_settings_headers);

    const bool has_nested_imgui_style = (g.contains("imgui_style_dark") && g["imgui_style_dark"].is_object()) ||
                                        (g.contains("imgui_style_light") && g["imgui_style_light"].is_object());
    if (!has_nested_imgui_style)
    {
      auto round_from_json = [&g](const char* key, float fallback) -> float
      {
        if (g.contains(key) && g[key].is_number())
        {
          const float v = g[key].get<float>();
          if (v >= 0.f && v <= 32.f)
            return v;
        }
        return fallback;
      };

      const bool has_legacy_rounding =
          g.contains("imgui_rounding_general") || g.contains("imgui_rounding_scroll") || g.contains("imgui_rounding_tabs");
      if (has_legacy_rounding)
      {
        const float gen                     = round_from_json("imgui_rounding_general", dark_defaults.rounding_general);
        const float scroll                  = round_from_json("imgui_rounding_scroll", dark_defaults.rounding_scroll);
        const float tabs                    = round_from_json("imgui_rounding_tabs", dark_defaults.rounding_tabs);
        m_imgui_style_dark.rounding_general = m_imgui_style_light.rounding_general = gen;
        m_imgui_style_dark.rounding_scroll = m_imgui_style_light.rounding_scroll = scroll;
        m_imgui_style_dark.rounding_tabs = m_imgui_style_light.rounding_tabs = tabs;
      }
    }

    m_view_roll_step_deg = k_gui_view_roll_step_deg_default;
    if (g.contains("view_roll_step_deg") && g["view_roll_step_deg"].is_number())
    {
      const double v = g["view_roll_step_deg"].get<double>();
      if (v >= k_gui_view_roll_step_deg_min && v <= k_gui_view_roll_step_deg_max)
        m_view_roll_step_deg = v;
      else
        log_message("EzyCad: settings gui.view_roll_step_deg out of range [" + std::to_string(k_gui_view_roll_step_deg_min) +
                    ", " + std::to_string(k_gui_view_roll_step_deg_max) + "], got " + std::to_string(v) + "; using default.");
    }

    m_inspection_orthographic = b("inspection_orthographic", false);
    if (m_view)
      m_view->apply_camera_projection();

    m_view_zoom_scroll_scale = k_gui_view_zoom_scroll_scale_default;
    if (g.contains("view_zoom_scroll_scale") && g["view_zoom_scroll_scale"].is_number())
    {
      const double v = g["view_zoom_scroll_scale"].get<double>();
      if (v >= k_gui_view_zoom_scroll_scale_min && v <= k_gui_view_zoom_scroll_scale_max)
        m_view_zoom_scroll_scale = v;
      else
        log_message("EzyCad: settings gui.view_zoom_scroll_scale out of range [" +
                    std::to_string(k_gui_view_zoom_scroll_scale_min) + ", " + std::to_string(k_gui_view_zoom_scroll_scale_max) +
                    "], got " + std::to_string(v) + "; using default.");
    }

    if (m_view)
      m_view->set_zoom_scroll_scale(m_view_zoom_scroll_scale);

    m_default_2d_view_width = k_gui_default_2d_view_size_default;
    if (g.contains("default_2d_view_width") && g["default_2d_view_width"].is_number())
    {
      const double v = g["default_2d_view_width"].get<double>();
      if (v >= k_gui_default_2d_view_size_min && v <= k_gui_default_2d_view_size_max)
        m_default_2d_view_width = v;
      else
        log_message("EzyCad: settings gui.default_2d_view_width out of range [" +
                    std::to_string(k_gui_default_2d_view_size_min) + ", " + std::to_string(k_gui_default_2d_view_size_max) +
                    "], got " + std::to_string(v) + "; using default.");
    }

    m_default_2d_view_height = k_gui_default_2d_view_size_default;
    if (g.contains("default_2d_view_height") && g["default_2d_view_height"].is_number())
    {
      const double v = g["default_2d_view_height"].get<double>();
      if (v >= k_gui_default_2d_view_size_min && v <= k_gui_default_2d_view_size_max)
        m_default_2d_view_height = v;
      else
        log_message("EzyCad: settings gui.default_2d_view_height out of range [" +
                    std::to_string(k_gui_default_2d_view_size_min) + ", " + std::to_string(k_gui_default_2d_view_size_max) +
                    "], got " + std::to_string(v) + "; using default.");
    }

    Sketch_nodes::set_snap_guide_color_node(0.823295f, 0.549411f, 0.953390f);
    Sketch_nodes::set_snap_guide_color_axis(0.957627f, 0.064924f, 0.541537f);
    if (g.contains("snap_guide_color_node") && g["snap_guide_color_node"].is_array() && g["snap_guide_color_node"].size() >= 3)
    {
      const json& a = g["snap_guide_color_node"];
      glm::vec3   c(0.823295f, 0.549411f, 0.953390f);
      for (size_t i = 0; i < 3; ++i)
        if (a[static_cast<json::size_type>(i)].is_number())
          c[static_cast<glm::vec3::length_type>(i)] = std::clamp(a[static_cast<json::size_type>(i)].get<float>(), 0.f, 1.f);
      Sketch_nodes::set_snap_guide_color_node(c[0], c[1], c[2]);
    }
    else if (g.contains("snap_guide_color") && g["snap_guide_color"].is_array() && g["snap_guide_color"].size() >= 3)
    {
      const json& a = g["snap_guide_color"];
      glm::vec3   c(0.823295f, 0.549411f, 0.953390f);
      for (size_t i = 0; i < 3; ++i)
        if (a[static_cast<json::size_type>(i)].is_number())
          c[static_cast<glm::vec3::length_type>(i)] = std::clamp(a[static_cast<json::size_type>(i)].get<float>(), 0.f, 1.f);

      Sketch_nodes::set_snap_guide_color_node(c[0], c[1], c[2]);
      Sketch_nodes::set_snap_guide_color_axis(c[0], c[1], c[2]);
    }

    if (g.contains("snap_guide_color_axis") && g["snap_guide_color_axis"].is_array() && g["snap_guide_color_axis"].size() >= 3)
    {
      const json& a = g["snap_guide_color_axis"];
      glm::vec3   c(0.957627f, 0.064924f, 0.541537f);
      for (size_t i = 0; i < 3; ++i)
        if (a[static_cast<json::size_type>(i)].is_number())
          c[static_cast<glm::vec3::length_type>(i)] = std::clamp(a[static_cast<json::size_type>(i)].get<float>(), 0.f, 1.f);

      Sketch_nodes::set_snap_guide_color_axis(c[0], c[1], c[2]);
    }
    else if (g.contains("snap_guide_color_node") && g["snap_guide_color_node"].is_array() &&
             g["snap_guide_color_node"].size() >= 3)
    {
      float r{}, g_c{}, b{};
      Sketch_nodes::get_snap_guide_color_node(r, g_c, b);
      Sketch_nodes::set_snap_guide_color_axis(r, g_c, b);
    }

    Sketch_nodes::set_snap_guide_mode(Sketch_nodes::Snap_guide_mode::Both);
    if (g.contains("snap_guide_mode") && g["snap_guide_mode"].is_number_integer())
    {
      const int mode = g["snap_guide_mode"].get<int>();
      if (mode >= static_cast<int>(Sketch_nodes::Snap_guide_mode::Traditional) &&
          mode <= static_cast<int>(Sketch_nodes::Snap_guide_mode::None))
        Sketch_nodes::set_snap_guide_mode(static_cast<Sketch_nodes::Snap_guide_mode>(mode));
    }

    Sketch_nodes::set_snap_guide_line_width(1.0f);
    if (g.contains("snap_guide_line_width") && g["snap_guide_line_width"].is_number())
      Sketch_nodes::set_snap_guide_line_width(g["snap_guide_line_width"].get<float>());

    Sketch_nodes::set_annotate_all_coaxial_nodes(true);
    if (g.contains("annotate_all_coaxial_nodes") && g["annotate_all_coaxial_nodes"].is_boolean())
      Sketch_nodes::set_annotate_all_coaxial_nodes(g["annotate_all_coaxial_nodes"].get<bool>());

    if (g.contains("underlay_highlight_color") && g["underlay_highlight_color"].is_array() &&
        g["underlay_highlight_color"].size() >= 3)
    {
      const json& a = g["underlay_highlight_color"];
      for (int i = 0; i < 3; ++i)
        if (a[static_cast<json::size_type>(i)].is_number())
          m_underlay_highlight_color[static_cast<glm::vec4::length_type>(i)] =
              std::clamp(a[static_cast<json::size_type>(i)].get<float>(), 0.f, 1.f);

      m_underlay_highlight_color[3] = 1.f;
      if (a.size() >= 4 && a[3].is_number())
        m_underlay_highlight_color[3] = std::clamp(a[3].get<float>(), 0.f, 1.f);
    }

    if (g.contains("elm_list_hover_color") && g["elm_list_hover_color"].is_array() && g["elm_list_hover_color"].size() >= 3)
    {
      const json& a = g["elm_list_hover_color"];
      for (int i = 0; i < 3; ++i)
        if (a[static_cast<json::size_type>(i)].is_number())
          m_elm_list_hover_color[static_cast<glm::vec4::length_type>(i)] =
              std::clamp(a[static_cast<json::size_type>(i)].get<float>(), 0.f, 1.f);

      m_elm_list_hover_color[3] = 1.f;
      if (a.size() >= 4 && a[3].is_number())
        m_elm_list_hover_color[3] = std::clamp(a[3].get<float>(), 0.f, 1.f);
    }

#ifndef NDEBUG
    set_show_dbg(b("show_dbg", false));
#endif
  }
  catch (...)
  {
    EZY_ASSERT_MSG(false, "Error parse_gui_panes_settings!");
  }
}

void GUI::load_occt_view_settings_()
{
  std::string content = settings::load_with_defaults();

  try
  {
    using namespace nlohmann;
    const json j                        = json::parse(content);
    auto       settings_version_matches = [](const json& doc) -> bool
    {
      if (!doc.contains("version"))
        return false;

      const json& v = doc["version"];
      if (v.is_string())
        return v.get<std::string>() == k_settings_version;

      if (v.is_number_integer())
        return std::to_string(v.get<long long>()) == k_settings_version;

      return false;
    };
    const bool version_ok = settings_version_matches(j);
    if (!version_ok)
    {
      content = settings::load_defaults();
      if (!content.empty())
      {
        try
        {
          json j_default       = json::parse(content);
          j_default["version"] = k_settings_version;
          settings::save(j_default.dump(2));
          content = j_default.dump(2);
        }
        catch (...)
        {
          settings::save(content);
        }
      }
      log_message("Settings version mismatch or missing; loaded defaults.");
    }
  }
  catch (...)
  {
    content = settings::load_defaults();
    if (!content.empty())
      settings::save(content);
  }

  EZY_ASSERT_MSG(!content.empty(), "Settings content empty!");

  parse_occt_view_settings_(content);
  parse_gui_panes_settings_(content);
  if (m_view)
  {
    apply_sketch_dimensions_visibility();
    m_view->apply_shape_selection_style();
    m_view->sync_sketch_shape_faint_style();
  }

  sync_sketch_add_mid_pt_edges_if_applicable_();

  try
  {
    using namespace nlohmann;
    const json j = json::parse(content);
    if (j.contains("imgui_ini") && j["imgui_ini"].is_string())
    {
      const std::string& ini = j["imgui_ini"].get<std::string>();
      if (!ini.empty())
      {
        ImGui::LoadIniSettingsFromMemory(ini.c_str(), ini.size());
        m_seed_default_dock_layout = (ini.find("[Docking]") == std::string::npos);
      }
    }
  }
  catch (...)
  {
    // EZY_ASSERT_MSG(false, "Settings invalid!");
  }

  log_message("Settings: loaded.");
}

bool GUI::settings_collapsing_header_(const char* label, bool& open_state)
{
  ImGui::SetNextItemOpen(open_state);
  const bool open = ImGui::CollapsingHeader(label);
  if (open != open_state)
  {
    open_state = open;
    save_occt_view_settings();
  }
  return open;
}

void GUI::settings_()
{
  if (!m_show_settings_dialog)
    return;

  ImGui::SetNextWindowSize(ImVec2(520, 0), ImGuiCond_FirstUseEver); // Auto height; width matches res defaults
  if (!ImGui::Begin("Settings", &m_show_settings_dialog, ImGuiWindowFlags_None))
  {
    ImGui::End();
    return;
  }

  constexpr float k_label_col_w = 230.f;

  {
    bool verb_changed = false;
    if (ImGui::BeginTable("settings_ui_verbosity", 2, ImGuiTableFlags_SizingStretchProp))
    {
      ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, k_label_col_w);
      ImGui::TableSetupColumn("control", ImGuiTableColumnFlags_WidthStretch);
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("UI verbosity");
      ImGui::TableSetColumnIndex(1);
      ImGui::BeginDisabled(m_ui_verbosity <= k_gui_ui_verbosity_min);
      if (ImGui::ArrowButton("##ui_verbosity_dec", ImGuiDir_Left))
      {
        --m_ui_verbosity;
        verb_changed = true;
      }

      ImGui::EndDisabled();
      ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
      ImGui::SetNextItemWidth(56.0f);
      int verb_edit = m_ui_verbosity;
      if (ImGui::InputInt("##ui_verbosity_val", &verb_edit, 0, 0))
      {
        m_ui_verbosity = std::max(k_gui_ui_verbosity_min, verb_edit);
        verb_changed   = true;
      }

      ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
      if (ImGui::ArrowButton("##ui_verbosity_inc", ImGuiDir_Right))
      {
        ++m_ui_verbosity;
        verb_changed = true;
      }

      ImGui::EndTable();
    }
    if (verb_changed)
      save_occt_view_settings();
  }

  if (ui_show_contextual_help())
    ImGui::TextWrapped("0 = minimal UI. Odd values add more controls and panes; even values add more help (tooltips and "
                       "hints). Higher values are reserved for future tiers.");

  if (settings_collapsing_header_("3D view navigation", m_settings_headers.view_nav))
  {
    if (ImGui::BeginTable("settings_view_nav", 2, ImGuiTableFlags_SizingStretchProp))
    {
      ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, k_label_col_w);
      ImGui::TableSetupColumn("control", ImGuiTableColumnFlags_WidthStretch);

      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("View rotation step");
      ImGui::TableSetColumnIndex(1);
      // SliderScalar(ImGuiDataType_Double): drag slider, or Ctrl+click for precise keyboard input (standard ImGui).
      if (ImGui::SliderScalar("##view_roll_step", ImGuiDataType_Double, &m_view_roll_step_deg, &k_gui_view_roll_step_deg_min,
                              &k_gui_view_roll_step_deg_max, "%.2f deg",
                              ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_ClampOnInput))
        save_occt_view_settings();

      m_view_roll_step_deg = std::clamp(m_view_roll_step_deg, k_gui_view_roll_step_deg_min, k_gui_view_roll_step_deg_max);

      ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
      GUI_DOC_HELP_("Degrees per key press: NumPad 8/2/4/6 orbit (like LMB drag), Shift+NumPad 4/6, Shift+4/6, or "
                    "Shift+Left/Right roll. Ctrl+click the slider to type a value. Click ? to open the user guide.",
                    doc_urls::k_view_roll);

      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("Zoom scroll scale");
      ImGui::TableSetColumnIndex(1);
      if (ImGui::SliderScalar("##view_zoom_scroll_scale", ImGuiDataType_Double, &m_view_zoom_scroll_scale,
                              &k_gui_view_zoom_scroll_scale_min, &k_gui_view_zoom_scroll_scale_max, "%.2f",
                              ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_ClampOnInput))
      {
        m_view_zoom_scroll_scale =
            std::clamp(m_view_zoom_scroll_scale, k_gui_view_zoom_scroll_scale_min, k_gui_view_zoom_scroll_scale_max);
        if (m_view)
          m_view->set_zoom_scroll_scale(m_view_zoom_scroll_scale);

        save_occt_view_settings();
      }

      m_view_zoom_scroll_scale =
          std::clamp(m_view_zoom_scroll_scale, k_gui_view_zoom_scroll_scale_min, k_gui_view_zoom_scroll_scale_max);

      ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
      GUI_DOC_HELP_("Multiplier for mouse wheel and +/- zoom (same as UpdateZoom scroll delta). Hold Shift while "
                    "zooming for Blender-style finer steps (x0.1). Ctrl+click to type a value. Click ? to open the "
                    "user guide.",
                    doc_urls::k_view_navigation);

      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("Default 2D view width");
      ImGui::TableSetColumnIndex(1);
      if (ImGui::SliderScalar("##default_2d_view_width", ImGuiDataType_Double, &m_default_2d_view_width,
                              &k_gui_default_2d_view_size_min, &k_gui_default_2d_view_size_max, "%.2f",
                              ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_ClampOnInput))
        save_occt_view_settings();
      m_default_2d_view_width =
          std::clamp(m_default_2d_view_width, k_gui_default_2d_view_size_min, k_gui_default_2d_view_size_max);
      ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
      GUI_DOC_HELP_("Horizontal span of the sketch plane framed by File -> New and by projects with no saved camera "
                    "(same length scale as sketch dimensions). Default 3. Ctrl+click to type. Click ? for the guide.",
                    doc_urls::k_view_navigation);

      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("Default 2D view height");
      ImGui::TableSetColumnIndex(1);
      if (ImGui::SliderScalar("##default_2d_view_height", ImGuiDataType_Double, &m_default_2d_view_height,
                              &k_gui_default_2d_view_size_min, &k_gui_default_2d_view_size_max, "%.2f",
                              ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_ClampOnInput))
        save_occt_view_settings();
      m_default_2d_view_height =
          std::clamp(m_default_2d_view_height, k_gui_default_2d_view_size_min, k_gui_default_2d_view_size_max);
      ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
      GUI_DOC_HELP_("Vertical span of the sketch plane framed by File -> New and by projects with no saved camera "
                    "(same length scale as sketch dimensions). Default 3. Ctrl+click to type. Click ? for the guide.",
                    doc_urls::k_view_navigation);

      ImGui::EndTable();
    }

    if (ui_show_contextual_help())
      ImGui::TextWrapped(
          "NumPad 8 / 2 / 4 / 6 orbit the view (same axes as left-drag orbit). Hold Shift and press NumPad 4 or NumPad 6, "
          "main 4 / 6, or Left / Right arrow for Blender-style roll around the screen Z axis (hold to repeat). "
          "Num Lock off is recommended for numpad shortcuts (see usage.md View navigation). "
          "Hold Shift while scrolling or pressing +/- for finer zoom. "
          "Default 2D view width/height set the top-view framing for File -> New.");
  }

  if (settings_collapsing_header_("UI", m_settings_headers.ui))
  {
    bool ui_changed = false;
    if (ImGui::Checkbox("Dark mode", &m_dark_mode))
      ui_changed = true;

    ui_changed |= settings_imgui_style_controls_(k_label_col_w, imgui_style_active_(), "settings_ui");

    if (ui_changed)
      save_occt_view_settings();
  }

  if (settings_collapsing_header_("View presentation", m_settings_headers.view_presentation))
  {
    float bg1[3], bg2[3];
    m_view->get_bg_gradient_colors(bg1, bg2);
    bool bg_changed = false;
    if (ImGui::BeginTable("settings_bg", 2, ImGuiTableFlags_SizingStretchProp))
    {
      ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, k_label_col_w);
      ImGui::TableSetupColumn("control", ImGuiTableColumnFlags_WidthStretch);

      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("Background color 1");
      ImGui::TableSetColumnIndex(1);
      if (ImGui::ColorEdit3("##bg1", bg1, ImGuiColorEditFlags_Float))
        bg_changed = true;

      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("Background color 2");
      ImGui::TableSetColumnIndex(1);
      if (ImGui::ColorEdit3("##bg2", bg2, ImGuiColorEditFlags_Float))
        bg_changed = true;

      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("Gradient blend");
      ImGui::TableSetColumnIndex(1);
      const char* gradient_items[] = {"Horizontal", "Vertical", "Diagonal 1", "Diagonal 2",
                                      "Corner 1",   "Corner 2", "Corner 3",   "Corner 4"};
      int         grad             = m_view->get_bg_gradient_method();
      if (ImGui::Combo("##bg_grad", &grad, gradient_items, 8))
      {
        m_view->set_bg_gradient_method(grad);
        save_occt_view_settings();
      }

      ImGui::EndTable();
    }

    if (bg_changed)
    {
      m_view->set_bg_gradient_colors(bg1[0], bg1[1], bg1[2], bg2[0], bg2[1], bg2[2]);
      save_occt_view_settings();
    }

    bool element_hover_changed = false;
    if (ImGui::BeginTable("settings_element_hover", 2, ImGuiTableFlags_SizingStretchProp))
    {
      ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, k_label_col_w);
      ImGui::TableSetupColumn("control", ImGuiTableColumnFlags_WidthStretch);
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("Element hover color");
      ImGui::TableSetColumnIndex(1);
      element_hover_changed |= ImGui::ColorEdit4("##element_hover", &m_elm_list_hover_color[0], ImGuiColorEditFlags_Float);
      ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
      GUI_DOC_HELP_("Highlight color when hovering a row in the Shape List or a dimension row in the Sketch List. "
                    "Updates immediately when a row is hovered.",
                    doc_urls::k_occt_view);
      ImGui::EndTable();
    }

    if (element_hover_changed)
    {
      m_view->refresh_shape_list_hover_highlight();
      save_occt_view_settings();
    }

    bool shape_sel_changed = false;
    if (ImGui::BeginTable("settings_shape_selection", 2, ImGuiTableFlags_SizingStretchProp))
    {
      ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, k_label_col_w);
      ImGui::TableSetupColumn("control", ImGuiTableColumnFlags_WidthStretch);

      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("Shape selection color");
      ImGui::TableSetColumnIndex(1);
      if (ImGui::ColorEdit4("##shape_sel_color", m_shape_selection_color,
                            ImGuiColorEditFlags_Float | ImGuiColorEditFlags_AlphaBar))
        shape_sel_changed = true;
      ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
      GUI_DOC_HELP_("Highlight color for selected 3D shapes in the viewer (edges and wires).", doc_urls::k_occt_view);

      ImGui::EndTable();
    }

    if (shape_sel_changed)
    {
      m_view->apply_shape_selection_style();
      save_occt_view_settings();
    }
  }

  if (ui_show_feature(3) && settings_collapsing_header_("3D view grid", m_settings_headers.grid))
  {
    float g1[3], g2[3];
    m_view->get_grid_colors(g1, g2);
    Occt_grid_rect_params gr{};
    m_view->get_occt_grid_rect_params(gr);
    const double dim_scale = m_view->get_dimension_scale();
    // Settings show the same length units as sketch dimensions (model / dimension_scale).
    double          step_ui          = gr.step / dim_scale;
    double          padding_ui       = gr.grid_padding / dim_scale;
    double          graphic_z_off_ui = gr.graphic_z_offset / dim_scale;
    bool            grid_changed     = false;
    bool            geom_changed     = false;
    constexpr float spd_s            = 0.08f;
    constexpr float spd_m            = 1.5f;
    constexpr float spd_extent       = 0.25f;
    if (ImGui::BeginTable("settings_grid", 2, ImGuiTableFlags_SizingStretchProp))
    {
      ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, k_label_col_w);
      ImGui::TableSetupColumn("control", ImGuiTableColumnFlags_WidthStretch);

      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("Show grid");
      ImGui::TableSetColumnIndex(1);
      bool grid_visible = m_view->get_grid_visible();
      if (ImGui::Checkbox("##grid_visible", &grid_visible))
      {
        m_view->set_grid_visible(grid_visible);
        save_occt_view_settings();
      }
      ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
      GUI_DOC_HELP_("Show or hide the OCCT reference grid in the 3D view. When shown, the grid lies on the active "
                    "sketch plane. Click ? to open the user guide.",
                    doc_urls::k_occt_view);

      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("Grid color 1");
      ImGui::TableSetColumnIndex(1);
      if (ImGui::ColorEdit3("##g1", g1, ImGuiColorEditFlags_Float))
        grid_changed = true;

      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("Grid color 2");
      ImGui::TableSetColumnIndex(1);
      if (ImGui::ColorEdit3("##g2", g2, ImGuiColorEditFlags_Float))
        grid_changed = true;

      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("Grid step");
      ImGui::TableSetColumnIndex(1);
      // Step must stay positive; grid extent is derived from sketch bounds + padding.
      const double step_min_ui = 1e-6;
      if (ImGui::DragScalar("##gstep", ImGuiDataType_Double, &step_ui, spd_s, &step_min_ui, nullptr, "%.8g"))
        geom_changed = true;

      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("Grid padding");
      ImGui::TableSetColumnIndex(1);
      const double padding_min_ui = 0.0;
      if (ImGui::DragScalar("##gpad", ImGuiDataType_Double, &padding_ui, spd_extent, &padding_min_ui, nullptr, "%.8g"))
        geom_changed = true;

      ImGui::SameLine(0.f, ImGui::GetStyle().ItemInnerSpacing.x);
      GUI_DOC_HELP_("Margin added around the active sketch when sizing the grid (same length scale as sketch "
                    "dimensions). The grid extent follows sketch geometry plus this padding. Click ? to open the user "
                    "guide.",
                    doc_urls::k_occt_view);

      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("Grid display Z offset");
      ImGui::TableSetColumnIndex(1);
      if (ImGui::DragScalar("##ggz", ImGuiDataType_Double, &graphic_z_off_ui, spd_m, nullptr, nullptr, "%.8g"))
        geom_changed = true;

      ImGui::EndTable();
    }
    if (grid_changed)
      m_view->set_grid_colors(g1[0], g1[1], g1[2], g2[0], g2[1], g2[2]);

    if (geom_changed)
    {
      gr.step             = step_ui * dim_scale;
      gr.grid_padding     = padding_ui * dim_scale;
      gr.graphic_z_offset = graphic_z_off_ui * dim_scale;
      m_view->set_occt_grid_rect_params(gr);
      m_view->get_occt_grid_rect_params(gr);
      step_ui          = gr.step / dim_scale;
      padding_ui       = gr.grid_padding / dim_scale;
      graphic_z_off_ui = gr.graphic_z_offset / dim_scale;
    }

    if (grid_changed || geom_changed)
      save_occt_view_settings();
  }

  if (settings_collapsing_header_("Sketch", m_settings_headers.sketch))
  {
    bool ul_changed        = false;
    bool dim_changed       = false;
    bool node_anno_changed = false;
    bool appear_changed    = false;

    ImGui::Indent(ImGui::GetStyle().IndentSpacing);
    if (settings_collapsing_header_("Appearance", m_settings_headers.sketch_appearance))
      if (ImGui::BeginTable("settings_sketch_appear", 2, ImGuiTableFlags_SizingStretchProp))
      {
        ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, k_label_col_w);
        ImGui::TableSetupColumn("control", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Edge color");
        ImGui::TableSetColumnIndex(1);
        if (ImGui::ColorEdit4("##sketch_edge_color", m_sketch_edge_color,
                              ImGuiColorEditFlags_Float | ImGuiColorEditFlags_AlphaBar))
          appear_changed = true;
        ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
        GUI_DOC_HELP_("Color and opacity of edges on the current sketch (alpha = opacity).", nullptr);

        if (ui_show_occt_line_width_settings())
        {
          ImGui::TableNextRow();
          ImGui::TableSetColumnIndex(0);
          ImGui::AlignTextToFramePadding();
          ImGui::TextUnformatted("Edge thickness");
          ImGui::TableSetColumnIndex(1);
          {
            float lw = m_sketch_edge_line_width;
            if (ImGui::SliderFloat("##sketch_edge_lw", &lw, k_gui_sketch_edge_line_width_min, k_gui_sketch_edge_line_width_max,
                                   "%.2f"))
            {
              m_sketch_edge_line_width = lw;
              appear_changed           = true;
            }
          }
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Edge selection color");
        ImGui::TableSetColumnIndex(1);
        if (ImGui::ColorEdit4("##sketch_edge_sel_color", m_sketch_edge_selection_color,
                              ImGuiColorEditFlags_Float | ImGuiColorEditFlags_AlphaBar))
          appear_changed = true;
        ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
        GUI_DOC_HELP_("Color and opacity for selected sketch edges.", nullptr);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Edge highlight color");
        ImGui::TableSetColumnIndex(1);
        if (ImGui::ColorEdit4("##sketch_edge_hl_color", m_sketch_edge_highlight_color,
                              ImGuiColorEditFlags_Float | ImGuiColorEditFlags_AlphaBar))
          appear_changed = true;
        ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
        GUI_DOC_HELP_("Color and opacity for mouse-over (dynamic) highlight on sketch edges.", nullptr);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Face fill color");
        ImGui::TableSetColumnIndex(1);
        if (ImGui::ColorEdit4("##sketch_face_color", m_sketch_face_color,
                              ImGuiColorEditFlags_Float | ImGuiColorEditFlags_AlphaBar))
          appear_changed = true;
        ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
        GUI_DOC_HELP_("Fill color and opacity of closed faces on the current sketch.", nullptr);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Face selection fill");
        ImGui::TableSetColumnIndex(1);
        if (ImGui::ColorEdit4("##sketch_face_sel_color", m_sketch_face_selection_color,
                              ImGuiColorEditFlags_Float | ImGuiColorEditFlags_AlphaBar))
          appear_changed = true;
        ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
        GUI_DOC_HELP_("Fill color and opacity for selected sketch faces.", nullptr);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Face highlight fill");
        ImGui::TableSetColumnIndex(1);
        if (ImGui::ColorEdit4("##sketch_face_hl_color", m_sketch_face_highlight_color,
                              ImGuiColorEditFlags_Float | ImGuiColorEditFlags_AlphaBar))
          appear_changed = true;
        ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
        GUI_DOC_HELP_("Fill color and opacity for mouse-over (dynamic) highlight on sketch faces.", nullptr);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Shapes in sketch mode");
        ImGui::TableSetColumnIndex(1);
        {
          bool faint = m_sketch_shape_faint_enabled;
          if (ImGui::Checkbox("##sketch_shape_faint_enabled_settings", &faint))
          {
            m_sketch_shape_faint_enabled = faint;
            appear_changed               = true;
          }
          ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
          ImGui::TextUnformatted("Enabled");
        }
        ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
        GUI_DOC_HELP_("Master switch also in Options -> Sketch options (all sketch tools). When off, solids are "
                      "hidden in sketch modes.",
                      doc_urls::k_sketching_2d);

        if (m_sketch_shape_faint_enabled)
        {
          ImGui::TableNextRow();
          ImGui::TableSetColumnIndex(0);
          ImGui::AlignTextToFramePadding();
          ImGui::TextUnformatted("Shape style");
          ImGui::TableSetColumnIndex(1);
          {
            static const char* faint_items[] = {"Off (hide)", "Ghost", "Wire"};
            int                style         = m_sketch_shape_faint_style;
            if (ImGui::Combo("##sketch_shape_faint_style", &style, faint_items, IM_ARRAYSIZE(faint_items)))
            {
              m_sketch_shape_faint_style = style;
              appear_changed             = true;
            }
          }
          ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
          GUI_DOC_HELP_("How 3D shapes appear while sketch tools are active when faint shapes are enabled: hide, "
                        "semi-transparent ghost, or wireframe. Shape List Hide all still hides everything.",
                        doc_urls::k_sketching_2d);

          // Strength applies to Ghost and Wire (not Off/hide).
          if (m_sketch_shape_faint_style == 1 || m_sketch_shape_faint_style == 2)
          {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Shape Faint Strength");
            ImGui::TableSetColumnIndex(1);
            {
              float strength_pct = m_sketch_shape_faint_opacity * 100.0f;
              if (ImGui::SliderFloat("##sketch_shape_faint_strength", &strength_pct,
                                     k_gui_sketch_shape_faint_opacity_min * 100.0f,
                                     k_gui_sketch_shape_faint_opacity_max * 100.0f, "%.0f%%"))
              {
                m_sketch_shape_faint_opacity =
                    std::clamp(strength_pct / 100.0f, k_gui_sketch_shape_faint_opacity_min, k_gui_sketch_shape_faint_opacity_max);
                appear_changed = true;
              }
            }
            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
            GUI_DOC_HELP_("How solid faint shapes look in sketch mode (higher = closer to normal inspection "
                          "opacity). Applies to Ghost and Wire styles.",
                          doc_urls::k_sketching_2d);
          }
        }

        ImGui::EndTable();
      }

    if (settings_collapsing_header_("Dimensions", m_settings_headers.sketch_dimensions))
      if (ImGui::BeginTable("settings_sketch_dims", 2, ImGuiTableFlags_SizingStretchProp))
      {
        ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, k_label_col_w);
        ImGui::TableSetupColumn("control", ImGuiTableColumnFlags_WidthStretch);

        if (ui_show_occt_line_width_settings())
        {
          ImGui::TableNextRow();
          ImGui::TableSetColumnIndex(0);
          ImGui::AlignTextToFramePadding();
          ImGui::TextUnformatted("Dimension line width");
          ImGui::TableSetColumnIndex(1);
          {
            float lw = m_edge_dim_line_width;
            if (ImGui::SliderFloat("##edge_dim_lw", &lw, 0.5f, 8.0f, "%.2f"))
            {
              m_edge_dim_line_width = lw;
              dim_changed           = true;
            }

            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
            GUI_DOC_HELP_("Thickness of sketch edge length dimensions (Open CASCADE line width scale; 1.0 = default).",
                          nullptr);
          }
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Dimension arrow size");
        ImGui::TableSetColumnIndex(1);
        {
          float arrow = m_edge_dim_arrow_size;
          if (ImGui::SliderFloat("##edge_dim_arrow", &arrow, 1.0f, 24.0f, "%.2f"))
          {
            m_edge_dim_arrow_size = arrow;
            dim_changed           = true;
          }

          ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
          GUI_DOC_HELP_("Arrow head length for sketch and extrude length dimensions (Open CASCADE display units).", nullptr);
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Dimension color");
        ImGui::TableSetColumnIndex(1);
        if (ImGui::ColorEdit3("##edge_dim_color", m_edge_dim_color, ImGuiColorEditFlags_Float))
          dim_changed = true;

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Dimension text scale");
        ImGui::TableSetColumnIndex(1);
        {
          float ts = m_edge_dim_text_scale;
          if (ImGui::SliderFloat("##edge_dim_text_scale", &ts, k_gui_edge_dim_text_scale_min, k_gui_edge_dim_text_scale_max,
                                 "%.2f"))
          {
            m_edge_dim_text_scale = ts;
            dim_changed           = true;
          }
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Label rendering");
        ImGui::TableSetColumnIndex(1);
        {
          constexpr std::array<const char*, 6> k_labels = {
              "Opaque 2D text", "SetCommonColor", "2D screen text", "3D text", "Z-layer Top", "Z-layer Topmost",
          };
          int rm = m_edge_dim_text_render_mode;
          if (rm < 0 || rm >= static_cast<int>(k_labels.size()))
            rm = k_gui_edge_dim_text_render_mode_default;

          ImGui::SetNextItemWidth(220.0f);
          if (ImGui::BeginCombo("##edge_dim_text_render", k_labels[static_cast<size_t>(rm)], ImGuiComboFlags_HeightSmall))
          {
            for (int i = 0; i < static_cast<int>(k_labels.size()); ++i)
              if (ImGui::Selectable(k_labels[static_cast<size_t>(i)], i == rm))
              {
                m_edge_dim_text_render_mode = i;
                dim_changed                 = true;
              }

            ImGui::EndCombo();
          }

          ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
          GUI_DOC_HELP_("How dimension value labels are composited. Z-layer Top and Topmost avoid ghosting against the "
                        "grid; Topmost is the default.",
                        nullptr);
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Length value placement");
        ImGui::TableSetColumnIndex(1);
        {
          constexpr std::array<const char*, 4> k_edge_dim_label_placement = {
              "Near first point",
              "Near second point",
              "Center on dimension line",
              "Automatic",
          };
          int h = m_edge_dim_label_h;
          ImGui::SetNextItemWidth(200.0f);
          if (ImGui::BeginCombo("##edge_dim_h", k_edge_dim_label_placement[static_cast<size_t>(h)],
                                ImGuiComboFlags_HeightSmall))
          {
            for (int i = 0; i < static_cast<int>(k_edge_dim_label_placement.size()); ++i)
              if (ImGui::Selectable(k_edge_dim_label_placement[static_cast<size_t>(i)], i == h))
              {
                m_edge_dim_label_h = i;
                dim_changed        = true;
              }
            ImGui::EndCombo();
          }
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Arrow style");
        ImGui::TableSetColumnIndex(1);
        {
          constexpr std::array<const char*, 4> k_arrow_styles = {"Standard", "Sharp", "Wide", "3D shaded"};
          int                                  st             = m_edge_dim_arrow_style;
          ImGui::SetNextItemWidth(160.0f);
          if (ImGui::BeginCombo("##edge_dim_arrow_style", k_arrow_styles[static_cast<size_t>(st)], ImGuiComboFlags_HeightSmall))
          {
            for (int i = 0; i < static_cast<int>(k_arrow_styles.size()); ++i)
              if (ImGui::Selectable(k_arrow_styles[static_cast<size_t>(i)], i == st))
              {
                m_edge_dim_arrow_style = i;
                dim_changed            = true;
              }

            ImGui::EndCombo();
          }
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Arrow orientation");
        ImGui::TableSetColumnIndex(1);
        {
          constexpr std::array<const char*, 3> k_arrow_orient = {"Automatic", "Internal", "External"};
          int                                  ao             = m_edge_dim_arrow_orientation;
          ImGui::SetNextItemWidth(160.0f);
          if (ImGui::BeginCombo("##edge_dim_arrow_orient", k_arrow_orient[static_cast<size_t>(ao)],
                                ImGuiComboFlags_HeightSmall))
          {
            for (int i = 0; i < static_cast<int>(k_arrow_orient.size()); ++i)
              if (ImGui::Selectable(k_arrow_orient[static_cast<size_t>(i)], i == ao))
              {
                m_edge_dim_arrow_orientation = i;
                dim_changed                  = true;
              }

            ImGui::EndCombo();
          }
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Show sketch dimensions");
        ImGui::TableSetColumnIndex(1);
        {
          bool show = m_show_sketch_dimensions;
          if (ImGui::Checkbox("##show_sketch_dims", &show))
          {
            set_show_sketch_dimensions(show);
            save_occt_view_settings();
          }

          ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
          GUI_DOC_HELP_("When off, hides all sketch length dimensions. Tool mode may still limit which sketch shows "
                        "dimensions when this is on.",
                        nullptr);
        }

        ImGui::EndTable();
      }

    if (settings_collapsing_header_("Nodes", m_settings_headers.sketch_nodes))
      if (ImGui::BeginTable("settings_sketch_nodes", 2, ImGuiTableFlags_SizingStretchProp))
      {
        ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, k_label_col_w);
        ImGui::TableSetupColumn("control", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Permanent node annotation size");
        ImGui::TableSetColumnIndex(1);
        {
          float size_scale = m_permanent_node_anno_scale;
          if (ImGui::SliderFloat("##permanent_node_anno_scale", &size_scale, k_gui_permanent_node_anno_scale_min,
                                 k_gui_permanent_node_anno_scale_max, "%.2f"))
          {
            m_permanent_node_anno_scale = size_scale;
            node_anno_changed           = true;
          }

          ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
          GUI_DOC_HELP_("Scale for '+' markers on the sketch Origin and Add node points. Click ? to open the user guide.",
                        doc_urls::k_sketch_origin);
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Origin marker color");
        ImGui::TableSetColumnIndex(1);
        {
          if (ImGui::ColorEdit3("##origin_marker_color", m_origin_marker_color, ImGuiColorEditFlags_Float))
            node_anno_changed = true;

          ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
          GUI_DOC_HELP_("Color for the active sketch's Origin marker (+ with circle). Click ? to open the user guide.",
                        doc_urls::k_sketch_origin);
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Add midpoints to line edges");
        ImGui::TableSetColumnIndex(1);
        {
          bool add = m_add_mid_pt_line_edges;
          if (ImGui::Checkbox("##add_mids_line", &add))
          {
            m_add_mid_pt_line_edges = add;
            sync_sketch_add_mid_pt_edges_if_applicable_();
            save_occt_view_settings();
          }

          ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
          GUI_DOC_HELP_("Line edge and multi-line edge tools (default off). Click ? to open the user guide.",
                        doc_urls::k_line_edge_midpoint_nodes);
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Add midpoints to square/rectangle");
        ImGui::TableSetColumnIndex(1);
        {
          bool add = m_add_mid_pt_rect_edges;
          if (ImGui::Checkbox("##add_mids_rect", &add))
          {
            m_add_mid_pt_rect_edges = add;
            sync_sketch_add_mid_pt_edges_if_applicable_();
            save_occt_view_settings();
          }

          ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
          GUI_DOC_HELP_("Square and rectangle tools (default on). Click ? to open the user guide.",
                        doc_urls::k_line_edge_midpoint_nodes);
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Add midpoints to slot edges");
        ImGui::TableSetColumnIndex(1);
        {
          bool add = m_add_mid_pt_slot_edges;
          if (ImGui::Checkbox("##add_mids_slot", &add))
          {
            m_add_mid_pt_slot_edges = add;
            sync_sketch_add_mid_pt_edges_if_applicable_();
            save_occt_view_settings();
          }

          ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
          GUI_DOC_HELP_("Slot tool straight edges only (default off). Click ? to open the user guide.",
                        doc_urls::k_line_edge_midpoint_nodes);
        }

        ImGui::EndTable();
      }

    if (settings_collapsing_header_("Snap", m_settings_headers.sketch_snap))
      if (ImGui::BeginTable("settings_sketch_snap", 2, ImGuiTableFlags_SizingStretchProp))
      {
        ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, k_label_col_w);
        ImGui::TableSetupColumn("control", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Snap guide color (node)");
        ImGui::TableSetColumnIndex(1);
        {
          glm::vec3 snap_col;
          Sketch_nodes::get_snap_guide_color_node(snap_col[0], snap_col[1], snap_col[2]);
          if (ImGui::ColorEdit3("##snap_guide_color_node", &snap_col[0], ImGuiColorEditFlags_Float))
          {
            Sketch_nodes::set_snap_guide_color_node(snap_col[0], snap_col[1], snap_col[2]);
            save_occt_view_settings();
          }

          ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
          GUI_DOC_HELP_("Guides when both X and Y snap to the same node (vertex lock). Click ? to open the user guide.",
                        doc_urls::k_sketch_snapping);
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Snap guide color (axis)");
        ImGui::TableSetColumnIndex(1);
        {
          glm::vec3 snap_col = Sketch_nodes::get_snap_guide_color_axis();
          if (ImGui::ColorEdit3("##snap_guide_color_axis", &snap_col[0], ImGuiColorEditFlags_Float))
          {
            Sketch_nodes::set_snap_guide_color_axis(snap_col[0], snap_col[1], snap_col[2]);
            save_occt_view_settings();
          }

          ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
          GUI_DOC_HELP_("Guides when the cursor aligns to a node on X or Y only (axis snap). Click ? to open the user "
                        "guide.",
                        doc_urls::k_sketch_snapping);
        }

        if (ui_show_occt_line_width_settings())
        {
          ImGui::TableNextRow();
          ImGui::TableSetColumnIndex(0);
          ImGui::AlignTextToFramePadding();
          ImGui::TextUnformatted("Snap guide line width");
          ImGui::TableSetColumnIndex(1);
          {
            float line_width = Sketch_nodes::get_snap_guide_line_width();
            if (ImGui::SliderFloat("##snap_guide_line_width", &line_width, 0.5f, 8.0f, "%.2f"))
            {
              Sketch_nodes::set_snap_guide_line_width(line_width);
              save_occt_view_settings();
            }

            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
            GUI_DOC_HELP_("Line width for sketch snap guides (axis lines, markers, and co-axial overlay). Click ? to open "
                          "the user guide.",
                          doc_urls::k_sketch_snapping);
          }
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Snap guide mode");
        ImGui::TableSetColumnIndex(1);
        {
          constexpr std::array<const char*, 4> k_snap_guide_mode_labels = {
              "Traditional",
              "Fullscreen",
              "Both",
              "None",
          };
          int mode = static_cast<int>(Sketch_nodes::get_snap_guide_mode());
          ImGui::SetNextItemWidth(160.0f);
          if (ImGui::BeginCombo("##settings_snap_guide_mode", k_snap_guide_mode_labels[static_cast<size_t>(mode)],
                                ImGuiComboFlags_HeightSmall))
          {
            for (int i = 0; i < static_cast<int>(k_snap_guide_mode_labels.size()); ++i)
              if (ImGui::Selectable(k_snap_guide_mode_labels[static_cast<size_t>(i)], i == mode))
              {
                Sketch_nodes::set_snap_guide_mode(static_cast<Sketch_nodes::Snap_guide_mode>(i));
                save_occt_view_settings();
              }

            ImGui::EndCombo();
          }
          ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
          GUI_DOC_HELP_("Traditional: compact local snap marker.\nFullscreen: full-view crosshair/axis guides.\nBoth: show "
                        "compact marker and fullscreen guides together.\nNone: disable snap-to-node and snap guides. Click ? "
                        "to open the user guide.",
                        doc_urls::k_sketch_snapping);
        }

        if (Sketch_nodes::get_snap_guide_mode() != Sketch_nodes::Snap_guide_mode::None)
        {
          ImGui::TableNextRow();
          ImGui::TableSetColumnIndex(0);
          ImGui::AlignTextToFramePadding();
          ImGui::TextUnformatted("All co-axial nodes");
          ImGui::TableSetColumnIndex(1);
          {
            bool annotate_all = Sketch_nodes::get_annotate_all_coaxial_nodes();
            if (ImGui::Checkbox("##settings_annotate_all_coaxial", &annotate_all))
            {
              Sketch_nodes::set_annotate_all_coaxial_nodes(annotate_all);
              save_occt_view_settings();
            }

            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
            GUI_DOC_HELP_("When on (global mode): axis guide lines + markers for *all* nodes in the current sketch and all "
                          "other visible sketches. When off (default): only closest node per active axis is annotated. "
                          "Click ? to open the user guide.",
                          doc_urls::k_sketch_snapping);
          }
        }

        ImGui::EndTable();
      }

    if (settings_collapsing_header_("Underlay", m_settings_headers.sketch_underlay))
      if (ImGui::BeginTable("settings_sketch_underlay", 2, ImGuiTableFlags_SizingStretchProp))
      {
        ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, k_label_col_w);
        ImGui::TableSetupColumn("control", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Underlay highlight color");
        ImGui::TableSetColumnIndex(1);
        ul_changed |= ImGui::ColorEdit4("##underlay_hi", &m_underlay_highlight_color[0], ImGuiColorEditFlags_Float);
        ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
        GUI_DOC_HELP_("Updates all sketch underlays immediately. Also used as the default when you import a new image. "
                      "Per-sketch overrides in Sketch List if needed. Click ? to open the user guide.",
                      doc_urls::k_image_underlay);

        ImGui::EndTable();
      }

    ImGui::Unindent(ImGui::GetStyle().IndentSpacing);

    if (dim_changed)
    {
      save_occt_view_settings();
      if (m_view)
        m_view->refresh_sketch_annotations({.length_dimensions = true});
    }

    if (node_anno_changed)
    {
      save_occt_view_settings();
      if (m_view)
        m_view->refresh_sketch_annotations({.permanent_node_marks = true});
    }

    if (appear_changed)
    {
      save_occt_view_settings();
      if (m_view)
      {
        m_view->refresh_sketch_annotations({.edge_face_style = true});
        m_view->sync_sketch_shape_faint_style();
        m_view->refresh_shape_list_hover_highlight();
      }
    }

    if (ul_changed)
    {
      uint8_t hr, hg, hb, ha;
      underlay_highlight_color_rgba(hr, hg, hb, ha);
      for (const Sketch::sptr& sk : m_view->get_sketches())
      {
        EZY_ASSERT(sk);
        if (sk->underlay().has_image())
        {
          Sketch_underlay& ul = sk->underlay();
          ul.set_line_tint_rgba(hr, hg, hb, ha);
          ul.rebuild_display(sk->get_plane(), sk->is_visible());
        }
      }

      m_underlay_panel_sketch = nullptr;
      save_occt_view_settings();
    }
  }

  if (ui_show_feature(3) && settings_collapsing_header_("Startup project", m_settings_headers.startup))
  {
#ifndef __EMSCRIPTEN__
    if (ImGui::BeginTable("settings_startup_native", 2, ImGuiTableFlags_SizingStretchProp))
    {
      ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, k_label_col_w);
      ImGui::TableSetupColumn("control", ImGuiTableColumnFlags_WidthStretch);
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("Load last opened on startup");
      ImGui::TableSetColumnIndex(1);
      if (ImGui::Checkbox("##load_last", &m_load_last_opened_on_startup))
        save_occt_view_settings();

      ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
      GUI_DOC_HELP_("When enabled, EzyCad opens the last .ezy file you opened (path is stored in settings). Click ? to "
                    "open the user guide.",
                    doc_urls::k_startup_project);
      ImGui::EndTable();
    }
    if (ui_show_contextual_help() && !m_last_opened_project_path.empty())
      ImGui::TextWrapped("Last opened path: %s", m_last_opened_project_path.c_str());
    else
      ImGui::TextDisabled("(No path saved yet.)");
    ImGui::Spacing();
#endif
    if (ImGui::Button("Save current as startup project"))
      save_startup_project_();
    ImGui::SameLine();
    if (ImGui::Button("Clear saved startup"))
      clear_saved_startup_project_();

    ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
    GUI_DOC_HELP_("Save the current document (geometry, view, and tool mode) as what loads when EzyCad starts. If none is "
                  "saved, the install default (res/default.ezy) is used. Click ? to open the user guide.",
                  doc_urls::k_startup_project);
  }

  ImGui::Separator();
  if (ImGui::Button("Defaults"))
  {
    std::string content = settings::load_defaults();
    if (content.empty())
      show_message("Failed to load default settings.");
    else
    {
      try
      {
        using namespace nlohmann;
        json j       = json::parse(content);
        j["version"] = k_settings_version;
        content      = j.dump(2);
        settings::save(content);
        parse_occt_view_settings_(content);
        parse_gui_panes_settings_(content);
        if (j.contains("imgui_ini") && j["imgui_ini"].is_string())
        {
          const std::string& ini = j["imgui_ini"].get<std::string>();
          if (!ini.empty())
          {
            ImGui::LoadIniSettingsFromMemory(ini.c_str(), ini.size());
            m_seed_default_dock_layout = (ini.find("[Docking]") == std::string::npos);
          }
        }

        show_message("Default settings applied.");
        if (m_view)
        {
          m_view->refresh_sketch_annotations(
              {.length_dimensions = true, .permanent_node_marks = true, .edge_face_style = true});
          m_view->apply_shape_selection_style();
          m_view->sync_sketch_shape_faint_style();
          m_view->refresh_shape_list_hover_highlight();
        }
      }
      catch (...)
      {
        show_message("Failed to apply default settings.");
      }
    }
  }

  ImGui::End();
}

void GUI::underlay_highlight_color_rgba(uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) const
{
  const auto to_u8 = [](float c) -> uint8_t
  {
    const float x = std::clamp(c, 0.f, 1.f) * 255.f;
    return static_cast<uint8_t>(x + 0.5f);
  };

  r = to_u8(m_underlay_highlight_color[0]);
  g = to_u8(m_underlay_highlight_color[1]);
  b = to_u8(m_underlay_highlight_color[2]);
  a = to_u8(m_underlay_highlight_color[3]);
}

void GUI::elm_list_hover_color_rgba(uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) const
{
  const auto to_u8 = [](float c) -> uint8_t
  {
    const float x = std::clamp(c, 0.f, 1.f) * 255.f;
    return static_cast<uint8_t>(x + 0.5f);
  };

  r = to_u8(m_elm_list_hover_color[0]);
  g = to_u8(m_elm_list_hover_color[1]);
  b = to_u8(m_elm_list_hover_color[2]);
  a = to_u8(m_elm_list_hover_color[3]);
}

void GUI::imgui_style_defaults_from_theme_(bool dark, Gui_imgui_style_settings& out) const
{
  ImGuiStyle s;
  if (dark)
    ImGui::StyleColorsDark(&s);
  else
    ImGui::StyleColorsLight(&s);

  s.ScaleAllSizes(ImGui::GetStyle().FontScaleDpi);
  out.rounding_general = s.WindowRounding;
  out.rounding_scroll  = s.ScrollbarRounding;
  out.rounding_tabs    = s.TabRounding;
  out.window_alpha     = s.Colors[ImGuiCol_WindowBg].w;
  out.window_border    = s.WindowBorderSize;
  out.frame_border     = s.FrameBorderSize;
  out.window_padding_x = s.WindowPadding.x;
  out.window_padding_y = s.WindowPadding.y;
  out.frame_padding_x  = s.FramePadding.x;
  out.frame_padding_y  = s.FramePadding.y;
  out.item_spacing_x   = s.ItemSpacing.x;
  out.item_spacing_y   = s.ItemSpacing.y;
}

const Gui_imgui_style_settings& GUI::imgui_style_active_() const
{
  return m_dark_mode ? m_imgui_style_dark : m_imgui_style_light;
}

Gui_imgui_style_settings& GUI::imgui_style_active_() { return m_dark_mode ? m_imgui_style_dark : m_imgui_style_light; }

void GUI::apply_imgui_style_from_members_()
{
  const Gui_imgui_style_settings& style = imgui_style_active_();
  ImGuiStyle&                     st    = ImGui::GetStyle();
  st.WindowRounding                     = style.rounding_general;
  st.ChildRounding                      = style.rounding_general;
  st.FrameRounding                      = style.rounding_general;
  st.PopupRounding                      = style.rounding_general;
  st.ScrollbarRounding                  = style.rounding_scroll;
  st.GrabRounding                       = style.rounding_scroll;
  st.TabRounding                        = style.rounding_tabs;
  st.WindowBorderSize                   = style.window_border;
  st.FrameBorderSize                    = style.frame_border;
  st.WindowPadding                      = ImVec2(style.window_padding_x, style.window_padding_y);
  st.FramePadding                       = ImVec2(style.frame_padding_x, style.frame_padding_y);
  st.ItemSpacing                        = ImVec2(style.item_spacing_x, style.item_spacing_y);
  st.Colors[ImGuiCol_WindowBg].w        = style.window_alpha;
  st.Colors[ImGuiCol_PopupBg].w         = style.window_alpha;
}

namespace
{
/// `occt_view` JSON object: view background gradient and grid (shared with `save_occt_view_settings` /
/// `occt_view_settings_json`).
nlohmann::json build_occt_view_settings_object_(const Occt_view& view)
{
  float bg1[3], bg2[3], g1[3], g2[3];
  view.get_bg_gradient_colors(bg1, bg2);
  view.get_grid_colors(g1, g2);
  const int             method = view.get_bg_gradient_method();
  Occt_grid_rect_params grid_rect{};
  view.get_occt_grid_rect_params(grid_rect);
  return nlohmann::json{
      {"bg_color1", {bg1[0], bg1[1], bg1[2]}},
      {"bg_color2", {bg2[0], bg2[1], bg2[2]}},
      {"bg_gradient_method", method},
      {"grid_color1", {g1[0], g1[1], g1[2]}},
      {"grid_color2", {g2[0], g2[1], g2[2]}},
      {"grid_step", grid_rect.step},
      {"grid_padding", grid_rect.grid_padding},
      {"grid_graphic_z_offset", grid_rect.graphic_z_offset},
      {"grid_visible", view.get_grid_visible()},
  };
}
} // namespace