// Sketch viewer display: visibility, edge styling, sketch switching, list hover.
#include "sketch.h"

#include <Aspect_TypeOfLine.hxx>
#include <Aspect_InteriorStyle.hxx>
#include <AIS_DisplayMode.hxx>
#include <Graphic3d_AspectFillArea3d.hxx>
#include <Graphic3d_NameOfMaterial.hxx>
#include <Prs3d_Drawer.hxx>
#include <Prs3d_LineAspect.hxx>
#include <Prs3d_ShadingAspect.hxx>
#include <Quantity_Color.hxx>
#include <V3d_Viewer.hxx>
#include <algorithm>

#include "gui.h"
#include "gui_occt_view.h"
#include "utl.h"

namespace
{
constexpr float k_originating_face_full_rgba[4] = {0.3f, 0.0f, 0.0f, 1.0f};
/// Fixed style for non-current sketches (not user-configurable).
constexpr float k_background_edge_rgba[4]   = {0.3f, 0.3f, 0.3f, 0.3f};
constexpr float k_background_face_rgba[4]   = {0.3f, 0.3f, 0.3f, 0.2f};
constexpr float k_edge_highlight_line_width = 2.0f;

Quantity_Color rgb_from_rgba_(const float* rgba)
{
  return Quantity_Color(static_cast<double>(rgba[0]), static_cast<double>(rgba[1]), static_cast<double>(rgba[2]),
                        Quantity_TOC_RGB);
}

float transparency_from_rgba_(const float* rgba) { return std::clamp(1.f - rgba[3], 0.f, 1.f); }

void apply_rgba_style_(AIS_Shape& shp, const float* rgba, float line_width)
{
  shp.SetWidth(static_cast<double>(line_width));
  shp.SetColor(rgb_from_rgba_(rgba));
  shp.SetTransparency(static_cast<double>(transparency_from_rgba_(rgba)));
}

Prs3d_Drawer_ptr make_edge_hilight_drawer_(const float* rgba, float line_width)
{
  Prs3d_Drawer_ptr     drawer = new Prs3d_Drawer();
  const Quantity_Color qc     = rgb_from_rgba_(rgba);
  drawer->SetColor(qc);
  drawer->SetTransparency(transparency_from_rgba_(rgba));
  Prs3d_LineAspect_ptr line = new Prs3d_LineAspect(qc, Aspect_TOL_SOLID, static_cast<double>(line_width));
  drawer->SetLineAspect(line);
  drawer->SetWireAspect(line);
  drawer->SetSeenLineAspect(line);
  drawer->SetFaceBoundaryAspect(line);
  return drawer;
}

Prs3d_Drawer_ptr make_face_hilight_drawer_(const float* rgba)
{
  Prs3d_Drawer_ptr drawer = new Prs3d_Drawer();
  drawer->SetupOwnDefaults();
  const Quantity_Color qc     = rgb_from_rgba_(rgba);
  const float          transp = transparency_from_rgba_(rgba);
  drawer->SetColor(qc);
  drawer->SetTransparency(transp);

  Prs3d_ShadingAspect_ptr shading = new Prs3d_ShadingAspect();
  shading->SetColor(qc);
  shading->SetTransparency(static_cast<double>(transp));
  drawer->SetShadingAspect(shading);

  Graphic3d_AspectFillArea3d_ptr fill = new Graphic3d_AspectFillArea3d();
  fill->SetAlphaMode(Graphic3d_AlphaMode_Blend);
  fill->SetInteriorStyle(Aspect_IS_SOLID);
  fill->SetInteriorColor(qc);
  fill->SetColor(qc);
  drawer->SetBasicFillAreaAspect(fill);

  Prs3d_LineAspect_ptr line = new Prs3d_LineAspect(qc, Aspect_TOL_SOLID, 2.0);
  drawer->SetWireAspect(line);
  drawer->SetFaceBoundaryAspect(line);
  return drawer;
}

void apply_edge_hilight_(AIS_Shape& shp, const GUI& gui)
{
  Prs3d_Drawer_ptr selected = make_edge_hilight_drawer_(gui.sketch_edge_selection_color_rgba(), k_edge_highlight_line_width);
  Prs3d_Drawer_ptr hover    = make_edge_hilight_drawer_(gui.sketch_edge_highlight_color_rgba(), k_edge_highlight_line_width);
  shp.SetHilightAttributes(selected);
  shp.SetDynamicHilightAttributes(hover);
}

void apply_face_hilight_(AIS_Shape& shp, const GUI& gui)
{
  // Use shaded hilight so selection/hover tint the face fill, not only a wire outline.
  shp.SetHilightMode(AIS_Shaded);
  Prs3d_Drawer_ptr selected = make_face_hilight_drawer_(gui.sketch_face_selection_color_rgba());
  Prs3d_Drawer_ptr hover    = make_face_hilight_drawer_(gui.sketch_face_highlight_color_rgba());
  shp.SetHilightAttributes(selected);
  shp.SetDynamicHilightAttributes(hover);
}
} // namespace

void Sketch::update_edge_style_(const AIS_Shape_ptr& shp)
{
  if (shp.IsNull())
    return;

  const GUI& gui = m_view.gui();
  switch (m_edge_style)
  {
  case Edge_style::Full:
    apply_rgba_style_(*shp, gui.sketch_edge_color_rgba(), gui.sketch_edge_line_width());
    break;

  case Edge_style::Background:
    apply_rgba_style_(*shp, k_background_edge_rgba, gui.sketch_edge_line_width());
    break;

  default:
    EZY_ASSERT(false);
  }

  apply_edge_hilight_(*shp, gui);
}

void Sketch::update_face_style_(const AIS_Shape_ptr& shp)
{
  if (shp.IsNull())
    return;

  const GUI& gui = m_view.gui();
  switch (m_edge_style)
  {
  case Edge_style::Full:
    apply_rgba_style_(*shp, gui.sketch_face_color_rgba(), 1.0f);
    break;

  case Edge_style::Background:
    apply_rgba_style_(*shp, k_background_face_rgba, 1.0f);
    break;

  default:
    EZY_ASSERT(false);
  }

  shp->SetMaterial(Graphic3d_NOM_PLASTIC);
  apply_face_hilight_(*shp, gui);
}

void Sketch::update_all_face_styles_()
{
  for (const Sketch_face_shp_ptr& face : m_topo.faces())
  {
    update_face_style_(face);
    if (!face.IsNull())
      m_ctx.Redisplay(face, false);
  }
}

void Sketch::update_originating_face_style()
{
  if (!m_originating_face)
    return;

  const GUI& gui = m_view.gui();
  switch (m_edge_style)
  {
  case Edge_style::Full:
    // Originating face wire stays a distinct dark-red cue for "from face" sketches.
    apply_rgba_style_(*m_originating_face, k_originating_face_full_rgba, gui.sketch_edge_line_width());
    break;

  case Edge_style::Background:
    apply_rgba_style_(*m_originating_face, k_background_edge_rgba, gui.sketch_edge_line_width());
    break;

  default:
    EZY_ASSERT(false);
  }

  apply_edge_hilight_(*m_originating_face, gui);
  m_ctx.Redisplay(m_originating_face, true);
}

void Sketch::set_visible(bool state)
{
  m_visible = state;

  if (state)
  {
    if (m_show_faces)
      for (AIS_Shape_ptr& face : m_topo.faces())
        m_ctx.Display(face, AIS_Shaded, 0, false);

    for (Edge& e : m_edges.edges())
      m_ctx.Display(e.shp, AIS_WireFrame, 0, false);

    m_dims.on_sketch_shown();

    if (m_originating_face)
      m_ctx.Display(m_originating_face, AIS_WireFrame, 0, false);

    if (m_underlay.has_image())
      m_underlay.rebuild_and_display(m_pln);

    sync_operation_axis_display_();
    m_node_marks.sync();
  }
  else
  {
    for (AIS_Shape_ptr& face : m_topo.faces())
      m_ctx.Erase(face, false);

    for (Edge& e : m_edges.edges())
      m_ctx.Erase(e.shp, false);

    m_dims.on_sketch_hidden();

    if (m_originating_face)
      m_ctx.Erase(m_originating_face, false);

    m_underlay.ctx_erase();

    if (m_operation_axis.has_value())
      m_ctx.Erase(m_operation_axis->shp, false);

    m_nodes.hide_snap_annos();
    cancel_elm();

    m_node_marks.erase_all();
  }

  m_ctx.UpdateCurrentViewer();

  m_view.curr_sketch().set_current();
}

bool Sketch::is_visible() const { return m_visible; }

void Sketch::set_show_faces(bool show)
{
  if (show && m_visible)
    for (AIS_Shape_ptr& face : m_topo.faces())
      m_ctx.Display(face, AIS_Shaded, 0, false);
  else
    for (AIS_Shape_ptr& face : m_topo.faces())
      m_ctx.Erase(face, false);

  m_show_faces = show;
}

void Sketch::set_show_edges(bool show)
{
  if (show && m_visible)
    for (Edge& e : m_edges.edges())
      m_ctx.Display(e.shp, AIS_WireFrame, 0, false);
  else
    for (Edge& e : m_edges.edges())
      m_ctx.Erase(e.shp, false);
}

void Sketch::append_list_hover_ais(std::vector<AIS_InteractiveObject_ptr>& out) const
{
  if (!m_visible)
    return;

  for (const Edge& e : m_edges.edges())
    if (!e.shp.IsNull())
      out.push_back(e.shp);

  if (m_show_faces)
    for (const Sketch_face_shp_ptr& face : m_topo.faces())
      if (!face.IsNull())
        out.push_back(face);

  if (m_originating_face)
    out.push_back(m_originating_face);

  if (m_operation_axis.has_value() && !m_operation_axis->shp.IsNull())
    out.push_back(m_operation_axis->shp);

  if (m_underlay.has_image() && m_underlay.visible())
    m_underlay.append_list_hover_ais(out);
}

bool Sketch::is_current() const { return m_view.current_sketch_if_any() == this; }

void Sketch::set_current()
{
  m_ctx.CurrentViewer()->SetPrivilegedPlane(m_pln.Position());
  set_edge_style(Edge_style::Full);
  m_nodes.clear_outside_snap_pnts();

  m_tmp_pts_3d.clear();
  get_originating_face_snp_pts_3d_(m_tmp_pts_3d);

  for (const gp_Pnt& pt3d : m_tmp_pts_3d)
    m_nodes.add_outside_snap_pnt(pt3d);

  for (Sketch_ptr& sketch : m_view.get_sketches())
    if (sketch.get() != this)
    {
      sketch->set_edge_style(Edge_style::Background);

      if (sketch->m_operation_axis.has_value())
        m_ctx.Erase(sketch->m_operation_axis->shp, false);

      if (sketch->is_visible())
      {
        sketch->get_snap_pts_3d_(m_tmp_pts_3d);
        for (const gp_Pnt& pt3d : m_tmp_pts_3d)
          m_nodes.add_outside_snap_pnt(pt3d);
      }
    }

  sync_operation_axis_display_();

  for (Sketch_ptr& sketch : m_view.get_sketches())
    if (sketch->is_visible())
      sketch->m_node_marks.sync();
}

void Sketch::set_edge_style(Edge_style style)
{
  m_edge_style = style;

  for (Edge& e : m_edges.edges())
  {
    update_edge_style_(e.shp);
    if (!e.shp.IsNull())
      m_ctx.Redisplay(e.shp, false);
  }

  update_all_face_styles_();
  update_originating_face_style();
}
