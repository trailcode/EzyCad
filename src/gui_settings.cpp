// Settings dialog, JSON load/save, and OCCT view appearance from ezycad_settings.json.

#include "gui.h"

#include <sstream>

#include <nlohmann/json.hpp>

#include "imgui.h"
#include "occt_view.h"
#include "settings.h"

namespace
{
const char* const k_settings_version = "1";
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
  float bg1[3], bg2[3], g1[3], g2[3];
  m_view->get_bg_gradient_colors(bg1, bg2);
  m_view->get_grid_colors(g1, g2);
  int method     = m_view->get_bg_gradient_method();
  j["occt_view"] = {
      {         "bg_color1", {bg1[0], bg1[1], bg1[2]}},
      {         "bg_color2", {bg2[0], bg2[1], bg2[2]}},
      {"bg_gradient_method",                   method},
      {       "grid_color1",    {g1[0], g1[1], g1[2]}},
      {       "grid_color2",    {g2[0], g2[1], g2[2]}},
  };
  j["gui"] = {
      {        "show_options",         m_show_options},
      {    "show_sketch_list",     m_show_sketch_list},
      {     "show_shape_list",      m_show_shape_list},
      {  "log_window_visible",   m_log_window_visible},
      {"show_settings_dialog", m_show_settings_dialog},
      {           "dark_mode",            m_dark_mode},
      {    "show_lua_console",     m_show_lua_console},
      { "show_python_console",  m_show_python_console},
      {   "edge_dim_label_h",     m_edge_dim_label_h},
      {"load_last_opened_on_startup", m_load_last_opened_on_startup},
      {   "last_opened_project_path",    m_last_opened_project_path},
      {   "imgui_rounding_general",   m_imgui_rounding_general},
      {      "imgui_rounding_scroll",      m_imgui_rounding_scroll},
      {        "imgui_rounding_tabs",        m_imgui_rounding_tabs},
#ifndef NDEBUG
      {            "show_dbg",             m_show_dbg},
#endif
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
  }
  catch (...)
  {
    // TODO: log error
  }
}

void GUI::parse_occt_view_ini_(const std::string& content)
{
  bool               in_section = false;
  std::istringstream ss(content);
  std::string        line;
  float              bg1[3]     = {0.85f, 0.88f, 0.90f};
  float              bg2[3]     = {0.45f, 0.55f, 0.60f};
  int                method     = 1;
  float              g1[3]      = {0.1f, 0.1f, 0.1f};
  float              g2[3]      = {0.1f, 0.1f, 0.3f};
  auto               read_float = [](const std::string& v) -> float
  {
    float              x = 0.f;
    std::istringstream is(v);
    is >> x;
    return x;
  };
  while (std::getline(ss, line))
  {
    if (line.empty())
      continue;
    if (line[0] == '[')
    {
      in_section = (line == "[OCCTView]");
      continue;
    }
    if (!in_section)
      continue;
    size_t eq = line.find('=');
    if (eq == std::string::npos)
      continue;
    std::string key   = line.substr(0, eq);
    std::string value = line.substr(eq + 1);
    if (key == "BgR1")
      bg1[0] = read_float(value);
    else if (key == "BgG1")
      bg1[1] = read_float(value);
    else if (key == "BgB1")
      bg1[2] = read_float(value);
    else if (key == "BgR2")
      bg2[0] = read_float(value);
    else if (key == "BgG2")
      bg2[1] = read_float(value);
    else if (key == "BgB2")
      bg2[2] = read_float(value);
    else if (key == "BgMethod")
    {
      try
      {
        method = std::stoi(value);
      }
      catch (...)
      {
      }
    }
    else if (key == "GridR1")
      g1[0] = read_float(value);
    else if (key == "GridG1")
      g1[1] = read_float(value);
    else if (key == "GridB1")
      g1[2] = read_float(value);
    else if (key == "GridR2")
      g2[0] = read_float(value);
    else if (key == "GridG2")
      g2[1] = read_float(value);
    else if (key == "GridB2")
      g2[2] = read_float(value);
  }
  m_view->set_bg_gradient_colors(bg1[0], bg1[1], bg1[2], bg2[0], bg2[1], bg2[2]);
  m_view->set_bg_gradient_method(method);
  m_view->set_grid_colors(g1[0], g1[1], g1[2], g2[0], g2[1], g2[2]);
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
      return g.contains(key) && g[key].is_boolean() ? g[key].get<bool>() : current;
    };
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
    const json j          = json::parse(content);
    bool       version_ok = j.contains("version") && j["version"].is_string() &&
                      j["version"].get<std::string>() == k_settings_version;
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

  ImGui::SetNextWindowSize(ImVec2(520, 0), ImGuiCond_FirstUseEver);  // Auto height; width matches res defaults
  if (!ImGui::Begin("Settings", &m_show_settings_dialog, ImGuiWindowFlags_None))
  {
    ImGui::End();
    return;
  }

  constexpr float k_label_col_w = 230.f;

  if (ImGui::Checkbox("Dark mode", &m_dark_mode))
    save_occt_view_settings();

  if (ImGui::CollapsingHeader("UI corner rounding"))
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
    ImGui::TextDisabled(
        "Scrollbars and sliders applies the same radius to scrollbar tracks and slider grabs.");
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
                                        "Corner 1", "Corner 2", "Corner 3", "Corner 4"};
      int         grad               = m_view->get_bg_gradient_method();
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
  }

  if (ImGui::CollapsingHeader("3D view grid"))
  {
    float g1[3], g2[3];
    m_view->get_grid_colors(g1, g2);
    bool grid_changed = false;
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

      ImGui::EndTable();
    }
    if (grid_changed)
    {
      m_view->set_grid_colors(g1[0], g1[1], g1[2], g2[0], g2[1], g2[2]);
      save_occt_view_settings();
    }
  }

  if (ImGui::CollapsingHeader("Startup project"))
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
      ImGui::EndTable();
    }
    ImGui::TextWrapped(
        "When enabled, EzyCad opens the last .ezy file you opened (path is stored in settings).");
    if (!m_last_opened_project_path.empty())
      ImGui::TextWrapped("Last opened path: %s", m_last_opened_project_path.c_str());
    else
      ImGui::TextDisabled("(No path saved yet.)");
    ImGui::Spacing();
#endif
    ImGui::TextWrapped(
        "Save the current document (geometry, view, and tool mode) as what loads when EzyCad starts. "
        "If none is saved, the install default (res/default.ezy) is used.");
    if (ImGui::Button("Save current as startup project"))
      save_startup_project_();
    ImGui::SameLine();
    if (ImGui::Button("Clear saved startup"))
      clear_saved_startup_project_();
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
  ImGuiStyle& st        = ImGui::GetStyle();
  st.WindowRounding     = m_imgui_rounding_general;
  st.ChildRounding      = m_imgui_rounding_general;
  st.FrameRounding      = m_imgui_rounding_general;
  st.PopupRounding      = m_imgui_rounding_general;
  st.ScrollbarRounding  = m_imgui_rounding_scroll;
  st.GrabRounding       = m_imgui_rounding_scroll;
  st.TabRounding        = m_imgui_rounding_tabs;
}
