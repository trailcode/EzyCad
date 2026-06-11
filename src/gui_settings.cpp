// Settings dialog, JSON load/save, and OCCT view appearance from ezycad_settings.json.

#include <algorithm>
#include <array>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>

#include "dbg.h"
#include "gui.h"
#include "imgui.h"
#include "occt_view.h"
#include "settings.h"
#include "sketch.h"
#include "sketch_nodes.h"

namespace
{
const char* const k_settings_version                  = "1";
const char* const k_gui_key_permanent_node_anno_scale = "permanent_node_anno_scale";

/// `occt_view` JSON object: view background gradient and grid (shared with `save_occt_view_settings` /
/// `occt_view_settings_json`).
nlohmann::json build_occt_view_settings_object(const Occt_view& view)
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
      {"grid_graphic_x_size", grid_rect.graphic_x_size},
      {"grid_graphic_y_size", grid_rect.graphic_y_size},
      {"grid_graphic_z_offset", grid_rect.graphic_z_offset},
  };
}
} // namespace

void GUI::set_ui_verbosity(int v) { m_ui_verbosity = std::max(k_gui_ui_verbosity_min, v); }

std::string GUI::occt_view_settings_json() const
{
  using nlohmann::json;
  EZY_ASSERT(m_view);
  json j;
  j["occt_view"] = build_occt_view_settings_object(*m_view);
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
      {"view_roll_step_deg", m_view_roll_step_deg},
      {"view_zoom_scroll_scale", m_view_zoom_scroll_scale},
      {"inspection_orthographic", m_inspection_orthographic},
      {"snap_guide_color",
             [&]()
             {
         float r{}, g{}, b{};
         Sketch_nodes::get_snap_guide_color(r, g, b);
         return nlohmann::json::array({r, g, b});
       }()},
      {"snap_guide_mode", static_cast<int>(Sketch_nodes::get_snap_guide_mode())},
      {"annotate_all_coaxial_nodes", Sketch_nodes::get_annotate_all_coaxial_nodes()},
      {"ui_verbosity", m_ui_verbosity},
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
  j["occt_view"] = build_occt_view_settings_object(*m_view);
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
      {"load_last_opened_on_startup", m_load_last_opened_on_startup},
      {"last_opened_project_path", m_last_opened_project_path},
      {"imgui_rounding_general", m_imgui_rounding_general},
      {"imgui_rounding_scroll", m_imgui_rounding_scroll},
      {"imgui_rounding_tabs", m_imgui_rounding_tabs},
      {"view_roll_step_deg", m_view_roll_step_deg},
      {"view_zoom_scroll_scale", m_view_zoom_scroll_scale},
      {"inspection_orthographic", m_inspection_orthographic},
      {"snap_guide_color",
             [&]()
             {
         float r{}, g{}, b{};
         Sketch_nodes::get_snap_guide_color(r, g, b);
         return nlohmann::json::array({r, g, b});
       }()},
      {"snap_guide_mode", static_cast<int>(Sketch_nodes::get_snap_guide_mode())},
      {"annotate_all_coaxial_nodes", Sketch_nodes::get_annotate_all_coaxial_nodes()},
#ifndef NDEBUG
      {"show_dbg", m_show_dbg},
#endif
      {"underlay_highlight_color",
       {m_underlay_highlight_color[0], m_underlay_highlight_color[1], m_underlay_highlight_color[2],
        m_underlay_highlight_color[3]}},
      {"shape_list_hover_color",
       {m_shape_list_hover_color[0], m_shape_list_hover_color[1], m_shape_list_hover_color[2], m_shape_list_hover_color[3]}},
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
    float       bg1[3] = {0.85f, 0.88f, 0.90f};
    float       bg2[3] = {0.45f, 0.55f, 0.60f};
    int         method = 1;
    float       g1[3]  = {0.1f, 0.1f, 0.1f};
    float       g2[3]  = {0.1f, 0.1f, 0.3f};
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
    apply_num("grid_graphic_x_size", grid_rect.graphic_x_size);
    apply_num("grid_graphic_y_size", grid_rect.graphic_y_size);
    apply_num("grid_graphic_z_offset", grid_rect.graphic_z_offset);
    m_view->set_occt_grid_rect_params(grid_rect);
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
    m_show_lua_console    = b("show_lua_console", true);
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
    m_load_last_opened_on_startup = b("load_last_opened_on_startup", b("load_last_saved_on_startup", false));
    if (g.contains("last_opened_project_path") && g["last_opened_project_path"].is_string())
      m_last_opened_project_path = g["last_opened_project_path"].get<std::string>();
    else if (g.contains("last_saved_project_path") && g["last_saved_project_path"].is_string())
      m_last_opened_project_path = g["last_saved_project_path"].get<std::string>();

    float fb_general = 0.f, fb_scroll = 0.f, fb_tabs = 0.f;
    imgui_rounding_fallbacks_from_theme_(fb_general, fb_scroll, fb_tabs);
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
    m_imgui_rounding_general = round_from_json("imgui_rounding_general", fb_general);
    m_imgui_rounding_scroll  = round_from_json("imgui_rounding_scroll", fb_scroll);
    m_imgui_rounding_tabs    = round_from_json("imgui_rounding_tabs", fb_tabs);

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

    // Default snap-guide color is green unless overridden by settings JSON.
    Sketch_nodes::set_snap_guide_color(0.0f, 1.0f, 0.0f);
    if (g.contains("snap_guide_color") && g["snap_guide_color"].is_array() && g["snap_guide_color"].size() >= 3)
    {
      const json& a = g["snap_guide_color"];
      glm::vec3   c(0.0f, 1.0f, 0.0f);
      for (size_t i = 0; i < 3; ++i)
        if (a[static_cast<json::size_type>(i)].is_number())
          c[static_cast<glm::vec3::length_type>(i)] = std::clamp(a[static_cast<json::size_type>(i)].get<float>(), 0.f, 1.f);
      Sketch_nodes::set_snap_guide_color(c[0], c[1], c[2]);
    }

    Sketch_nodes::set_snap_guide_mode(Sketch_nodes::Snap_guide_mode::Traditional);
    if (g.contains("snap_guide_mode") && g["snap_guide_mode"].is_number_integer())
    {
      const int mode = g["snap_guide_mode"].get<int>();
      if (mode >= static_cast<int>(Sketch_nodes::Snap_guide_mode::Traditional) &&
          mode <= static_cast<int>(Sketch_nodes::Snap_guide_mode::Both))
        Sketch_nodes::set_snap_guide_mode(static_cast<Sketch_nodes::Snap_guide_mode>(mode));
    }

    Sketch_nodes::set_annotate_all_coaxial_nodes(false);
    if (g.contains("annotate_all_coaxial_nodes") && g["annotate_all_coaxial_nodes"].is_boolean())
    {
      Sketch_nodes::set_annotate_all_coaxial_nodes(g["annotate_all_coaxial_nodes"].get<bool>());
    }

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

    if (g.contains("shape_list_hover_color") && g["shape_list_hover_color"].is_array() &&
        g["shape_list_hover_color"].size() >= 3)
    {
      const json& a = g["shape_list_hover_color"];
      for (int i = 0; i < 3; ++i)
        if (a[static_cast<json::size_type>(i)].is_number())
          m_shape_list_hover_color[static_cast<glm::vec4::length_type>(i)] =
              std::clamp(a[static_cast<json::size_type>(i)].get<float>(), 0.f, 1.f);
      m_shape_list_hover_color[3] = 1.f;
      if (a.size() >= 4 && a[3].is_number())
        m_shape_list_hover_color[3] = std::clamp(a[3].get<float>(), 0.f, 1.f);
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
    apply_sketch_dimensions_visibility();

  try
  {
    using namespace nlohmann;
    const json j = json::parse(content);
    if (j.contains("imgui_ini") && j["imgui_ini"].is_string())
    {
      const std::string& ini = j["imgui_ini"].get<std::string>();
      if (!ini.empty())
        ImGui::LoadIniSettingsFromMemory(ini.c_str(), ini.size());
    }
  }
  catch (...)
  {
    // EZY_ASSERT_MSG(false, "Settings invalid!");
  }

  log_message("Settings: loaded.");
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
  if (ui_show_help(3))
    ImGui::TextWrapped("0 = minimal UI. Odd values add more controls and panes; even values add more help (tooltips and "
                       "hints). Higher values are reserved for future tiers.");

  if (ImGui::Checkbox("Dark mode", &m_dark_mode))
    save_occt_view_settings();

  if (ImGui::CollapsingHeader("3D view navigation", ImGuiTreeNodeFlags_DefaultOpen))
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

      if (ui_show_help(2) && ImGui::IsItemHovered())
        ImGui::SetTooltip("Degrees per key press: NumPad 8/2/4/6 orbit (like LMB drag), Shift+NumPad 4/6, Shift+4/6, or "
                          "Shift+Left/Right roll. "
                          "Ctrl+click the slider to type a value.");

      if (ui_show_help(3))
      {
        ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
        if (ImGui::SmallButton("?##view_roll_help"))
          open_url_("https://ezycad.readthedocs.io/en/latest/usage.html#view-roll");
        if (ImGui::IsItemHovered())
          ImGui::SetTooltip("Help: view roll (opens the online usage guide).");
      }

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

      if (ui_show_help(2) && ImGui::IsItemHovered())
        ImGui::SetTooltip("Multiplier for mouse wheel and +/- zoom (same as UpdateZoom scroll delta). "
                          "Hold Shift while zooming for Blender-style finer steps (x0.1). Ctrl+click to type a value.");

      ImGui::EndTable();
    }

    if (ui_show_help(3))
      ImGui::TextWrapped(
          "NumPad 8 / 2 / 4 / 6 orbit the view (same axes as left-drag orbit). Hold Shift and press NumPad 4 or NumPad 6, "
          "main 4 / 6, or Left / Right arrow for Blender-style roll around the screen Z axis (hold to repeat). "
          "Num Lock off is recommended for numpad shortcuts (see usage.md View navigation). "
          "Hold Shift while scrolling or pressing +/- for finer zoom.");
  }

  if (ui_show_feature(3) && ImGui::CollapsingHeader("UI corner rounding"))
  {
    bool r_changed = false;
    if (ImGui::BeginTable("settings_rounding", 2, ImGuiTableFlags_SizingStretchProp))
    {
      ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, k_label_col_w);
      ImGui::TableSetupColumn("control", ImGuiTableColumnFlags_WidthStretch);

      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("Windows, frames, popups");
      ImGui::TableSetColumnIndex(1);
      r_changed |= ImGui::SliderFloat("##round_gen", &m_imgui_rounding_general, 0.f, 16.f, "%.0f");

      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("Scrollbars and sliders");
      ImGui::TableSetColumnIndex(1);
      r_changed |= ImGui::SliderFloat("##round_scr", &m_imgui_rounding_scroll, 0.f, 16.f, "%.0f");
      ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
      if (ui_show_help(2))
      {
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered())
        {
          ImGui::BeginTooltip();
          ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
          ImGui::TextDisabled("Scrollbars and sliders applies the same radius to scrollbar tracks and slider grabs.");
          ImGui::PopTextWrapPos();
          ImGui::EndTooltip();
        }
      }

      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("Tabs");
      ImGui::TableSetColumnIndex(1);
      r_changed |= ImGui::SliderFloat("##round_tabs", &m_imgui_rounding_tabs, 0.f, 16.f, "%.0f");

      ImGui::EndTable();
    }
    if (r_changed)
      save_occt_view_settings();
  }

  if (ImGui::CollapsingHeader("3D view background"))
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

    bool shape_list_hover_changed = false;
    if (ImGui::BeginTable("settings_shape_list_hover", 2, ImGuiTableFlags_SizingStretchProp))
    {
      ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, k_label_col_w);
      ImGui::TableSetupColumn("control", ImGuiTableColumnFlags_WidthStretch);
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("Shape list hover color");
      ImGui::TableSetColumnIndex(1);
      shape_list_hover_changed |=
          ImGui::ColorEdit4("##shape_list_hover", &m_shape_list_hover_color[0], ImGuiColorEditFlags_Float);
      if (ui_show_help(2))
      {
        ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered())
        {
          ImGui::BeginTooltip();
          ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
          ImGui::TextDisabled("Highlight color when hovering a row in the Shape List pane. Updates immediately if a shape "
                              "is hovered.");
          ImGui::PopTextWrapPos();
          ImGui::EndTooltip();
        }
      }
      ImGui::EndTable();
    }
    if (shape_list_hover_changed)
    {
      m_view->refresh_shape_list_hover_highlight();
      save_occt_view_settings();
    }
  }

  if (ui_show_feature(3) && ImGui::CollapsingHeader("3D view grid"))
  {
    float g1[3], g2[3];
    m_view->get_grid_colors(g1, g2);
    Occt_grid_rect_params gr{};
    m_view->get_occt_grid_rect_params(gr);
    const double dim_scale = m_view->get_dimension_scale();
    // Settings show the same length units as sketch dimensions (model / dimension_scale).
    // Grid extent UI is full span; OCCT SetGraphicValues uses half-extent (x0.5 on apply).
    double          step_ui          = gr.step / dim_scale;
    double          graphic_x_ui     = (gr.graphic_x_size * 2.0) / dim_scale;
    double          graphic_y_ui     = (gr.graphic_y_size * 2.0) / dim_scale;
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
      if (ImGui::DragScalar("##gstep", ImGuiDataType_Double, &step_ui, spd_s, nullptr, nullptr, "%.8g"))
        geom_changed = true;

      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("Grid extent X");
      ImGui::SameLine(0.f, ImGui::GetStyle().ItemInnerSpacing.x);
      if (ui_show_help(2))
      {
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered())
        {
          ImGui::BeginTooltip();
          ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
          ImGui::TextUnformatted(
              "Full width of the drawn grid on X (edge to edge through the center). Same length scale as sketch dimensions. "
              "OCCT uses half this value internally.");
          ImGui::PopTextWrapPos();
          ImGui::EndTooltip();
        }
      }
      ImGui::TableSetColumnIndex(1);
      if (ImGui::DragScalar("##ggx", ImGuiDataType_Double, &graphic_x_ui, spd_extent, nullptr, nullptr, "%.8g"))
        geom_changed = true;

      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("Grid extent Y");
      if (ui_show_help(2))
      {
        ImGui::SameLine(0.f, ImGui::GetStyle().ItemInnerSpacing.x);
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered())
        {
          ImGui::BeginTooltip();
          ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
          ImGui::TextUnformatted(
              "Full height of the drawn grid on Y (edge to edge through the center). OCCT uses half this value internally.");
          ImGui::PopTextWrapPos();
          ImGui::EndTooltip();
        }
      }
      ImGui::TableSetColumnIndex(1);
      if (ImGui::DragScalar("##ggy", ImGuiDataType_Double, &graphic_y_ui, spd_extent, nullptr, nullptr, "%.8g"))
        geom_changed = true;

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
      gr.graphic_x_size   = graphic_x_ui * dim_scale * 0.5;
      gr.graphic_y_size   = graphic_y_ui * dim_scale * 0.5;
      gr.graphic_z_offset = graphic_z_off_ui * dim_scale;
      m_view->set_occt_grid_rect_params(gr);
    }
    if (grid_changed || geom_changed)
      save_occt_view_settings();
  }

  if (ImGui::CollapsingHeader("Sketch"))
  {
    bool ul_changed        = false;
    bool dim_changed       = false;
    bool node_anno_changed = false;

    ImGui::Indent(ImGui::GetStyle().IndentSpacing);
    if (ImGui::CollapsingHeader("Dimensions", ImGuiTreeNodeFlags_DefaultOpen))
    {
      if (ImGui::BeginTable("settings_sketch_dims", 2, ImGuiTableFlags_SizingStretchProp))
      {
        ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, k_label_col_w);
        ImGui::TableSetupColumn("control", ImGuiTableColumnFlags_WidthStretch);

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
          if (ui_show_help(2))
          {
            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered())
            {
              ImGui::BeginTooltip();
              ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
              ImGui::TextDisabled("Thickness of sketch edge length dimensions (Open CASCADE line width scale; 1.0 = default).");
              ImGui::PopTextWrapPos();
              ImGui::EndTooltip();
            }
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
          if (ui_show_help(2))
          {
            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered())
            {
              ImGui::BeginTooltip();
              ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
              ImGui::TextDisabled("Arrow head length for sketch and extrude length dimensions (Open CASCADE display units).");
              ImGui::PopTextWrapPos();
              ImGui::EndTooltip();
            }
          }
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
          if (ui_show_help(2) && ImGui::IsItemHovered())
            ImGui::SetTooltip("How dimension value labels are composited. Z-layer Top and Topmost avoid ghosting "
                              "against the grid; Topmost is the default.");
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
          if (ui_show_help(2) && ImGui::IsItemHovered())
            ImGui::SetTooltip("When off, hides all sketch length dimensions. Tool mode may still limit which sketch shows "
                              "dimensions when this is on.");
        }

        ImGui::EndTable();
      }
    }
    ImGui::Unindent(ImGui::GetStyle().IndentSpacing);

    if (ImGui::BeginTable("settings_sketch", 2, ImGuiTableFlags_SizingStretchProp))
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
        if (ui_show_help(2))
        {
          ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
          ImGui::TextDisabled("(?)");
          if (ImGui::IsItemHovered())
          {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextDisabled("Scale for permanent '+' node markers in sketch mode.");
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
          }
        }
      }

      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("Underlay highlight color");
      ImGui::TableSetColumnIndex(1);
      ul_changed |= ImGui::ColorEdit4("##underlay_hi", &m_underlay_highlight_color[0], ImGuiColorEditFlags_Float);
      if (ui_show_help(2))
      {
        ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered())
        {
          ImGui::BeginTooltip();
          ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
          ImGui::TextDisabled("Updates all sketch underlays immediately. Also used as the default when you import a new image. "
                              "Per-sketch overrides in Sketch List if needed.");
          ImGui::PopTextWrapPos();
          ImGui::EndTooltip();
        }
      }

      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("Snap guide color");
      ImGui::TableSetColumnIndex(1);
      {
        glm::vec3 snap_col;
        Sketch_nodes::get_snap_guide_color(snap_col[0], snap_col[1], snap_col[2]);
        if (ImGui::ColorEdit3("##snap_guide_color", &snap_col[0], ImGuiColorEditFlags_Float))
        {
          Sketch_nodes::set_snap_guide_color(snap_col[0], snap_col[1], snap_col[2]);
          save_occt_view_settings();
        }
        if (ui_show_help(2))
        {
          ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
          ImGui::TextDisabled("(?)");
          if (ImGui::IsItemHovered())
          {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextDisabled("Color used by fullscreen snap guides and snap markers in sketch mode.");
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
          }
        }
      }

      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("Snap guide mode");
      ImGui::TableSetColumnIndex(1);
      {
        constexpr std::array<const char*, 3> k_snap_guide_mode_labels = {
            "Traditional",
            "Fullscreen",
            "Both",
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
      }

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
        if (ImGui::IsItemHovered())
        {
          ImGui::BeginTooltip();
          ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
          ImGui::TextDisabled("When enabled (global mode): axis guide lines + markers for *all* nodes in the\n"
                              "current sketch and all other visible sketches (full co-axial alignments).\n"
                              "Disabled = only the closest node per active axis is marked (classic behavior).");
          ImGui::PopTextWrapPos();
          ImGui::EndTooltip();
        }
      }

      ImGui::EndTable();
    }
    if (dim_changed)
    {
      save_occt_view_settings();
      if (m_view)
        m_view->refresh_all_length_dimensions();
    }
    if (node_anno_changed)
    {
      save_occt_view_settings();
      if (m_view)
        m_view->refresh_all_permanent_node_annotations();
    }
    if (ul_changed)
    {
      uint8_t hr, hg, hb, ha;
      underlay_highlight_color_rgba(hr, hg, hb, ha);
      for (const Sketch::sptr& sk : m_view->get_sketches())
      {
        EZY_ASSERT(sk);
        if (sk->has_underlay())
          sk->underlay_set_line_tint_rgba(hr, hg, hb, ha);
      }
      m_underlay_panel_sketch = nullptr;
      save_occt_view_settings();
    }
  }

  if (ui_show_feature(3) && ImGui::CollapsingHeader("Startup project"))
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
      if (ui_show_help(2))
      {
        ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered())
        {
          ImGui::BeginTooltip();
          ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
          ImGui::TextDisabled("When enabled, EzyCad opens the last .ezy file you opened (path is stored in settings).");
          ImGui::PopTextWrapPos();
          ImGui::EndTooltip();
        }
      }
      ImGui::EndTable();
    }
    if (ui_show_help(3) && !m_last_opened_project_path.empty())
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
    if (ui_show_help(2))
    {
      ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
      ImGui::TextDisabled("(?)");
      if (ImGui::IsItemHovered())
      {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextDisabled("Save the current document (geometry, view, and tool mode) as what loads when EzyCad starts. "
                            "If none is saved, the install default (res/default.ezy) is used.");
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
      }
    }
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
            ImGui::LoadIniSettingsFromMemory(ini.c_str(), ini.size());
        }
        show_message("Default settings applied.");
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

void GUI::shape_list_hover_color_rgba(uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) const
{
  const auto to_u8 = [](float c) -> uint8_t
  {
    const float x = std::clamp(c, 0.f, 1.f) * 255.f;
    return static_cast<uint8_t>(x + 0.5f);
  };
  r = to_u8(m_shape_list_hover_color[0]);
  g = to_u8(m_shape_list_hover_color[1]);
  b = to_u8(m_shape_list_hover_color[2]);
  a = to_u8(m_shape_list_hover_color[3]);
}

void GUI::imgui_rounding_fallbacks_from_theme_(float& general, float& scroll, float& tabs) const
{
  ImGuiStyle s;
  if (m_dark_mode)
    ImGui::StyleColorsDark(&s);
  else
    ImGui::StyleColorsLight(&s);
  s.ScaleAllSizes(ImGui::GetStyle().FontScaleDpi);
  general = s.WindowRounding;
  scroll  = s.ScrollbarRounding;
  tabs    = s.TabRounding;
}

void GUI::apply_imgui_rounding_from_members_()
{
  ImGuiStyle& st       = ImGui::GetStyle();
  st.WindowRounding    = m_imgui_rounding_general;
  st.ChildRounding     = m_imgui_rounding_general;
  st.FrameRounding     = m_imgui_rounding_general;
  st.PopupRounding     = m_imgui_rounding_general;
  st.ScrollbarRounding = m_imgui_rounding_scroll;
  st.GrabRounding      = m_imgui_rounding_scroll;
  st.TabRounding       = m_imgui_rounding_tabs;
}
