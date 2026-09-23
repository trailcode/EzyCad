// Shape List pane, Shape info dialog, and .ezy ui.shapeList expand/current-group state.

#include "gui.h"

#include "gui_occt_view.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "shp_info.h"
#include "shp_set_frame.h"
#include "utl.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_set>

namespace gui_shp_detail
{

enum class Shp_list_kind
{
  Design,
  Workbench
};

struct Shape_list_row_drawer
{
  Shape_list_row_drawer(GUI& gui, const std::vector<std::string>& mat_names, int nmat, float mat_popup_w,
                        Shp_list_kind kind);

  void draw(const Shp_ptr& shape);

  Shp_ptr  hover;
  Shp_ptr  to_delete;
  Shape_id to_ungroup_id = 0;

private:
  [[nodiscard]] bool                   row_is_selected_(const Shp_ptr& shape) const;
  [[nodiscard]] std::vector<Shp_ptr>   children_(Shape_id parent_id) const;
  [[nodiscard]] std::vector<Shp_ptr>   descendant_solids_(Shape_id id) const;
  [[nodiscard]] std::unordered_map<Shape_id, bool>& expanded_();
  void               select_row_(const Shp_ptr& shape);
  void               apply_material_(const Shp_ptr& shape, int i);
  void               draw_ctx_menu_(const Shp_ptr& shape, bool is_group);
  void               accept_reparent_drop_(const Shp_ptr& shape, bool is_group);

  GUI&                                   m_gui;
  Occt_view&                             m_view;
  Shp_list_kind                          m_kind;
  const std::vector<std::string>&        m_mat_names;
  int                                    m_nmat        = 0;
  float                                  m_mat_popup_w = 0.0f;
  std::unordered_set<const AIS_Shape*>   m_selected_in_viewer;
  std::unordered_set<Shape_id>           m_ancestors;
};

} // namespace gui_shp_detail

void GUI::shape_list_()
{
  if (!show_shape_list_effective())
  {
    m_view->set_shape_list_hover(nullptr);
    return;
  }

  if (!ImGui::Begin("Shape List", &m_show_shape_list, ImGuiWindowFlags_None))
  {
    m_view->set_shape_list_hover(nullptr);
    ImGui::End();
    return;
  }

  if (ImGui::Checkbox("Hide all", &m_hide_all_shapes))
  {
    if (m_hide_all_shapes)
      m_view->set_shape_list_hover(nullptr);
    m_view->sync_sketch_shape_faint_style();
  }

  ImGui::SameLine();
  if (ImGui::SmallButton("New group"))
  {
    Shp_ptr grp = m_view->create_group("Group", m_view->current_group_id());
    if (!grp.IsNull())
      m_view->set_current_group_id(grp->get_id());
  }

  ImGui::SameLine();
  {
    const std::vector<Shp_ptr> sel       = m_view->get_selected_shps();
    const bool                 can_group = !sel.empty();
    if (!can_group)
      ImGui::BeginDisabled();

    if (ImGui::SmallButton("Group"))
      (void)m_view->group_shapes(sel);

    if (!can_group)
      ImGui::EndDisabled();
  }

  ImGui::SameLine();
  {
    const std::vector<Shp_ptr> sel     = m_view->get_selected_shps();
    const bool                 can_add = !sel.empty() || m_view->current_group_id() != 0;
    if (!can_add)
      ImGui::BeginDisabled();

    if (ImGui::SmallButton("Add to Workbench"))
    {
      std::vector<Shp_ptr> nodes = sel;
      if (nodes.empty() && m_view->current_group_id() != 0)
        if (Shp_ptr g = m_view->find_design_shape_by_id(m_view->current_group_id()); !g.IsNull())
          nodes.push_back(g);

      const Status st = m_view->add_to_workbench(nodes);
      if (!st.is_ok())
        show_status(st);
      else
        show_message("Added to Workbench.", Status_msg::Success);
    }

    if (!can_add)
      ImGui::EndDisabled();
  }

  ImGui::Separator();

  const std::vector<std::string>& mat_names       = occt_material_combo_labels_();
  const int                       nmat            = static_cast<int>(mat_names.size());
  float                           mat_label_w_max = 0.0f;
  for (int mi = 0; mi < nmat; ++mi)
    mat_label_w_max = std::max(mat_label_w_max, ImGui::CalcTextSize(mat_names[static_cast<size_t>(mi)].c_str()).x);

  const ImGuiStyle& st_mat      = ImGui::GetStyle();
  const float       mat_popup_w = std::min(440.0f, std::max(280.0f, mat_label_w_max + st_mat.WindowPadding.x * 2.0f +
                                                                        st_mat.FramePadding.x * 2.0f + st_mat.ScrollbarSize + 8.0f));
  const float       check_col_w = ImGui::GetFrameHeight();
  const float       mat_col_w   = ImGui::CalcTextSize("M").x + st_mat.FramePadding.x * 2.0f;

  gui_shp_detail::Shape_list_row_drawer row(*this, mat_names, nmat, mat_popup_w, gui_shp_detail::Shp_list_kind::Design);

  const ImGuiTableFlags table_flags =
      ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_SizingFixedFit;
  if (ImGui::BeginTable("##shape_outliner", 4, table_flags, ImVec2(0.f, 0.f)))
  {
    // Fixed actions on the left; stretch name on the right (tree indent on name only).
    ImGui::TableSetupColumn(
        "vis", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_IndentDisable,
        check_col_w);
    ImGui::TableSetupColumn(
        "disp", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_IndentDisable,
        check_col_w);
    ImGui::TableSetupColumn(
        "mat", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_IndentDisable,
        mat_col_w);
    ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_IndentEnable, 1.0f);

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                        ImVec2(st_mat.FramePadding.x, std::max(1.0f, st_mat.FramePadding.y * 0.65f)));
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(st_mat.CellPadding.x, std::max(1.0f, st_mat.CellPadding.y * 0.5f)));

    for (const Shp_ptr& root : m_view->shape_children(0))
      row.draw(root);

    // Empty pad below the last row: drop here to move to document root (no permanent root node).
    // Size from the visible clip remainder, not GetContentRegionAvail().y -- inside a ScrollY
    // table that avail tracks content size, so a fill-avail pad feeds back into growing scroll.
    {
      const float        min_pad_h = ImGui::GetFrameHeight();
      const ImGuiWindow* inner     = ImGui::GetCurrentWindow();
      const float visible_remain = inner->InnerClipRect.Max.y - ImGui::GetCursorScreenPos().y - ImGui::GetStyle().CellPadding.y;
      const float pad_h          = std::max(min_pad_h, visible_remain);
      ImGui::TableNextRow(ImGuiTableRowFlags_None, pad_h);
      ImGui::TableSetColumnIndex(0);

      const ImGuiPayload* active_payload = ImGui::GetDragDropPayload();
      const bool          dragging_shape = active_payload != nullptr && active_payload->IsDataType("EZY_SHAPE_ID") &&
                                  active_payload->DataSize == sizeof(Shape_id);

      if (dragging_shape)
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGuiCol_Header, 0.20f));

      ImGui::Selectable("##shape_root_drop", false,
                        ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap | ImGuiSelectableFlags_Disabled,
                        ImVec2(0.0f, pad_h));

      bool root_drop_hovered = false;
      if (ImGui::BeginDragDropTarget())
      {
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("EZY_SHAPE_ID", ImGuiDragDropFlags_AcceptBeforeDelivery);
        if (payload != nullptr)
        {
          root_drop_hovered = payload->Preview;
          if (payload->IsDelivery())
          {
            Shape_id drag_id = 0;
            std::memcpy(&drag_id, payload->Data, sizeof(drag_id));
            (void)m_view->reparent_shape(drag_id, 0, -1, true);
          }
        }
        ImGui::EndDragDropTarget();
      }

      if (dragging_shape)
      {
        const ImVec2 rmin = ImGui::GetItemRectMin();
        const ImVec2 rmax = ImGui::GetItemRectMax();
        if (root_drop_hovered)
          ImGui::GetWindowDrawList()->AddRectFilled(rmin, rmax, ImGui::GetColorU32(ImGuiCol_HeaderHovered, 0.55f));

        const char*  hint = "Move to root";
        const ImVec2 ts   = ImGui::CalcTextSize(hint);
        ImGui::GetWindowDrawList()->AddText(
            ImVec2(rmin.x + (rmax.x - rmin.x - ts.x) * 0.5f, rmin.y + (rmax.y - rmin.y - ts.y) * 0.5f),
            ImGui::GetColorU32(ImGuiCol_TextDisabled), hint);
      }
    }

    ImGui::PopStyleVar(2);
    ImGui::EndTable();
  }

  m_view->set_shape_list_hover(row.hover);

  if (row.to_ungroup_id != 0)
  {
    m_shape_list_expanded.erase(row.to_ungroup_id);
    (void)m_view->ungroup_shape(row.to_ungroup_id);
  }

  if (row.to_delete)
    m_view->delete_shapes({row.to_delete});

  ImGui::End();
}

void GUI::workbench_list_()
{
  if (!show_workbench_list_effective())
  {
    if (m_view->shape_list_hover() && m_view->shape_list_hover()->is_workbench())
      m_view->set_shape_list_hover(nullptr);

    return;
  }

  if (!ImGui::Begin("Workbench List", &m_show_workbench_list, ImGuiWindowFlags_None))
  {
    if (m_view->shape_list_hover() && m_view->shape_list_hover()->is_workbench())
      m_view->set_shape_list_hover(nullptr);

    ImGui::End();
    return;
  }

  if (ImGui::Checkbox("Hide all", &m_hide_all_workbench))
  {
    if (m_hide_all_workbench && m_view->shape_list_hover() && m_view->shape_list_hover()->is_workbench())
      m_view->set_shape_list_hover(nullptr);

    m_view->sync_sketch_shape_faint_style();
  }

  ImGui::SameLine();
  if (ImGui::SmallButton("New group"))
  {
    Shp_ptr grp = m_view->create_workbench_group("Group", m_view->current_workbench_group_id());
    if (!grp.IsNull())
      m_view->set_current_workbench_group_id(grp->get_id());
  }

  ImGui::SameLine();
  {
    const std::vector<Shp_ptr> sel       = m_view->get_selected_shps();
    std::vector<Shp_ptr>       wbk_sel;
    for (const Shp_ptr& s : sel)
      if (!s.IsNull() && s->is_workbench())
        wbk_sel.push_back(s);

    const bool can_group = !wbk_sel.empty();
    if (!can_group)
      ImGui::BeginDisabled();

    if (ImGui::SmallButton("Group"))
      (void)m_view->group_workbench_shapes(wbk_sel);

    if (!can_group)
      ImGui::EndDisabled();
  }

  ImGui::Separator();

  const std::vector<std::string>& mat_names       = occt_material_combo_labels_();
  const int                       nmat            = static_cast<int>(mat_names.size());
  float                           mat_label_w_max = 0.0f;
  for (int mi = 0; mi < nmat; ++mi)
    mat_label_w_max = std::max(mat_label_w_max, ImGui::CalcTextSize(mat_names[static_cast<size_t>(mi)].c_str()).x);

  const ImGuiStyle& st_mat      = ImGui::GetStyle();
  const float       mat_popup_w = std::min(440.0f, std::max(280.0f, mat_label_w_max + st_mat.WindowPadding.x * 2.0f +
                                                                        st_mat.FramePadding.x * 2.0f + st_mat.ScrollbarSize + 8.0f));
  const float       check_col_w = ImGui::GetFrameHeight();
  const float       mat_col_w   = ImGui::CalcTextSize("M").x + st_mat.FramePadding.x * 2.0f;

  gui_shp_detail::Shape_list_row_drawer row(*this, mat_names, nmat, mat_popup_w, gui_shp_detail::Shp_list_kind::Workbench);

  const ImGuiTableFlags table_flags =
      ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_SizingFixedFit;
  if (ImGui::BeginTable("##workbench_outliner", 4, table_flags, ImVec2(0.f, 0.f)))
  {
    ImGui::TableSetupColumn(
        "vis", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_IndentDisable,
        check_col_w);
    ImGui::TableSetupColumn(
        "disp", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_IndentDisable,
        check_col_w);
    ImGui::TableSetupColumn(
        "mat", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_IndentDisable,
        mat_col_w);
    ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_IndentEnable, 1.0f);

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                        ImVec2(st_mat.FramePadding.x, std::max(1.0f, st_mat.FramePadding.y * 0.65f)));
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(st_mat.CellPadding.x, std::max(1.0f, st_mat.CellPadding.y * 0.5f)));

    for (const Shp_ptr& root : m_view->workbench_children(0))
      row.draw(root);

    {
      const float        min_pad_h = ImGui::GetFrameHeight();
      const ImGuiWindow* inner     = ImGui::GetCurrentWindow();
      const float visible_remain = inner->InnerClipRect.Max.y - ImGui::GetCursorScreenPos().y - ImGui::GetStyle().CellPadding.y;
      const float pad_h          = std::max(min_pad_h, visible_remain);
      ImGui::TableNextRow(ImGuiTableRowFlags_None, pad_h);
      ImGui::TableSetColumnIndex(0);

      const ImGuiPayload* active_payload = ImGui::GetDragDropPayload();
      const bool          dragging_wbk   = active_payload != nullptr && active_payload->IsDataType("EZY_WBK_ID") &&
                                active_payload->DataSize == sizeof(Shape_id);
      const bool dragging_design = active_payload != nullptr && active_payload->IsDataType("EZY_SHAPE_ID") &&
                                   active_payload->DataSize == sizeof(Shape_id);

      if (dragging_wbk || dragging_design)
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGuiCol_Header, 0.20f));

      ImGui::Selectable("##wbk_root_drop", false,
                        ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap | ImGuiSelectableFlags_Disabled,
                        ImVec2(0.0f, pad_h));

      bool root_drop_hovered = false;
      if (ImGui::BeginDragDropTarget())
      {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("EZY_WBK_ID", ImGuiDragDropFlags_AcceptBeforeDelivery))
        {
          root_drop_hovered = payload->Preview;
          if (payload->IsDelivery())
          {
            Shape_id drag_id = 0;
            std::memcpy(&drag_id, payload->Data, sizeof(drag_id));
            (void)m_view->reparent_workbench_shape(drag_id, 0, -1, true);
          }
        }

        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("EZY_SHAPE_ID", ImGuiDragDropFlags_AcceptBeforeDelivery))
        {
          root_drop_hovered = payload->Preview;
          if (payload->IsDelivery())
          {
            Shape_id drag_id = 0;
            std::memcpy(&drag_id, payload->Data, sizeof(drag_id));
            if (Shp_ptr src = m_view->find_design_shape_by_id(drag_id); !src.IsNull())
            {
              const Status st = m_view->add_to_workbench({src});
              if (!st.is_ok())
                show_status(st);
            }
          }
        }
        ImGui::EndDragDropTarget();
      }

      if (dragging_wbk || dragging_design)
      {
        const ImVec2 rmin = ImGui::GetItemRectMin();
        const ImVec2 rmax = ImGui::GetItemRectMax();
        if (root_drop_hovered)
          ImGui::GetWindowDrawList()->AddRectFilled(rmin, rmax, ImGui::GetColorU32(ImGuiCol_HeaderHovered, 0.55f));

        const char*  hint = dragging_design ? "Add to Workbench" : "Move to root";
        const ImVec2 ts   = ImGui::CalcTextSize(hint);
        ImGui::GetWindowDrawList()->AddText(
            ImVec2(rmin.x + (rmax.x - rmin.x - ts.x) * 0.5f, rmin.y + (rmax.y - rmin.y - ts.y) * 0.5f),
            ImGui::GetColorU32(ImGuiCol_TextDisabled), hint);
      }
    }

    ImGui::PopStyleVar(2);
    ImGui::EndTable();
  }

  if (row.hover)
    m_view->set_shape_list_hover(row.hover);
  else if (m_view->shape_list_hover() && m_view->shape_list_hover()->is_workbench())
    m_view->set_shape_list_hover(nullptr);

  if (row.to_ungroup_id != 0)
  {
    m_workbench_list_expanded.erase(row.to_ungroup_id);
    (void)m_view->ungroup_workbench_shape(row.to_ungroup_id);
  }

  if (row.to_delete)
    m_view->delete_shapes({row.to_delete});

  ImGui::End();
}

void GUI::open_shape_info_(const Shp_ptr& shape)
{
  if (shape.IsNull() || shape->is_group())
    return;

  const std::vector<std::string>& mat_names = occt_material_combo_labels_();
  int                             mat_idx   = shape->Material();
  if (mat_idx < 0 || mat_idx >= static_cast<int>(mat_names.size()))
    mat_idx = static_cast<int>(m_view->get_default_material().Name());

  shp_info::Display_meta meta;
  meta.name         = shape->get_name();
  meta.material     = mat_names[static_cast<size_t>(mat_idx)];
  meta.display_mode = shape->get_disp_mode() == AIS_Shaded ? "Shaded" : "Wireframe";
  meta.visible      = shape->get_visible();

  m_shape_info_shp   = shape;
  m_shape_info_lines = shp_info::collect(shape->Shape(), &meta);
  m_shape_info_open  = true;
}

void GUI::shape_info_dialog_()
{
  if (!m_shape_info_open)
    return;

  if (m_shape_info_shp.IsNull())
  {
    m_shape_info_open = false;
    return;
  }

  bool shape_still_exists = false;
  for (const Shp_ptr& s : m_view->get_shapes())
    if (s == m_shape_info_shp)
    {
      shape_still_exists = true;
      break;
    }

  if (!shape_still_exists)
    for (const Shp_ptr& s : m_view->get_workbench_shapes())
      if (s == m_shape_info_shp)
      {
        shape_still_exists = true;
        break;
      }

  if (!shape_still_exists)
  {
    clear_all(m_shape_info_open, m_shape_info_shp, m_shape_info_lines);
    return;
  }

  const std::string title = "Shape info: " + m_shape_info_shp->get_name();
  ImGui::SetNextWindowSize(ImVec2(440.0f, 0.0f), ImGuiCond_FirstUseEver);
  bool open = m_shape_info_open;
  if (!ImGui::Begin(title.c_str(), &open, ImGuiWindowFlags_None))
  {
    m_shape_info_open = open;
    ImGui::End();
    return;
  }

  if (ImGui::Button("Refresh"))
    open_shape_info_(m_shape_info_shp);

  ImGui::Separator();

  const float max_h = ImGui::GetTextLineHeightWithSpacing() * 18.0f;
  if (ImGui::BeginChild("shape_info_scroll", ImVec2(0.0f, max_h), ImGuiChildFlags_Borders,
                        ImGuiWindowFlags_AlwaysVerticalScrollbar))
  {
    if (ImGui::BeginTable("shape_info_tbl", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg))
    {
      ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, 120.0f);
      ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);

      for (const shp_info::Line& line : m_shape_info_lines)
      {
        if (line.label.empty() && line.value.empty())
        {
          ImGui::TableNextRow();
          ImGui::TableSetColumnIndex(0);
          ImGui::Separator();
          ImGui::TableSetColumnIndex(1);
          ImGui::Separator();
          continue;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(line.label.c_str());
        ImGui::TableSetColumnIndex(1);
        ImGui::TextUnformatted(line.value.c_str());
      }

      ImGui::EndTable();
    }

    ImGui::EndChild();
  }

  m_shape_info_open = open;
  if (!open)
    m_shape_info_shp.Nullify();

  ImGui::End();
}

nlohmann::json GUI::shape_list_ui_to_json_() const
{
  using namespace nlohmann;
  json out;
  json expanded = json::object();
  for (const auto& [id, is_open] : m_shape_list_expanded)
  {
    if (!is_open)
      expanded[std::to_string(id)] = false;
  }
  if (!expanded.empty())
    out["expanded"] = std::move(expanded);

  if (m_view && m_view->current_group_id() != 0)
    out["currentGroupId"] = m_view->current_group_id();

  return out;
}

void GUI::apply_shape_list_ui_from_json_(const nlohmann::json& j)
{
  using namespace nlohmann;
  m_shape_list_expanded.clear();
  if (m_view)
    m_view->set_current_group_id(0);

  if (!j.contains("ui") || !j["ui"].is_object())
    return;

  const json& ui = j["ui"];
  if (!ui.contains("shapeList") || !ui["shapeList"].is_object())
    return;

  const json& sl = ui["shapeList"];
  if (sl.contains("expanded") && sl["expanded"].is_object())
  {
    for (auto it = sl["expanded"].begin(); it != sl["expanded"].end(); ++it)
    {
      if (!it.value().is_boolean())
        continue;

      try
      {
        const Shape_id id         = static_cast<Shape_id>(std::stoull(it.key()));
        m_shape_list_expanded[id] = it.value().get<bool>();
      }
      catch (...)
      {
      }
    }
  }

  if (m_view && sl.contains("currentGroupId") && sl["currentGroupId"].is_number_unsigned())
    m_view->set_current_group_id(sl["currentGroupId"].get<Shape_id>());
}

nlohmann::json GUI::workbench_list_ui_to_json_() const
{
  using namespace nlohmann;
  json out;
  json expanded = json::object();
  for (const auto& [id, is_open] : m_workbench_list_expanded)
  {
    if (!is_open)
      expanded[std::to_string(id)] = false;
  }
  if (!expanded.empty())
    out["expanded"] = std::move(expanded);

  if (m_view && m_view->current_workbench_group_id() != 0)
    out["currentGroupId"] = m_view->current_workbench_group_id();

  return out;
}

void GUI::apply_workbench_list_ui_from_json_(const nlohmann::json& j)
{
  using namespace nlohmann;
  m_workbench_list_expanded.clear();
  if (m_view)
    m_view->set_current_workbench_group_id(0);

  if (!j.contains("ui") || !j["ui"].is_object())
    return;

  const json& ui = j["ui"];
  if (!ui.contains("workbenchList") || !ui["workbenchList"].is_object())
    return;

  const json& sl = ui["workbenchList"];
  if (sl.contains("expanded") && sl["expanded"].is_object())
  {
    for (auto it = sl["expanded"].begin(); it != sl["expanded"].end(); ++it)
    {
      if (!it.value().is_boolean())
        continue;

      try
      {
        const Shape_id id             = static_cast<Shape_id>(std::stoull(it.key()));
        m_workbench_list_expanded[id] = it.value().get<bool>();
      }
      catch (...)
      {
      }
    }
  }

  if (m_view && sl.contains("currentGroupId") && sl["currentGroupId"].is_number_unsigned())
    m_view->set_current_workbench_group_id(sl["currentGroupId"].get<Shape_id>());
}

namespace gui_shp_detail
{

Shape_list_row_drawer::Shape_list_row_drawer(GUI& gui, const std::vector<std::string>& mat_names, int nmat,
                                             float mat_popup_w, Shp_list_kind kind)
  : m_gui(gui)
  , m_view(*gui.get_view())
  , m_kind(kind)
  , m_mat_names(mat_names)
  , m_nmat(nmat)
  , m_mat_popup_w(mat_popup_w)
{
  for (const AIS_Shape_ptr& ais : m_view.get_selected())
    if (!ais.IsNull())
      m_selected_in_viewer.insert(ais.get());
}

std::vector<Shp_ptr> Shape_list_row_drawer::children_(Shape_id parent_id) const
{
  return m_kind == Shp_list_kind::Workbench ? m_view.workbench_children(parent_id) : m_view.shape_children(parent_id);
}

std::vector<Shp_ptr> Shape_list_row_drawer::descendant_solids_(Shape_id id) const
{
  return m_kind == Shp_list_kind::Workbench ? m_view.workbench_descendant_solids(id) : m_view.shape_descendant_solids(id);
}

std::unordered_map<Shape_id, bool>& Shape_list_row_drawer::expanded_()
{
  return m_kind == Shp_list_kind::Workbench ? m_gui.m_workbench_list_expanded : m_gui.m_shape_list_expanded;
}

bool Shape_list_row_drawer::row_is_selected_(const Shp_ptr& shape) const
{
  if (shape.IsNull())
    return false;

  if (shape->is_group())
  {
    for (const Shp_ptr& leaf : descendant_solids_(shape->get_id()))
      if (m_selected_in_viewer.count(leaf.get()) != 0)
        return true;

    return false;
  }

  return m_selected_in_viewer.count(shape.get()) != 0;
}

void Shape_list_row_drawer::select_row_(const Shp_ptr& shape)
{
  m_gui.ensure_task_(m_kind == Shp_list_kind::Workbench ? Task::Workbench : Task::Design);

  AIS_InteractiveContext& ctx  = m_view.ctx();
  const bool              ctrl = ImGui::GetIO().KeyCtrl;
  if (!ctrl)
    ctx.ClearSelected(false);

  if (shape->is_group())
  {
    if (m_kind == Shp_list_kind::Workbench)
      m_view.set_current_workbench_group_id(shape->get_id());
    else
      m_view.set_current_group_id(shape->get_id());

    for (const Shp_ptr& leaf : descendant_solids_(shape->get_id()))
      ctx.AddOrRemoveSelected(leaf, true);
  }
  else
  {
    if (m_kind == Shp_list_kind::Workbench)
      m_view.set_current_workbench_group_id(shape->get_parent_id());
    else
      m_view.set_current_group_id(shape->get_parent_id());

    ctx.AddOrRemoveSelected(shape, true);
  }

  ctx.UpdateCurrentViewer();
}

void Shape_list_row_drawer::apply_material_(const Shp_ptr& shape, int i)
{
  if (shape->is_group() || i < 0 || i >= m_nmat)
    return;

  // OwnColor (e.g. from older wasm SetColor workarounds) overrides material presets.
  if (shape->HasColor())
    shape->UnsetColor();

  shape->SetMaterial(Graphic3d_MaterialAspect(static_cast<Graphic3d_NameOfMaterial>(i)));
  m_view.refresh_shape_shading_(shape);
  m_view.ctx().Redisplay(shape, true);
  m_view.ctx().UpdateCurrentViewer();
}

void Shape_list_row_drawer::draw_ctx_menu_(const Shp_ptr& shape, bool is_group)
{
  const bool can_zoom = is_group ? !descendant_solids_(shape->get_id()).empty() : !shape->Shape().IsNull();
  if (ImGui::MenuItem("Zoom to", nullptr, false, can_zoom))
  {
    select_row_(shape);
    m_view.fit_shapes_in_view(shape);
  }

  if (!is_group && ImGui::MenuItem("Shape info..."))
    m_gui.open_shape_info_(shape);

  if (is_group && ImGui::MenuItem("Ungroup"))
    to_ungroup_id = shape->get_id();

  if (!is_group)
  {
    ImGui::Separator();
    bool show_axes = shape->show_frame_axes();
    if (ImGui::MenuItem("Show axes", nullptr, show_axes))
      shape->set_show_frame_axes(!show_axes);
    bool show_plane = shape->show_frame_plane();
    if (ImGui::MenuItem("Show plane", nullptr, show_plane))
      shape->set_show_frame_plane(!show_plane);
    bool show_up = shape->show_frame_up();
    if (ImGui::MenuItem("Show up", nullptr, show_up))
      shape->set_show_frame_up(!show_up);
    ImGui::Separator();
    if (ImGui::MenuItem("Reset frame to bbox"))
    {
      select_row_(shape);
      m_view.set_shape_frame(shape, Shp::default_frame_for(shape->Shape()));
      shape->set_show_frame_axes(true);
    }

    const Mode set_frame_mode = shape->is_workbench() ? Mode::Workbench_set_frame : Mode::Shape_set_frame;
    if (ImGui::MenuItem("Set from planar face..."))
    {
      select_row_(shape);
      m_view.shp_set_frame().begin(shape, Shp_set_frame::Pick::Planar_face);
      m_gui.set_mode(set_frame_mode);
    }

    if (ImGui::MenuItem("Set from cylindrical face..."))
    {
      select_row_(shape);
      m_view.shp_set_frame().begin(shape, Shp_set_frame::Pick::Cylindrical_face);
      m_gui.set_mode(set_frame_mode);
    }

    if (ImGui::MenuItem("Flip up"))
    {
      select_row_(shape);
      gp_Ax3 f = shape->is_workbench() ? shape->get_local_frame() : shape->get_frame();
      f.XReverse();
      m_view.set_shape_frame(shape, f);
    }

    if (ImGui::MenuItem("Flip axis (Z)"))
    {
      select_row_(shape);
      gp_Ax3 f = shape->is_workbench() ? shape->get_local_frame() : shape->get_frame();
      f.ZReverse();
      m_view.set_shape_frame(shape, f);
    }
  }

  if (m_kind == Shp_list_kind::Design && ImGui::MenuItem("Add to Workbench"))
  {
    const Status st = m_view.add_to_workbench({shape});
    if (!st.is_ok())
      m_gui.show_status(st);
    else
      m_gui.show_message("Added to Workbench.", Status_msg::Success);
  }

  if (ImGui::MenuItem("Delete"))
    to_delete = shape;
}

void Shape_list_row_drawer::accept_reparent_drop_(const Shp_ptr& shape, bool is_group)
{
  if (!ImGui::BeginDragDropTarget())
    return;

  const char* payload_type = m_kind == Shp_list_kind::Workbench ? "EZY_WBK_ID" : "EZY_SHAPE_ID";
  if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payload_type))
  {
    Shape_id drag_id = 0;
    std::memcpy(&drag_id, payload->Data, sizeof(drag_id));
    const Shape_id new_parent = is_group ? shape->get_id() : shape->get_parent_id();
    if (m_kind == Shp_list_kind::Workbench)
      (void)m_view.reparent_workbench_shape(drag_id, new_parent, -1, true);
    else
      (void)m_view.reparent_shape(drag_id, new_parent, -1, true);
  }

  if (m_kind == Shp_list_kind::Workbench)
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("EZY_SHAPE_ID"))
    {
      Shape_id drag_id = 0;
      std::memcpy(&drag_id, payload->Data, sizeof(drag_id));
      if (Shp_ptr src = m_view.find_design_shape_by_id(drag_id); !src.IsNull())
      {
        if (is_group)
          m_view.set_current_workbench_group_id(shape->get_id());

        const Status st = m_view.add_to_workbench({src});
        if (!st.is_ok())
          m_gui.show_status(st);
      }
    }
  ImGui::EndDragDropTarget();
}

void Shape_list_row_drawer::draw(const Shp_ptr& shape)
{
  EZY_ASSERT(shape);
  if (!m_ancestors.insert(shape->get_id()).second)
    return; // Parent cycle: skip rather than hang the Shape List.

  const bool                 is_group         = shape->is_group();
  const std::vector<Shp_ptr> children         = children_(shape->get_id());
  const bool                 has_children     = !children.empty();
  const Shape_id             cur_group =
      m_kind == Shp_list_kind::Workbench ? m_view.current_workbench_group_id() : m_view.current_group_id();
  const bool                 is_current_group = is_group && shape->get_id() == cur_group;
  // Selection highlight follows the 3D viewer only. Current group uses a distinct tint so
  // Alt-drag / clear-selection cannot look like the group (or its children) stayed selected.
  const bool row_selected = row_is_selected_(shape);
  bool       row_hovered  = false;

  ImGui::PushID(static_cast<int>(shape->get_id()));
  ImGui::TableNextRow();
  if (row_selected)
    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGuiCol_Header, 0.45f));
  else if (is_current_group)
    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGuiCol_Header, 0.18f));

  char name_buffer[1024];
  safe_cstr_copy(name_buffer, sizeof(name_buffer), shape->get_name().c_str());

  int mat_idx = 0;
  if (!is_group)
  {
    mat_idx = shape->Material();
    if (mat_idx < 0 || mat_idx >= m_nmat)
      mat_idx = static_cast<int>(m_view.get_default_material().Name());
  }

  // Columns 0-2: fixed actions on the left (no tree indent).
  ImGui::TableSetColumnIndex(0);
  // Full-row hit target under the widgets so padding / gaps between controls
  // still select and open the context menu (widgets draw on top via AllowOverlap).
  const ImVec2 cell0_pos = ImGui::GetCursorScreenPos();
  ImGui::Selectable("##row_hit", false, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap);
  row_hovered |= ImGui::IsItemHovered();
  if (ImGui::IsItemClicked())
    select_row_(shape);
  ImGui::SetCursorScreenPos(cell0_pos);

  bool visible = shape->get_visible();
  if (ImGui::Checkbox("##vis", &visible))
  {
    if (!visible && m_view.shape_list_hover() == shape)
      m_view.set_shape_list_hover(nullptr);

    shape->set_visible(visible);
    m_view.sync_sketch_shape_faint_style();
  }
  row_hovered |= ImGui::IsItemHovered();
  if (m_gui.ui_show_contextual_help() && ImGui::IsItemHovered())
    ImGui::SetTooltip(is_group ? "Show/hide group subtree" : "visibility");

  ImGui::TableSetColumnIndex(1);
  if (is_group)
    ImGui::TextUnformatted("");
  else
  {
    bool shaded = shape->get_disp_mode() == AIS_Shaded;
    if (ImGui::Checkbox("##shaded", &shaded))
      shape->set_disp_mode(shaded ? AIS_Shaded : AIS_WireFrame);

    row_hovered |= ImGui::IsItemHovered();
    if (m_gui.ui_show_contextual_help() && ImGui::IsItemHovered())
      ImGui::SetTooltip("solid/wire");
  }

  ImGui::TableSetColumnIndex(2);
  if (is_group)
    ImGui::TextUnformatted("");
  else
  {
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, ImGui::GetStyle().FramePadding.y));
    if (ImGui::Button("M"))
      ImGui::OpenPopup("mat_pick");

    ImGui::PopStyleVar();
    row_hovered |= ImGui::IsItemHovered();
    if (m_gui.ui_show_contextual_help() && ImGui::IsItemHovered())
      ImGui::SetTooltip("%s\n(click: material; right-click row: menu)", m_mat_names[static_cast<size_t>(mat_idx)].c_str());

    ImGui::SetNextWindowSize(ImVec2(m_mat_popup_w, 0.0f), ImGuiCond_Appearing);
    if (ImGui::BeginPopup("mat_pick"))
    {
      ImGui::TextUnformatted("Material");
      ImGui::Separator();
      const float max_h = ImGui::GetTextLineHeightWithSpacing() * 12.0f;
      const float sc_w  = std::max(1.0f, ImGui::GetContentRegionAvail().x);
      if (ImGui::BeginChild("mat_sc", ImVec2(sc_w, max_h), ImGuiChildFlags_Borders, ImGuiWindowFlags_AlwaysVerticalScrollbar))
      {
        for (int i = 0; i < m_nmat; ++i)
          if (ImGui::Selectable(m_mat_names[static_cast<size_t>(i)].c_str(), i == mat_idx))
          {
            apply_material_(shape, i);
            ImGui::CloseCurrentPopup();
          }

        ImGui::EndChild();
      }

      ImGui::EndPopup();
    }
  }

  // Column 3: tree arrow + name (indent applies here so hierarchy stays under the name).
  ImGui::TableSetColumnIndex(3);
  ImGui::AlignTextToFramePadding();

  ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_OpenOnArrow |
                                  ImGuiTreeNodeFlags_DrawLinesToNodes | ImGuiTreeNodeFlags_NoTreePushOnOpen;
  if (!has_children)
    node_flags |= ImGuiTreeNodeFlags_Leaf;

  if (row_selected)
    node_flags |= ImGuiTreeNodeFlags_Selected;

  bool open = true;
  if (has_children)
  {
    const auto exp_it = expanded_().find(shape->get_id());
    open              = (exp_it == expanded_().end()) ? true : exp_it->second;
    ImGui::SetNextItemOpen(open);
  }

  ImGui::SetNextItemAllowOverlap();
  const bool node_open = ImGui::TreeNodeEx("##node", node_flags);
  if (has_children && node_open != open)
    expanded_()[shape->get_id()] = node_open;

  if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
    select_row_(shape);

  row_hovered |= ImGui::IsItemHovered();
  if (m_gui.ui_show_contextual_help() && ImGui::IsItemHovered())
  {
    if (row_selected)
      ImGui::SetTooltip("Selected in 3D viewer");
    else if (is_current_group)
      ImGui::SetTooltip("Current group (new shapes go here)");
  }

  const char* drag_type = m_kind == Shp_list_kind::Workbench ? "EZY_WBK_ID" : "EZY_SHAPE_ID";
  if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
  {
    const Shape_id drag_id = shape->get_id();
    ImGui::SetDragDropPayload(drag_type, &drag_id, sizeof(drag_id));
    ImGui::TextUnformatted(shape->get_name().c_str());
    ImGui::EndDragDropSource();
  }

  // Drop onto group -> that group; onto solid -> solid's parent. Register on both the
  // tree arrow and the name field - the name covers most of the row.
  accept_reparent_drop_(shape, is_group);

  ImGui::SameLine();
  ImGui::SetNextItemWidth(std::max(1.0f, ImGui::GetContentRegionAvail().x));
  if (ImGui::InputText("##name", name_buffer, sizeof(name_buffer)))
    shape->set_name(std::string(name_buffer));

  if (ImGui::IsItemClicked())
    select_row_(shape);

  if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
  {
    const Shape_id drag_id = shape->get_id();
    ImGui::SetDragDropPayload(drag_type, &drag_id, sizeof(drag_id));
    ImGui::TextUnformatted(shape->get_name().c_str());
    ImGui::EndDragDropSource();
  }
  accept_reparent_drop_(shape, is_group);

  row_hovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenOverlapped | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

  // Last-column clip would miss gaps between vis / shaded / M; test the full row.
  if (const ImGuiTable* table = ImGui::GetCurrentTable())
  {
    if (ImGui::TableGetHoveredColumn() >= 0)
    {
      const ImRect row_bb(ImVec2(table->WorkRect.Min.x, table->RowPosY1), ImVec2(table->WorkRect.Max.x, table->RowPosY2));
      row_hovered |= ImGui::IsMouseHoveringRect(row_bb.Min, row_bb.Max, false);
    }
  }

  const ImGuiPayload* dd           = ImGui::GetDragDropPayload();
  const bool          dragging_row = dd != nullptr && (dd->IsDataType("EZY_SHAPE_ID") || dd->IsDataType("EZY_WBK_ID"));
  if (row_hovered && !dragging_row && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
  {
    m_gui.ensure_task_(m_kind == Shp_list_kind::Workbench ? Task::Workbench : Task::Design);
    ImGui::OpenPopup("shape_row_ctx");
  }

  if (ImGui::BeginPopup("shape_row_ctx"))
  {
    m_gui.ensure_task_(m_kind == Shp_list_kind::Workbench ? Task::Workbench : Task::Design);
    draw_ctx_menu_(shape, is_group);
    ImGui::EndPopup();
  }

  if (row_hovered && shape->get_visible() && !is_group)
    hover = shape;

  if (has_children && node_open)
  {
    ImGui::TreePush(reinterpret_cast<const void*>(static_cast<uintptr_t>(shape->get_id())));
    for (const Shp_ptr& child : children)
      draw(child);
    ImGui::TreePop();
  }

  ImGui::PopID();
  m_ancestors.erase(shape->get_id());
}

} // namespace gui_shp_detail
