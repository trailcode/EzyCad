#include "skt_tools.h"

#include <BRep_Builder.hxx>
#include <Precision.hxx>
#include <Quantity_NameOfColor.hxx>
#include <AIS_Shape.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Wire.hxx>
#include <cmath>
#include <numbers>
#include <utility>
#include <gp_Dir2d.hxx>
#include <gp_Vec2d.hxx>

#include "config.h"
#include "gui.h"
#include "gui_occt_view.h"
#include "mode.h"
#include "skt.h"
#include "skt_bone.h"
#include "utl_geom.h"
#include "utl_occt.h"
#include "utl.h"

#include "skt_tools.inl"

namespace
{
bool bone_hole_radius_ok_(double outer_r, double hole_r)
{
  return hole_r > Precision::Confusion() && hole_r + Precision::Confusion() < outer_r;
}

bool bone_hole_radius_too_large_(double outer_r, double hole_r)
{
  return hole_r + Precision::Confusion() >= outer_r;
}

void bone_toast_(GUI& gui, const char* text, Status_msg kind)
{
  gui.show_message(text, kind);
}

void bone_toast_hole_reject_(GUI& gui, double outer_r, double hole_r)
{
  if (bone_hole_radius_too_large_(outer_r, hole_r))
    bone_toast_(gui, "Hole radius must be smaller than the end circle.", Status_msg::Constraint);
  else
    bone_toast_(gui, "Hole radius must be positive.", Status_msg::Constraint);
}
} // namespace

void Sketch_tools::bone_prompt_next_()
{
  GUI& gui = m_sketch.m_view.gui();
  if (!m_bone_centers)
  {
    bone_toast_(m_sketch.m_view.gui(), "Add bone: click the two circle centers.", Status_msg::Info);
    return;
  }

  if (!m_bone_r2)
  {
    bone_toast_(m_sketch.m_view.gui(), "Click to set radius 2 (at the second center).", Status_msg::Info);
    return;
  }

  if (!m_bone_r1)
  {
    bone_toast_(m_sketch.m_view.gui(), "Click to set radius 1 (snap on circle 2 to match).", Status_msg::Info);
    return;
  }

  if (!m_bone_waist)
  {
    bone_toast_(m_sketch.m_view.gui(), "Click to set waist width.", Status_msg::Info);
    return;
  }

  const Bone_holes holes = gui.get_bone_holes();
  if (holes == Bone_holes::One_radius && !m_bone_hole_r1)
    bone_toast_(m_sketch.m_view.gui(), "Click to set the hole radius (both ends).", Status_msg::Info);
  else if (holes == Bone_holes::Two_radii && !m_bone_hole_r1)
    bone_toast_(m_sketch.m_view.gui(), "Click to set hole radius at end A.", Status_msg::Info);
  else if (holes == Bone_holes::Two_radii && !m_bone_hole_r2)
    bone_toast_(m_sketch.m_view.gui(), "Click to set hole radius at end B.", Status_msg::Info);
}

void Sketch_tools::bone_on_enter_()
{
  if (!m_tmp_edges.empty() && !m_bone_centers && m_sketch.m_dims.entered_edge_len().has_value())
  {
    Sketch_edge&    edge = m_tmp_edges.back();
    const gp_Pnt2d& pt_a = m_sketch.m_nodes[edge.node_idx_a];
    m_last_pt            = gp_Pnt2d(pt_a).Translated(gp_Vec2d(m_sketch.m_dims.entered_edge_len()->dir) *
                                         m_sketch.m_dims.entered_edge_len()->len);
    if (unique(pt_a, *m_last_pt))
      m_sketch.update_edge_end_pt_(edge, m_sketch.m_nodes.get_node_exact(*m_last_pt));
    else
      bone_toast_(m_sketch.m_view.gui(), "Circle centers must be different.", Status_msg::Constraint);

    m_sketch.m_dims.clear_typed_constraints();
  }

  if (!m_bone_centers && !m_tmp_edges.empty() && m_tmp_edges.back().node_idx_b.has_value())
  {
    const Sketch_edge& e = m_tmp_edges.back();
    bone_on_centers_ready_(m_sketch.m_nodes[e.node_idx_a], m_sketch.m_nodes[*e.node_idx_b]);
    return;
  }

  if (!m_bone_centers || !m_sketch.m_dims.entered_edge_len().has_value())
    return;

  const double len = m_sketch.m_dims.entered_edge_len()->len;
  if (len <= Precision::Confusion())
  {
    bone_toast_(m_sketch.m_view.gui(), "Distance must be positive.", Status_msg::Constraint);
    return;
  }

  m_sketch.m_dims.clear_typed_constraints();

  if (!m_bone_r2)
  {
    m_bone_r2 = len;
    m_last_pt.reset();
    bone_ensure_measure_from_(m_bone_centers->first);
    bone_refresh_radius_session_snap_();
    bone_update_preview_();
    bone_prompt_next_();
    return;
  }

  if (!m_bone_r1)
  {
    m_bone_r1 = len;
    m_last_pt.reset();
    bone_refresh_radius_session_snap_();
    bone_update_preview_();
    bone_prompt_next_();
    return;
  }

  if (!m_bone_waist)
    (void)bone_after_waist_(len);
  else if (m_sketch.m_view.gui().get_bone_holes() == Bone_holes::None)
    (void)bone_try_commit_();
  else if (!m_bone_hole_r1)
    (void)bone_after_hole_a_(len);
  else if (!m_bone_hole_r2)
    (void)bone_after_hole_b_(len);
}

void Sketch_tools::add_bone_pt_(const ScreenCoords& screen_coords)
{
  if (m_tmp_edges.empty())
  {
    add_line_string_pt_(screen_coords, Linestring_type::Multiple);
    return;
  }

  if (!m_bone_centers)
  {
    auto on_second = [&](size_t node_idx)
    {
      Sketch_edge& last = m_tmp_edges.back();
      if (node_idx == last.node_idx_a)
      {
        bone_toast_(m_sketch.m_view.gui(), "Circle centers must be different.", Status_msg::Constraint);
        return;
      }

      m_sketch.update_edge_end_pt_(last, node_idx);
      bone_on_centers_ready_(m_sketch.m_nodes[last.node_idx_a], m_sketch.m_nodes[node_idx]);
    };

    if (m_sketch.m_dims.entered_edge_angle().has_value() && !m_tmp_edges.empty())
    {
      std::optional<gp_Pnt2d> pt_opt = m_sketch.m_view.pt_on_plane(screen_coords, m_sketch.m_pln);
      if (!pt_opt)
        return;

      const gp_Pnt2d& pt_a      = m_sketch.m_nodes[m_tmp_edges.back().node_idx_a];
      const double    angle_rad = to_radians(*m_sketch.m_dims.entered_edge_angle());
      gp_Dir2d        constrained_dir(std::cos(angle_rad), std::sin(angle_rad));
      gp_Vec2d        to_click(pt_opt->X() - pt_a.X(), pt_opt->Y() - pt_a.Y());
      const double    dist_along = to_click.Dot(gp_Vec2d(constrained_dir));
      gp_Pnt2d        final_pt   = gp_Pnt2d(pt_a).Translated(gp_Vec2d(constrained_dir) * dist_along);
      if (!unique(pt_a, final_pt))
      {
        bone_toast_(m_sketch.m_view.gui(), "Circle centers must be different.", Status_msg::Constraint);
        return;
      }

      const size_t node_idx = m_sketch.m_nodes.get_node_exact(final_pt);
      m_tmp_node_idxs.push_back(node_idx);
      on_second(node_idx);
      return;
    }

    add_sketch_pt_(screen_coords, 1, on_second);
    return;
  }

  const std::optional<gp_Pnt2d> pt = bone_pick_snapped_(screen_coords);
  if (!pt)
    return;

  m_last_pt = *pt;
  const gp_Pnt2d& c1 = m_bone_centers->first;
  const gp_Pnt2d& c2 = m_bone_centers->second;

  if (!m_bone_r2)
  {
    const double r = c2.Distance(*pt);
    if (r <= Precision::Confusion())
    {
      bone_toast_(m_sketch.m_view.gui(), "Radius must be positive.", Status_msg::Constraint);
      return;
    }

    m_bone_r2 = r;
    m_last_pt.reset();
    bone_ensure_measure_from_(c1);
    bone_refresh_radius_session_snap_();
    bone_update_preview_();
    bone_prompt_next_();
    return;
  }

  if (!m_bone_r1)
  {
    const std::optional<double> r = bone_r1_from_pick_(*pt);
    if (!r)
    {
      bone_toast_(m_sketch.m_view.gui(), "Radius must be positive.", Status_msg::Constraint);
      return;
    }

    m_bone_r1 = r;
    m_last_pt.reset();
    bone_refresh_radius_session_snap_();
    bone_update_preview_();
    bone_prompt_next_();
    return;
  }

  if (!m_bone_waist)
  {
    if (const std::optional<double> waist = bone_waist_from_pt_(*pt))
      (void)bone_after_waist_(*waist);
    else
      bone_toast_(m_sketch.m_view.gui(), "Waist must be greater than zero.", Status_msg::Constraint);

    return;
  }

  if (m_sketch.m_view.gui().get_bone_holes() == Bone_holes::None)
  {
    (void)bone_try_commit_();
    return;
  }

  if (!m_bone_hole_r1)
    (void)bone_after_hole_a_(c1.Distance(*pt));

  else if (!m_bone_hole_r2)
    (void)bone_after_hole_b_(c2.Distance(*pt));
}

void Sketch_tools::move_bone_pt_(const ScreenCoords& screen_coords)
{
  if (!m_bone_centers)
  {
    move_line_string_pt_(screen_coords);
    return;
  }

  const gp_Pnt2d& c1 = m_bone_centers->first;
  const gp_Pnt2d& c2 = m_bone_centers->second;

  auto l = [&](const std::optional<size_t>&, const gp_Pnt2d& pt)
  {
    m_last_pt = pt;

    if (!m_bone_r2)
    {
      const double r = c2.Distance(pt);
      if (r > Precision::Confusion())
        bone_show_len_seg_(c2, bone_perp_rim_(c2, r, pt));
    }
    else if (!m_bone_r1)
    {
      if (const std::optional<double> r = bone_r1_from_pick_(pt))
        bone_show_len_seg_(c1, bone_perp_rim_(c1, *r, pt));
    }
    else if (!m_bone_waist)
    {
      if (const std::optional<double> waist = bone_waist_from_pt_(pt))
      {
        const std::optional<gp_Vec2d> n = bone_axis_perp_();
        if (n)
        {
          const gp_Pnt2d mid  = get_midpoint(c1, c2);
          const double   half = *waist * 0.5;
          bone_show_len_seg_(gp_Pnt2d(mid).Translated(-(*n) * half), gp_Pnt2d(mid).Translated((*n) * half));
        }
      }
    }
    else if (!m_bone_hole_r1)
    {
      const double r = c1.Distance(pt);
      if (r > Precision::Confusion())
        bone_show_len_seg_(c1, bone_perp_rim_(c1, r, pt));
    }
    else if (!m_bone_hole_r2)
    {
      const double r = c2.Distance(pt);
      if (r > Precision::Confusion())
        bone_show_len_seg_(c2, bone_perp_rim_(c2, r, pt));
    }

    bone_update_preview_();
  };

  move_sketch_pt_(screen_coords, l);
}

void Sketch_tools::bone_on_centers_ready_(const gp_Pnt2d& c1, const gp_Pnt2d& c2)
{
  if (!unique(c1, c2))
  {
    bone_toast_(m_sketch.m_view.gui(), "Circle centers must be different.", Status_msg::Constraint);
    return;
  }

  m_bone_centers = std::make_pair(c1, c2);
  m_sketch.m_dims.clear_typed_constraints();
  m_sketch.m_dims.set_show_angle_input(false);
  m_sketch.m_view.gui().hide_angle_edit();
  m_sketch.m_view.gui().hide_dist_edit(false);
  m_sketch.m_dims.show_tmp_dim_preview(c1, c2);
  bone_ensure_measure_from_(c2);
  bone_prompt_next_();
}

void Sketch_tools::bone_refresh_radius_session_snap_()
{
  m_sketch.m_nodes.clear_session_snap_pnts();
  if (!m_bone_centers || !m_bone_r2 || m_bone_r1)
    return;

  const std::optional<gp_Vec2d> n = bone_axis_perp_();
  if (!n)
    return;

  // Tangent points on the already-sized second circle (axis-parallel tangents).
  // Snapping radius 1 to either point matches r1 to r2.
  const gp_Pnt2d c2 = m_bone_centers->second;
  m_sketch.m_nodes.add_session_snap_pnt(gp_Pnt2d(c2).Translated((*n) * (*m_bone_r2)));
  m_sketch.m_nodes.add_session_snap_pnt(gp_Pnt2d(c2).Translated(-(*n) * (*m_bone_r2)));
}

void Sketch_tools::bone_ensure_measure_from_(const gp_Pnt2d& origin)
{
  const size_t idx = m_sketch.m_nodes.get_node_exact(origin);
  if (m_tmp_edges.size() <= 1)
    m_tmp_edges.push_back({idx});
  else
  {
    m_tmp_edges.back().node_idx_a = idx;
    m_tmp_edges.back().node_idx_b.reset();
  }
}

void Sketch_tools::bone_show_len_seg_(const gp_Pnt2d& a, const gp_Pnt2d& b)
{
  if (!unique(a, b))
    return;

  if (m_tmp_edges.size() <= 1)
    bone_ensure_measure_from_(a);

  m_sketch.update_edge_shp_(m_tmp_edges.back(), a, b);
  const double dist = a.Distance(b) / m_sketch.m_view.get_display_to_model_scale();
  m_sketch.m_dims.show_tmp_dim_preview(a, b);
  m_sketch.m_dims.offer_dist_edit_for_segment(a, b, dist);
}

gp_Pnt2d Sketch_tools::bone_perp_rim_(const gp_Pnt2d& center, double radius, const gp_Pnt2d& hint) const
{
  const std::optional<gp_Vec2d> n = bone_axis_perp_();
  if (!n || radius <= Precision::Confusion())
    return hint;

  double s = gp_Vec2d(center, hint).Dot(*n);
  if (std::abs(s) <= Precision::Confusion())
    s = 1.0;

  return gp_Pnt2d(center).Translated(*n * (s > 0.0 ? radius : -radius));
}

std::optional<gp_Pnt2d> Sketch_tools::bone_pick_snapped_(const ScreenCoords& screen_coords)
{
  std::optional<gp_Pnt2d> pt = m_sketch.m_view.pt_on_plane(screen_coords, m_sketch.m_pln);
  if (!pt)
    return std::nullopt;

  (void)m_sketch.m_nodes.try_get_node_idx_snap(*pt);
  return pt;
}

bool Sketch_tools::bone_after_waist_(double waist)
{
  if (!m_bone_centers || !m_bone_r1 || !m_bone_r2)
    return false;

  Bone_params params;
  params.c1    = m_bone_centers->first;
  params.c2    = m_bone_centers->second;
  params.r1    = *m_bone_r1;
  params.r2    = *m_bone_r2;
  params.waist = waist;
  params.drive = Bone_drive::Waist;
  if (!compute_bone_geom(params))
  {
    bone_toast_(m_sketch.m_view.gui(), "Cannot form a bone with that waist (too wide, or one circle inside the other).",
                Status_msg::Constraint);
    return false;
  }

  m_bone_waist = waist;
  const Bone_holes holes = m_sketch.m_view.gui().get_bone_holes();
  if (holes == Bone_holes::None)
    return bone_try_commit_();

  bone_prompt_next_();
  return true;
}

bool Sketch_tools::bone_after_hole_a_(double hole_r)
{
  if (!m_bone_centers || !m_bone_r1 || !m_bone_r2 || !m_bone_waist)
    return false;

  const Bone_holes holes = m_sketch.m_view.gui().get_bone_holes();
  if (holes == Bone_holes::One_radius)
  {
    if (!bone_hole_radius_ok_(*m_bone_r1, hole_r) || !bone_hole_radius_ok_(*m_bone_r2, hole_r))
    {
      const double outer = bone_hole_radius_too_large_(*m_bone_r1, hole_r) ? *m_bone_r1 : *m_bone_r2;
      bone_toast_hole_reject_(m_sketch.m_view.gui(), outer, hole_r);
      return false;
    }

    m_bone_hole_r1 = hole_r;
    m_bone_hole_r2 = hole_r;
    return bone_try_commit_();
  }

  if (holes != Bone_holes::Two_radii)
    return false;

  if (!bone_hole_radius_ok_(*m_bone_r1, hole_r))
  {
    bone_toast_hole_reject_(m_sketch.m_view.gui(), *m_bone_r1, hole_r);
    return false;
  }

  m_bone_hole_r1 = hole_r;
  bone_prompt_next_();
  return true;
}

bool Sketch_tools::bone_after_hole_b_(double hole_r)
{
  if (!m_bone_centers || !m_bone_r2 || !m_bone_waist || !m_bone_hole_r1)
    return false;

  if (!bone_hole_radius_ok_(*m_bone_r2, hole_r))
  {
    bone_toast_hole_reject_(m_sketch.m_view.gui(), *m_bone_r2, hole_r);
    return false;
  }

  m_bone_hole_r2 = hole_r;
  return bone_try_commit_();
}

bool Sketch_tools::bone_try_commit_()
{
  if (!m_bone_centers || !m_bone_r1 || !m_bone_r2 || !m_bone_waist)
    return false;

  Bone_params params;
  params.c1    = m_bone_centers->first;
  params.c2    = m_bone_centers->second;
  params.r1    = *m_bone_r1;
  params.r2    = *m_bone_r2;
  params.waist = *m_bone_waist;
  params.drive = Bone_drive::Waist;
  if (!compute_bone_geom(params))
  {
    bone_toast_(m_sketch.m_view.gui(), "Could not create bone.", Status_msg::Error);
    return false;
  }

  const GUI& gui = m_sketch.m_view.gui();
  if (gui.get_bone_holes() == Bone_holes::None)
  {
    m_bone_hole_r1.reset();
    m_bone_hole_r2.reset();
  }

  m_sketch.add_bone(params.c1, params.c2, params.r1, params.r2, *m_bone_waist, gui.get_bone_add_center_nodes(),
                    m_bone_hole_r1, m_bone_hole_r2, gui.get_bone_add_radius_nodes(),
                    gui.get_bone_add_total_length_nodes());
  clear_tmps();
  bone_toast_(m_sketch.m_view.gui(), "Bone added.", Status_msg::Success);
  m_sketch.m_view.gui().set_parent_mode();
  return true;
}

void Sketch_tools::finalize_bone_()
{
  if (!m_bone_centers || !m_bone_r1 || !m_bone_r2)
    return;

  if (!m_bone_waist)
  {
    if (!m_last_pt || m_sketch.m_view.gui().get_bone_holes() != Bone_holes::None)
      return;

    if (const std::optional<double> waist = bone_waist_from_pt_(*m_last_pt))
      (void)bone_after_waist_(*waist);

    return;
  }

  if (m_sketch.m_view.gui().get_bone_holes() == Bone_holes::None)
  {
    (void)bone_try_commit_();
    return;
  }

  if (!m_last_pt)
    return;

  if (!m_bone_hole_r1)
  {
    (void)bone_after_hole_a_(m_bone_centers->first.Distance(*m_last_pt));
    return;
  }

  if (!m_bone_hole_r2)
    (void)bone_after_hole_b_(m_bone_centers->second.Distance(*m_last_pt));
}

std::optional<gp_Vec2d> Sketch_tools::bone_axis_perp_() const
{
  if (!m_bone_centers)
    return std::nullopt;

  gp_Vec2d     axis(m_bone_centers->first, m_bone_centers->second);
  const double dist = axis.Magnitude();
  if (dist <= Precision::Confusion())
    return std::nullopt;

  return gp_Vec2d(axis / dist).Rotated(std::numbers::pi / 2.0);
}

std::optional<double> Sketch_tools::bone_r1_from_pick_(const gp_Pnt2d& pt) const
{
  if (!m_bone_centers || !m_bone_r2)
    return std::nullopt;

  if (const std::optional<gp_Vec2d> n = bone_axis_perp_())
  {
    const gp_Pnt2d c2 = m_bone_centers->second;
    const gp_Pnt2d t0 = gp_Pnt2d(c2).Translated((*n) * (*m_bone_r2));
    const gp_Pnt2d t1 = gp_Pnt2d(c2).Translated(-(*n) * (*m_bone_r2));
    const double   tol = Precision::Confusion();
    if (pt.Distance(t0) <= tol || pt.Distance(t1) <= tol)
      return m_bone_r2;
  }

  const double r = m_bone_centers->first.Distance(pt);
  if (r <= Precision::Confusion())
    return std::nullopt;

  return r;
}

std::optional<double> Sketch_tools::bone_waist_from_pt_(const gp_Pnt2d& pt) const
{
  const std::optional<gp_Vec2d> n = bone_axis_perp_();
  if (!n || !m_bone_centers)
    return std::nullopt;

  // Twice the distance from the bone axis (c1->c2); matches min neck when cutters solve.
  const double waist = 2.0 * std::abs(gp_Vec2d(m_bone_centers->first, pt).Dot(*n));
  if (waist <= Precision::Confusion())
    return std::nullopt;

  return waist;
}

void Sketch_tools::refresh_bone_preview() { bone_update_preview_(); }

void Sketch_tools::apply_bone_holes_option()
{
  if (m_bone_waist && m_sketch.m_view.gui().get_bone_holes() == Bone_holes::None)
  {
    (void)bone_try_commit_();
    return;
  }

  bone_update_preview_();
  bone_prompt_next_();
}

#if DEV_MODE
void Sketch_tools::bone_hide_debug_()
{
  m_sketch.m_view.remove(m_tmp_debug_shp);
  m_tmp_debug_shp = nullptr;
}

void Sketch_tools::bone_show_debug_(const Bone_geom& g)
{
  const std::optional<TopoDS_Shape> dbg =
      make_bone_debug_shape(m_sketch.m_pln, g, m_sketch.m_view.gui().get_bone_debug_flags());
  if (!dbg)
  {
    bone_hide_debug_();
    return;
  }

  show(m_sketch.m_ctx, m_tmp_debug_shp, *dbg);
  m_tmp_debug_shp->SetColor(Quantity_NOC_CYAN);
  m_tmp_debug_shp->SetWidth(1.5);
  m_sketch.m_ctx.Deactivate(m_tmp_debug_shp);
}
#endif

void Sketch_tools::bone_update_preview_()
{
  if (!m_bone_centers)
  {
#if DEV_MODE
    bone_hide_debug_();
#endif
    return;
  }

  const gp_Pnt2d& c1 = m_bone_centers->first;
  const gp_Pnt2d& c2 = m_bone_centers->second;

  auto circle_at = [&](const gp_Pnt2d& c, double r) -> TopoDS_Wire
  { return make_circle_wire(m_sketch.m_pln, c, gp_Pnt2d(c.X() + r, c.Y())); };

  // Live preview: r2 first (cursor still near c2), then r1.
  const double r2 = m_bone_r2.value_or(m_last_pt ? c2.Distance(*m_last_pt) : 0.0);
  const double r1 = m_bone_r1.value_or((m_bone_r2 && m_last_pt) ? bone_r1_from_pick_(*m_last_pt).value_or(0.0) : 0.0);

  auto show_compound = [&](const TopoDS_Compound& comp)
  { show(m_sketch.m_ctx, m_tmp_shp, comp); };

  if (m_bone_r1 && m_bone_r2)
  {
    const std::optional<double> waist =
        m_bone_waist
            ? m_bone_waist
            : (m_sketch.m_dims.entered_edge_len().has_value()
                   ? std::optional<double>(m_sketch.m_dims.entered_edge_len()->len)
                   : (m_last_pt ? bone_waist_from_pt_(*m_last_pt) : std::nullopt));
    if (waist)
    {
      Bone_params params;
      params.c1    = c1;
      params.c2    = c2;
      params.r1    = *m_bone_r1;
      params.r2    = *m_bone_r2;
      params.waist = *waist;
      params.drive = Bone_drive::Waist;
      if (const std::optional<Bone_geom> g = compute_bone_geom(params))
      {
        TopoDS_Compound comp;
        BRep_Builder    bb;
        bb.MakeCompound(comp);
        bb.Add(comp, make_bone_preview_shape(m_sketch.m_pln, *g));

        auto maybe_hole = [&](const gp_Pnt2d& c, double outer_r, const std::optional<double>& fixed,
                              bool live_from_center) -> void
        {
          double hr = 0.0;
          if (fixed)
            hr = *fixed;
          else if (live_from_center && m_last_pt)
            hr = c.Distance(*m_last_pt);
          if (bone_hole_radius_ok_(outer_r, hr))
            bb.Add(comp, circle_at(c, hr));
        };

        if (m_bone_waist)
        {
          const Bone_holes holes = m_sketch.m_view.gui().get_bone_holes();
          if (holes == Bone_holes::One_radius)
          {
            const double hr = m_bone_hole_r1 ? *m_bone_hole_r1
                                            : (m_last_pt ? c1.Distance(*m_last_pt) : 0.0);
            if (bone_hole_radius_ok_(*m_bone_r1, hr))
              bb.Add(comp, circle_at(c1, hr));

            if (bone_hole_radius_ok_(*m_bone_r2, hr))
              bb.Add(comp, circle_at(c2, hr));
          }
          else if (holes == Bone_holes::Two_radii)
          {
            maybe_hole(c1, *m_bone_r1, m_bone_hole_r1, !m_bone_hole_r1);
            maybe_hole(c2, *m_bone_r2, m_bone_hole_r2, m_bone_hole_r1.has_value() && !m_bone_hole_r2);
          }
        }

        show_compound(comp);
#if DEV_MODE
        bone_show_debug_(*g);
#endif
        return;
      }
    }

    m_sketch.m_view.remove(m_tmp_shp);
    m_tmp_shp = nullptr;
#if DEV_MODE
    bone_hide_debug_();
#endif
    return;
  }

  TopoDS_Compound comp;
  BRep_Builder    bb;
  bb.MakeCompound(comp);
  bool any = false;
  if (r1 > Precision::Confusion())
  {
    bb.Add(comp, circle_at(c1, r1));
    any = true;
  }

  if (r2 > Precision::Confusion())
  {
    bb.Add(comp, circle_at(c2, r2));
    any = true;
  }

  if (!any)
  {
    m_sketch.m_view.remove(m_tmp_shp);
    m_tmp_shp = nullptr;
#if DEV_MODE
    bone_hide_debug_();
#endif
    return;
  }

  show_compound(comp);
#if DEV_MODE
  bone_hide_debug_();
#endif
}
