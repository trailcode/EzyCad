#include "skt_node_marks.h"

#include <Precision.hxx>
#include <Quantity_Color.hxx>

#include "gui.h"
#include "mode.h"
#include "gui_occt_view.h"
#include "sketch.h"
#include "skt_nodes.h"
#include "skt_topo.h"
#include "utl_geom.h"

Sketch_node_marks::Sketch_node_marks(Sketch& sketch)
    : m_sketch(sketch)
{
}

void Sketch_node_marks::apply_style_(AIS_Shape_ptr& shp, bool origin) const
{
  // Keep permanent node markers visually stable across sketch switching.
  shp->SetWidth(1.25);
  if (origin)
  {
    const float* c = m_sketch.m_view.gui().origin_marker_color_rgb();
    shp->SetColor(Quantity_Color(c[0], c[1], c[2], Quantity_TOC_RGB));
  }
  else
    shp->SetColor(Quantity_NOC_RED);
  shp->SetTransparency(0.0);
}

void Sketch_node_marks::sync()
{
  if (m_marks.size() < m_sketch.m_nodes.size())
    m_marks.resize(m_sketch.m_nodes.size());

  const double size_scale = std::max(static_cast<double>(m_sketch.m_view.gui().permanent_node_anno_scale()), 0.0);
  const double half_arm =
      std::max(m_sketch.m_topo.plane_pick_snap_radius_world() * 0.45 * size_scale, Precision::Confusion() * 50.0);

  const Mode mode = m_sketch.get_mode();
  // Show "+" markers for permanent user nodes in sketch modes and polar duplicate (which snaps to sketch nodes).
  // Hide during face extrude so face selection is unobstructed.
  const bool show_permanent_marks =
      mode != Mode::Sketch_face_extrude && (is_sketch_mode(mode) || mode == Mode::Shape_polar_duplicate);
  const bool hide_for_operation_axis = m_sketch.operation_axis_suppresses_sketch_snap_();

  for (size_t i = 0, n = m_sketch.m_nodes.size(); i < n; ++i)
  {
    const Sketch_nodes::Node& node = m_sketch.m_nodes[i];
    bool show = m_sketch.m_visible && node.permanent && !node.deleted && show_permanent_marks && !hide_for_operation_axis;
    if (node.origin)
      show = show && m_sketch.is_current() && m_sketch.m_show_origin_marker;

    if (!show)
    {
      if (m_marks[i])
      {
        m_sketch.m_ctx.Remove(m_marks[i], false);
        m_marks[i].Nullify();
      }
      continue;
    }

    const gp_Pnt2d     p2(node.X(), node.Y());
    const gp_Pnt       c3     = to_3d(m_sketch.m_pln, p2);
    const TopoDS_Shape marker = node.origin ? create_origin_marker_shape(m_sketch.m_pln, c3, half_arm)
                                            : create_plus_cross_shape(m_sketch.m_pln, c3, half_arm);

    if (m_marks[i])
    {
      m_marks[i]->Set(marker);
      apply_style_(m_marks[i], node.origin);
      m_sketch.m_ctx.Redisplay(m_marks[i], true);
      if (node.origin)
        m_sketch.m_ctx.Deactivate(m_marks[i]);
    }
    else
    {
      Sketch_AIS_node_mark_ptr mk = new Sketch_AIS_node_mark(m_sketch, i, marker);
      apply_style_(mk, node.origin);
      m_marks[i] = mk;
      m_sketch.m_ctx.Display(mk, AIS_WireFrame, 0, false);
      if (node.origin)
        m_sketch.m_ctx.Deactivate(mk);
    }
  }
}

void Sketch_node_marks::remove_at(size_t node_idx)
{
  if (node_idx >= m_marks.size() || m_marks[node_idx].IsNull())
    return;

  m_sketch.m_ctx.Remove(m_marks[node_idx], false);
  m_marks[node_idx].Nullify();
}

void Sketch_node_marks::remove_all()
{
  for (Sketch_AIS_node_mark_ptr& mk : m_marks)
    if (mk)
      m_sketch.m_ctx.Remove(mk, false);

  m_marks.clear();
}

void Sketch_node_marks::erase_all()
{
  for (Sketch_AIS_node_mark_ptr& mk : m_marks)
    if (mk)
      m_sketch.m_ctx.Erase(mk, false);
}

void Sketch_node_marks::trim_trailing()
{
  while (m_marks.size() > m_sketch.m_nodes.size())
  {
    const size_t last = m_marks.size() - 1;
    if (m_marks[last])
      m_sketch.m_ctx.Remove(m_marks[last], false);

    m_marks.pop_back();
  }
}

Sketch_AIS_node_mark_ptr Sketch_node_marks::get(size_t node_idx) const
{
  if (node_idx >= m_marks.size())
    return {};
  return m_marks[node_idx];
}

Sketch_AIS_node_mark_ptr Sketch_node_marks::ensure_for_list_hover(size_t node_idx)
{
  if (node_idx >= m_sketch.m_nodes.size())
    return {};

  const Sketch_nodes::Node& node = m_sketch.m_nodes[node_idx];
  if (!node.permanent || node.deleted)
    return {};

  if (m_marks.size() <= node_idx)
    m_marks.resize(node_idx + 1);

  const double size_scale = std::max(static_cast<double>(m_sketch.m_view.gui().permanent_node_anno_scale()), 0.0);
  const double half_arm =
      std::max(m_sketch.m_topo.plane_pick_snap_radius_world() * 0.45 * size_scale, Precision::Confusion() * 50.0);

  const gp_Pnt2d     p2(node.X(), node.Y());
  const gp_Pnt       c3     = to_3d(m_sketch.m_pln, p2);
  const TopoDS_Shape marker = node.origin ? create_origin_marker_shape(m_sketch.m_pln, c3, half_arm)
                                          : create_plus_cross_shape(m_sketch.m_pln, c3, half_arm);

  if (m_marks[node_idx])
  {
    m_marks[node_idx]->Set(marker);
    apply_style_(m_marks[node_idx], node.origin);
    if (m_sketch.m_ctx.IsDisplayed(m_marks[node_idx]))
      m_sketch.m_ctx.Redisplay(m_marks[node_idx], false);
  }
  else
  {
    Sketch_AIS_node_mark_ptr mk = new Sketch_AIS_node_mark(m_sketch, node_idx, marker);
    apply_style_(mk, node.origin);
    m_marks[node_idx] = mk;
  }

  if (node.origin)
    m_sketch.m_ctx.Deactivate(m_marks[node_idx]);

  return m_marks[node_idx];
}
