// Sketch viewer display: visibility, edge styling, sketch switching, list hover.
#include "sketch.h"

#include <Quantity_Color.hxx>
#include <V3d_Viewer.hxx>

#include "occt_view.h"
#include "utl.h"

void Sketch::update_edge_style_(AIS_Shape_ptr& shp)
{
  switch (m_edge_style)
  {
  case Edge_style::Full:
    shp->SetWidth(1.0);
    shp->SetColor(Quantity_Color(0.0, 1.0, 0.0, Quantity_TOC_RGB));
    shp->SetTransparency(0.0);
    break;

  case Edge_style::Background:
    shp->SetWidth(1.0);
    shp->SetColor(Quantity_Color(0.3, 0.3, 0.3, Quantity_TOC_RGB));
    shp->SetTransparency(0.7);
    break;

  default:
    EZY_ASSERT(false);
  }
}

void Sketch::update_originating_face_style()
{
  if (!m_originating_face)
    return;

  switch (m_edge_style)
  {
  case Edge_style::Full:
    m_originating_face->SetWidth(1.0);
    m_originating_face->SetColor(Quantity_Color(0.3, 0.0, 0.0, Quantity_TOC_RGB));
    m_originating_face->SetTransparency(0.0);
    break;

  case Edge_style::Background:
    m_originating_face->SetWidth(1.0);
    m_originating_face->SetColor(Quantity_Color(0.3, 0.3, 0.3, Quantity_TOC_RGB));
    m_originating_face->SetTransparency(0.7);
    break;

  default:
    EZY_ASSERT(false);
  }

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
}

void Sketch::set_edge_style(Edge_style style)
{
  m_edge_style = style;

  for (Edge& e : m_edges.edges())
    update_edge_style_(e.shp);

  update_originating_face_style();
}
