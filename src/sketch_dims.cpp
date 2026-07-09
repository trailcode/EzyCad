#include "sketch_dims.h"

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <Precision.hxx>
#include <imgui.h>

#include <functional>

#include <glm/glm.hpp>

#include "gui.h"
#include "mode.h"
#include "gui_occt_view.h"
#include "sketch.h"
#include "sketch_delta.h"
#include "sketch_edge.h"
#include "utl_geom.h"

using namespace glm;

namespace
{
struct Symmetric_edge_span
{
  gp_Pnt2d pt_a;
  gp_Pnt2d pt_b;
  double   full_len;
};

std::optional<Symmetric_edge_span> symmetric_edge_from_center(const gp_Pnt2d& center, const gp_Dir2d& dir, double full_len)
{
  if (full_len <= Precision::Confusion())
    return std::nullopt;

  const double   half = full_len * 0.5;
  const gp_Vec2d v(dir);
  return Symmetric_edge_span{center.Translated(-v * half), center.Translated(v * half), full_len};
}
} // namespace

Sketch_dims::Sketch_dims(Sketch& sketch)
    : m_sketch(sketch)
{
}

void Sketch_dims::begin_dimension_input() { m_show_dim_input = true; }

void Sketch_dims::begin_angle_input() { m_show_angle_input = true; }

void Sketch_dims::remove_displayed()
{
  for (Length_dimension& ld : m_length_dimensions)
    if (!ld.dim.IsNull())
      m_sketch.m_ctx.Remove(ld.dim, false);

  m_length_dimensions.clear();

  if (m_len_dim_rubber_shp)
    m_sketch.m_ctx.Remove(m_len_dim_rubber_shp, false);

  m_len_dim_rubber_shp.Nullify();
}

void Sketch_dims::clear_tmp_dim_anno()
{
  m_sketch.m_ctx.Remove(m_tmp_dim_anno, true);
  m_tmp_dim_anno.Nullify();
}

void Sketch_dims::on_finalize_elm_start()
{
  m_show_dim_input     = false;
  m_show_angle_input   = false;
  m_entered_edge_angle = std::nullopt;
  m_sketch.m_view.gui().hide_angle_edit();
  clear_tmp_dim_anno();
}

void Sketch_dims::on_clear_tmps() { clear_all(m_entered_edge_len, m_show_dim_input, m_entered_edge_angle, m_show_angle_input); }

void Sketch_dims::clear_typed_constraints()
{
  m_entered_edge_angle = std::nullopt;
  m_entered_edge_len   = std::nullopt;
  m_show_angle_input   = false;
  m_show_dim_input     = false;
}

std::optional<gp_Pnt> Sketch_dims::approx_sketch_interior_ref_3d_() const
{
  gp_XYZ acc(0., 0., 0.);
  size_t n = 0;
  for (size_t i = 0; i < m_sketch.m_nodes.size(); ++i)
  {
    if (m_sketch.m_nodes[i].deleted)
      continue;

    acc += m_sketch.to_3d_(i).XYZ();
    ++n;
  }
  if (n == 0)
    return std::nullopt;

  return gp_Pnt(acc / static_cast<double>(n));
}

void Sketch_dims::rebuild_length_dimension_display_(Length_dimension& d)
{
  if (!d.dim.IsNull())
    m_sketch.m_ctx.Remove(d.dim, false);

  d.dim = create_distance_annotation(m_sketch.m_nodes[d.node_idx_lo], m_sketch.m_nodes[d.node_idx_hi], m_sketch.m_pln,
                                     m_sketch.m_view.gui().length_dimension_style(), approx_sketch_interior_ref_3d_(),
                                     m_sketch.m_topo.dim_classifier_faces().empty() ? nullptr
                                                                                    : &m_sketch.m_topo.dim_classifier_faces());

  const double dist = m_sketch.m_nodes[d.node_idx_lo].Distance(m_sketch.m_nodes[d.node_idx_hi]);
  d.dim->SetCustomValue(dist / m_sketch.m_view.get_dimension_scale());
  if (d.flyout_offset.has_value() && *d.flyout_offset > 0.0)
  {
    const double f    = d.dim->GetFlyout();
    const double sign = f < 0.0 ? -1.0 : 1.0;
    d.dim->SetFlyout(static_cast<double>(sign * *d.flyout_offset));
  }

  if (m_sketch.m_visible && m_show_dims && d.visible)
    m_sketch.m_ctx.Display(d.dim, false);
}

void Sketch_dims::purge_stale_length_dimensions()
{
  for (auto it = m_length_dimensions.begin(); it != m_length_dimensions.end();)
  {
    const bool bad = it->node_idx_lo >= m_sketch.m_nodes.size() || it->node_idx_hi >= m_sketch.m_nodes.size() ||
                     m_sketch.m_nodes[it->node_idx_lo].deleted || m_sketch.m_nodes[it->node_idx_hi].deleted;
    if (bad)
    {
      if (!it->dim.IsNull())
        m_sketch.m_ctx.Remove(it->dim, true);

      it = m_length_dimensions.erase(it);
    }
    else
      ++it;
  }
}

void Sketch_dims::refresh_all_length_dimensions()
{
  for (Length_dimension& d : m_length_dimensions)
    rebuild_length_dimension_display_(d);
}

void Sketch_dims::remove_length_dimensions_referencing_node_(size_t node_idx)
{
  for (auto it = m_length_dimensions.begin(); it != m_length_dimensions.end();)
    if (it->node_idx_lo == node_idx || it->node_idx_hi == node_idx)
    {
      if (!it->dim.IsNull())
        m_sketch.m_ctx.Remove(it->dim, true);

      it = m_length_dimensions.erase(it);
    }
    else
      ++it;
}

void Sketch_dims::add_or_toggle_length_dim_between_node_indices_(size_t node_a, size_t node_b)
{
  const size_t lo = std::min(node_a, node_b);
  const size_t hi = std::max(node_a, node_b);
  if (lo == hi)
    return;

  for (auto it = m_length_dimensions.begin(); it != m_length_dimensions.end(); ++it)
    if (it->node_idx_lo == lo && it->node_idx_hi == hi)
    {
      Sketch_op_recorder rec(m_sketch.m_view, m_sketch);
      rec.note_prev_length_dim(lo, hi, it->visible, it->flyout_offset, it->name);
      if (!it->dim.IsNull())
        m_sketch.m_ctx.Remove(it->dim, true);

      m_length_dimensions.erase(it);
      rec.commit();
      return;
    }

  Sketch_op_recorder rec(m_sketch.m_view, m_sketch);
  {
    Length_dimension d;
    d.node_idx_lo = lo;
    d.node_idx_hi = hi;
    m_length_dimensions.push_back(std::move(d));
    rebuild_length_dimension_display_(m_length_dimensions.back());
    rec.note_curr_length_dim(lo, hi, m_length_dimensions.back().visible, m_length_dimensions.back().flyout_offset,
                             m_length_dimensions.back().name);
    rec.commit();
  }
}

void Sketch_dims::json_add_length_dimension_(size_t node_a, size_t node_b, const bool visible,
                                             const std::optional<double> flyout_offset, const std::string& name)
{
  const size_t lo = std::min(node_a, node_b);
  const size_t hi = std::max(node_a, node_b);
  if (lo == hi)
    return;

  for (const Length_dimension& x : m_length_dimensions)
    if (x.node_idx_lo == lo && x.node_idx_hi == hi)
      return;

  Length_dimension d;
  d.node_idx_lo   = lo;
  d.node_idx_hi   = hi;
  d.visible       = visible;
  d.flyout_offset = flyout_offset;
  d.name          = name;
  m_length_dimensions.push_back(std::move(d));
  rebuild_length_dimension_display_(m_length_dimensions.back());
}

bool Sketch_dims::try_remove_length_dimension(PrsDim_LengthDimension* dim)
{
  if (!dim)
    return false;

  for (auto it = m_length_dimensions.begin(); it != m_length_dimensions.end(); ++it)
    if (it->dim.get() == dim)
    {
      m_sketch.m_ctx.Remove(it->dim, true);
      m_length_dimensions.erase(it);
      return true;
    }

  return false;
}

void Sketch_dims::toggle_edge_dim_anno(const ScreenCoords& screen_coords)
{
  if (std::optional<size_t> n = m_sketch.m_nodes.try_pick_existing_node(screen_coords))
  {
    if (!m_len_dim_pick_anchor_node.has_value())
    {
      m_len_dim_pick_anchor_node = *n;
      update_len_dim_rubber_line_(screen_coords);
      return;
    }

    if (*m_len_dim_pick_anchor_node != *n)
      add_or_toggle_length_dim_between_node_indices_(*m_len_dim_pick_anchor_node, *n);

    clear_pick_state();
    return;
  }

  if (std::list<Sketch::Edge>::iterator itr = m_sketch.get_edge_at_(screen_coords); itr != m_sketch.m_edges.edges().end())
    if (sketch_edge_is_linear(*itr))
    {
      add_or_toggle_length_dim_between_node_indices_(itr->node_idx_a, *itr->node_idx_b);
      clear_pick_state();
      return;
    }

  clear_pick_state();
}

void Sketch_dims::clear_len_dim_rubber_line_()
{
  if (m_len_dim_rubber_shp)
  {
    m_sketch.m_ctx.Remove(m_len_dim_rubber_shp, false);
    m_len_dim_rubber_shp.Nullify();
  }
}

void Sketch_dims::clear_pick_state()
{
  m_len_dim_pick_anchor_node.reset();
  clear_len_dim_rubber_line_();
}

void Sketch_dims::update_len_dim_rubber_line_(const ScreenCoords& screen_coords)
{
  if (!m_len_dim_pick_anchor_node.has_value())
  {
    clear_len_dim_rubber_line_();
    return;
  }

  const gp_Pnt2d&         pt_a   = m_sketch.m_nodes[*m_len_dim_pick_anchor_node];
  std::optional<gp_Pnt2d> pt_opt = m_sketch.m_view.pt_on_plane(screen_coords, m_sketch.m_pln);
  if (!pt_opt)
    return;

  gp_Pnt2d pt_b = *pt_opt;
  (void)m_sketch.m_nodes.try_get_node_idx_snap(pt_b);

  if (!unique(pt_a, pt_b))
  {
    clear_len_dim_rubber_line_();
    return;
  }

  const TopoDS_Shape edge_shape = BRepBuilderAPI_MakeEdge(to_3d(m_sketch.m_pln, pt_a), to_3d(m_sketch.m_pln, pt_b)).Edge();

  if (m_len_dim_rubber_shp)
  {
    m_len_dim_rubber_shp->Set(edge_shape);
    m_sketch.update_edge_style_(m_len_dim_rubber_shp);
    m_sketch.m_ctx.Redisplay(m_len_dim_rubber_shp, true);
  }
  else
  {
    m_len_dim_rubber_shp = new AIS_Shape(edge_shape);
    m_sketch.update_edge_style_(m_len_dim_rubber_shp);
    m_sketch.m_ctx.Display(m_len_dim_rubber_shp, AIS_WireFrame, 0, true);
  }
}

void Sketch_dims::show_tmp_dim_preview(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b)
{
  m_sketch.m_ctx.Remove(m_tmp_dim_anno, true);
  if (!unique(pt_a, pt_b))
  {
    m_tmp_dim_anno.Nullify();
    return;
  }

  const double dist = pt_a.Distance(pt_b) / m_sketch.m_view.get_dimension_scale();
  m_tmp_dim_anno    = create_distance_annotation(
      pt_a, pt_b, m_sketch.m_pln, m_sketch.m_view.gui().length_dimension_style(), approx_sketch_interior_ref_3d_(),
      m_sketch.m_topo.dim_classifier_faces().empty() ? nullptr : &m_sketch.m_topo.dim_classifier_faces());
  m_tmp_dim_anno->SetCustomValue(dist);
  m_sketch.m_ctx.Display(m_tmp_dim_anno, true);
}

void Sketch_dims::offer_dist_edit_for_segment(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, double dist)
{
  if (!m_show_dim_input || !unique(pt_a, pt_b))
    return;

  const gp_Dir2d     edge_dir = get_unit_dir(pt_a, pt_b);
  const ScreenCoords spos     = m_sketch.m_view.get_screen_coords(to_3d(m_sketch.m_pln, center_point(pt_a, pt_b)));

  auto cb = [this, edge_dir](float new_dist, bool is_finial)
  {
    m_entered_edge_len = {edge_dir, new_dist * m_sketch.m_view.get_dimension_scale()};
    m_show_dim_input   = !is_finial;
    if (is_finial)
      m_sketch.on_enter();
  };

  m_sketch.m_view.gui().set_dist_edit(static_cast<float>(dist), std::move(std::function<void(float, bool)>(cb)), spos);
}

void Sketch_dims::offer_angle_edit_for_segment(const gp_Pnt2d& pt_a, const gp_Pnt2d& mouse_pt, double current_angle_deg)
{
  if (!m_show_angle_input)
    return;

  const ScreenCoords spos = m_sketch.m_view.get_screen_coords(to_3d(m_sketch.m_pln, center_point(pt_a, mouse_pt)));

  auto cb = [this](float new_angle, bool is_finial)
  {
    m_entered_edge_angle = new_angle;
    m_show_angle_input   = !is_finial;
    m_sketch.sketch_pt_move(m_sketch.m_view.gui().cursor_screen_coords());
  };

  const float angle_to_show = m_entered_edge_angle.has_value() ? float(*m_entered_edge_angle) : float(current_angle_deg);
  m_sketch.m_view.gui().set_angle_edit(angle_to_show, std::move(std::function<void(float, bool)>(cb)), spos);
}

void Sketch_dims::check_dimension_seg_(int kind)
{
  const auto linestring_type = static_cast<Sketch::Linestring_type>(kind);

  if (!m_entered_edge_len.has_value())
    return;

  if (m_entered_edge_len->len <= Precision::Confusion())
    return;

  EZY_ASSERT(m_sketch.m_tools.tmp_edges().size());

  Sketch_edge& edge = m_sketch.m_tools.tmp_edges().back();
  if (m_sketch.m_tools.edge_from_center_active() && linestring_type == Sketch::Linestring_type::Single)
  {
    const gp_Pnt2d&                    center = m_sketch.m_nodes[edge.node_idx_a];
    std::optional<Symmetric_edge_span> span =
        symmetric_edge_from_center(center, m_entered_edge_len->dir, m_entered_edge_len->len);

    if (span)
    {
      edge.node_idx_a = m_sketch.m_nodes.get_node_exact(span->pt_a);
      m_sketch.update_edge_end_pt_(edge, m_sketch.m_nodes.get_node_exact(span->pt_b));
      clear_all(m_entered_edge_len);
      m_sketch.finalize_elm();
    }
    return;
  }

  const gp_Pnt2d& pt_a       = m_sketch.m_nodes[edge.node_idx_a];
  m_sketch.m_tools.last_pt() = gp_Pnt2d(pt_a).Translated(gp_Vec2d(m_entered_edge_len->dir) * m_entered_edge_len->len);

  m_sketch.update_edge_end_pt_(edge, m_sketch.m_nodes.get_node_exact(*m_sketch.m_tools.last_pt()));

  EZY_ASSERT(edge.node_idx_b.has_value());

  clear_all(m_entered_edge_len);

  if (linestring_type == Sketch::Linestring_type::Single)
    m_sketch.finalize_elm();

  else if (linestring_type == Sketch::Linestring_type::Two)
  {
    switch (m_sketch.m_tools.tmp_edges().size())
    {
    case 1:
      m_entered_edge_angle = std::nullopt;
      m_show_angle_input   = false;
      m_sketch.m_view.gui().hide_angle_edit();
      m_sketch.m_tools.tmp_edges().push_back({*edge.node_idx_b});
      break;

    case 2:
      m_sketch.finalize_elm();
      break;

    default:
      EZY_ASSERT(false);
    }
  }
  else
  {
    m_entered_edge_angle = std::nullopt;
    m_show_angle_input   = false;
    m_sketch.m_view.gui().hide_angle_edit();
    m_sketch.m_tools.tmp_edges().push_back({*edge.node_idx_b});
  }
}

void Sketch_dims::check_dimension_rubber_()
{
  if (!m_entered_edge_len.has_value())
    return;

  if (m_entered_edge_len->len <= Precision::Confusion())
    return;

  if (m_sketch.m_tools.tmp_edges().empty())
    return;

  Sketch_edge&    edge       = m_sketch.m_tools.tmp_edges().back();
  const gp_Pnt2d& pt_a       = m_sketch.m_nodes[edge.node_idx_a];
  m_sketch.m_tools.last_pt() = gp_Pnt2d(pt_a).Translated(gp_Vec2d(m_entered_edge_len->dir) * m_entered_edge_len->len);

  if (!unique(pt_a, *m_sketch.m_tools.last_pt()))
    return;

  const Mode mode = m_sketch.get_mode();
  if (mode == Mode::Sketch_add_square || mode == Mode::Sketch_add_circle || mode == Mode::Sketch_add_rectangle ||
      mode == Mode::Sketch_add_rectangle_center_pt)
  {
    m_sketch.update_edge_end_pt_(edge, m_sketch.m_nodes.get_node_exact(*m_sketch.m_tools.last_pt()));
    EZY_ASSERT(edge.node_idx_b.has_value());
    clear_all(m_entered_edge_len);
    m_sketch.finalize_elm();
    return;
  }

  EZY_ASSERT(mode == Mode::Sketch_add_node);

  Sketch_op_recorder rec(m_sketch.m_view, m_sketch);
  {
    const size_t b = m_sketch.m_nodes.get_node_exact(*m_sketch.m_tools.last_pt(), true);
    clear_all(m_entered_edge_len);

    rec.note_curr_node(b);
    m_sketch.m_topo.split_linear_edges_at_node_if_interior(b, rec);
    m_sketch.m_topo.split_arcs_at_node_if_interior(b, rec);

    m_sketch.m_tools.clear_tmp_node_idxs();
    m_sketch.m_tools.clear_tmps();
    clear_tmp_dim_anno();
    m_sketch.m_nodes.hide_snap_annos();
    m_sketch.update_faces_();
    rec.commit();
  }
}

size_t Sketch_dims::length_dimension_count() const { return m_length_dimensions.size(); }

std::vector<std::string> Sketch_dims::inspector_dimension_labels() const
{
  std::vector<std::string> labels;
  labels.reserve(m_length_dimensions.size());
  for (size_t i = 0; i < m_length_dimensions.size(); ++i)
  {
    const Length_dimension& d   = m_length_dimensions[i];
    std::string             lbl = d.name.empty() ? ("D" + std::to_string(i)) : d.name;
    labels.push_back(std::move(lbl));
  }

  return labels;
}

bool Sketch_dims::dimension_visible(size_t dim_index) const
{
  EZY_ASSERT(dim_index < m_length_dimensions.size());
  return m_length_dimensions[dim_index].visible;
}

void Sketch_dims::set_dimension_visible(size_t dim_index, bool visible)
{
  EZY_ASSERT(dim_index < m_length_dimensions.size());
  Length_dimension& d = m_length_dimensions[dim_index];
  if (d.visible == visible)
    return;

  d.visible = visible;
  if (!d.dim.IsNull())
  {
    if (visible && m_sketch.m_visible && m_show_dims)
      m_sketch.m_ctx.Display(d.dim, false);
    else
      m_sketch.m_ctx.Erase(d.dim, false);
  }
}

size_t Sketch_dims::dimension_node_lo(size_t dim_index) const
{
  EZY_ASSERT(dim_index < m_length_dimensions.size());
  return m_length_dimensions[dim_index].node_idx_lo;
}

size_t Sketch_dims::dimension_node_hi(size_t dim_index) const
{
  EZY_ASSERT(dim_index < m_length_dimensions.size());
  return m_length_dimensions[dim_index].node_idx_hi;
}

double Sketch_dims::dimension_offset(size_t dim_index) const
{
  EZY_ASSERT(dim_index < m_length_dimensions.size());
  const Length_dimension& d = m_length_dimensions[dim_index];
  if (d.flyout_offset.has_value())
    return *d.flyout_offset;

  if (!d.dim.IsNull())
    return std::abs(static_cast<double>(d.dim->GetFlyout()));

  return 0.0;
}

void Sketch_dims::set_dimension_offset(size_t dim_index, const double offset)
{
  EZY_ASSERT(dim_index < m_length_dimensions.size());
  Length_dimension& d = m_length_dimensions[dim_index];

  const double off = std::max(0.0, offset);
  if (off <= 0.0)
    d.flyout_offset.reset();
  else
    d.flyout_offset = off;

  rebuild_length_dimension_display_(d);
}

std::string Sketch_dims::dimension_name(size_t dim_index) const
{
  EZY_ASSERT(dim_index < m_length_dimensions.size());
  return m_length_dimensions[dim_index].name;
}

void Sketch_dims::set_dimension_name(size_t dim_index, const std::string& name)
{
  EZY_ASSERT(dim_index < m_length_dimensions.size());
  Length_dimension& d = m_length_dimensions[dim_index];
  if (d.name == name)
    return;

  d.name = name;
}

PrsDim_LengthDimension_ptr Sketch_dims::length_dimension_handle(const size_t dim_index) const
{
  EZY_ASSERT(dim_index < m_length_dimensions.size());
  return m_length_dimensions[dim_index].dim;
}

void Sketch_dims::set_show_dims(bool show)
{
  m_show_dims = show;
  if (show && m_sketch.m_visible)
  {
    for (Length_dimension& ld : m_length_dimensions)
      if (!ld.dim.IsNull() && ld.visible)
        m_sketch.m_ctx.Display(ld.dim, false);
  }
  else
  {
    for (Length_dimension& ld : m_length_dimensions)
      if (!ld.dim.IsNull())
        m_sketch.m_ctx.Erase(ld.dim, false);
  }
}

bool Sketch_dims::shows_dimensions() const { return m_show_dims; }

void Sketch_dims::on_sketch_shown()
{
  if (m_len_dim_rubber_shp && m_len_dim_pick_anchor_node.has_value())
    m_sketch.m_ctx.Display(m_len_dim_rubber_shp, AIS_WireFrame, 0, false);

  if (m_show_dims)
    for (Length_dimension& ld : m_length_dimensions)
      if (!ld.dim.IsNull() && ld.visible)
        m_sketch.m_ctx.Display(ld.dim, false);
}

void Sketch_dims::on_sketch_hidden()
{
  for (Length_dimension& ld : m_length_dimensions)
    if (!ld.dim.IsNull())
      m_sketch.m_ctx.Erase(ld.dim, false);
}
