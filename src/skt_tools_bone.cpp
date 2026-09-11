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
} // namespace

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
    return;

  Sketch_edge&    edge = m_tmp_edges.back();
  const gp_Pnt2d& pt_a = m_sketch.m_nodes[edge.node_idx_a];
  m_last_pt            = gp_Pnt2d(pt_a).Translated(gp_Vec2d(m_sketch.m_dims.entered_edge_len()->dir) * len);
  if (unique(pt_a, *m_last_pt))
    m_sketch.update_edge_end_pt_(edge, m_sketch.m_nodes.get_node_exact(*m_last_pt));

  m_sketch.m_dims.clear_typed_constraints();

  if (m_tmp_edges.size() == 2)
  {
    m_bone_r1 = len;
    bone_begin_next_edge_from_(m_bone_centers->second);
  }
  else if (m_tmp_edges.size() == 3)
  {
    m_bone_r2 = len;
    bone_begin_next_edge_from_(get_midpoint(m_bone_centers->first, m_bone_centers->second));
  }
  else if (m_tmp_edges.size() == 4)
    (void)bone_after_waist_(len);

  else if (m_tmp_edges.size() == 5)
    (void)bone_after_hole_a_(len);

  else if (m_tmp_edges.size() == 6)
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
        return;

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
        return;

      const size_t node_idx = m_sketch.m_nodes.get_node_exact(final_pt);
      m_tmp_node_idxs.push_back(node_idx);
      on_second(node_idx);
      return;
    }

    add_sketch_pt_(screen_coords, 1, on_second);
    return;
  }

  auto on_dim = [&](size_t node_idx)
  {
    Sketch_edge& last = m_tmp_edges.back();
    if (node_idx == last.node_idx_a)
      return;

    const gp_Pnt2d& pt_a = m_sketch.m_nodes[last.node_idx_a];
    const gp_Pnt2d& pt_b = m_sketch.m_nodes[node_idx];
    if (!unique(pt_a, pt_b))
      return;

    m_sketch.update_edge_end_pt_(last, node_idx);
    const double len = pt_a.Distance(pt_b);

    if (m_tmp_edges.size() == 2)
    {
      m_bone_r1 = len;
      bone_begin_next_edge_from_(m_bone_centers->second);
    }
    else if (m_tmp_edges.size() == 3)
    {
      m_bone_r2 = len;
      bone_begin_next_edge_from_(get_midpoint(m_bone_centers->first, m_bone_centers->second));
    }
    else if (m_tmp_edges.size() == 4)
    {
      const std::optional<double> waist = bone_waist_from_pt_(pt_b);
      if (waist)
        (void)bone_after_waist_(*waist);
    }
    else if (m_tmp_edges.size() == 5)
      (void)bone_after_hole_a_(len);

    else if (m_tmp_edges.size() == 6)
      (void)bone_after_hole_b_(len);
  };

  add_sketch_pt_(screen_coords, 1, on_dim);
}

void Sketch_tools::move_bone_pt_(const ScreenCoords& screen_coords)
{
  // Only center-to-center shows an AIS length dim. End radii / holes use the circle
  // preview; waist uses the bone outline. Tab distance entry still works.
  if (!m_bone_centers || m_tmp_edges.size() <= 1)
  {
    move_line_string_pt_(screen_coords);
    return;
  }

  if (m_tmp_edges.size() == 4)
  {
    const std::optional<gp_Vec2d> n = bone_axis_perp_();
    if (!n || !m_bone_r1 || !m_bone_r2)
      return;

    const gp_Pnt2d& c1  = m_bone_centers->first;
    const gp_Pnt2d& c2  = m_bone_centers->second;
    const gp_Pnt2d  mid = get_midpoint(c1, c2);

    auto l = [&](const std::optional<size_t>&, const gp_Pnt2d& pt_b)
    {
      Sketch_edge& edge = m_tmp_edges.back();
      // Waist value is twice the distance from the bone axis (independent of click along-axis).
      double half = std::abs(gp_Vec2d(c1, pt_b).Dot(*n));
      if (m_sketch.m_dims.entered_edge_len().has_value())
        half = m_sketch.m_dims.entered_edge_len()->len * 0.5;

      if (half <= Precision::Confusion())
      {
        m_sketch.m_dims.clear_tmp_dim_anno();
        m_sketch.m_view.remove(m_tmp_shp);
        m_tmp_shp = nullptr;
        return;
      }

      const double waist  = 2.0 * half;
      gp_Pnt2d     span_a = gp_Pnt2d(mid).Translated(-(*n) * half);
      gp_Pnt2d     span_b = gp_Pnt2d(mid).Translated((*n) * half);

      Bone_params params;
      params.c1    = c1;
      params.c2    = c2;
      params.r1    = *m_bone_r1;
      params.r2    = *m_bone_r2;
      params.waist = waist;
      params.drive = Bone_drive::Waist;
      if (const std::optional<Bone_geom> g = compute_bone_geom(params))
      {
        // Place the live rubber-band on the true neck (offset from mid when r1 != r2).
        const Bone_profile pr = get_bone_profile(*g);
        span_a                = pr.waist_plus;
        span_b                = pr.waist_minus;
      }

      m_last_pt = span_b;
      m_sketch.update_edge_shp_(edge, span_a, span_b);

      const double dist = waist / m_sketch.m_view.get_display_to_model_scale();
      m_sketch.m_dims.clear_tmp_dim_anno();
      m_sketch.m_dims.offer_dist_edit_for_segment(span_a, span_b, dist);
      bone_update_preview_();
    };

    move_sketch_pt_(screen_coords, l);
    return;
  }

  move_line_string_pt_(screen_coords);
  m_sketch.m_dims.clear_tmp_dim_anno();
  bone_update_preview_();
}

void Sketch_tools::bone_begin_next_edge_from_(const gp_Pnt2d& origin)
{
  m_sketch.m_dims.clear_typed_constraints();
  m_sketch.m_dims.set_show_angle_input(false);
  m_sketch.m_view.gui().hide_angle_edit();
  m_sketch.m_view.gui().hide_dist_edit(false);
  m_tmp_edges.push_back({m_sketch.m_nodes.get_node_exact(origin)});
}

void Sketch_tools::bone_on_centers_ready_(const gp_Pnt2d& c1, const gp_Pnt2d& c2)
{
  if (!unique(c1, c2))
    return;

  m_bone_centers = std::make_pair(c1, c2);
  bone_begin_next_edge_from_(c1);
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
    return false;

  m_bone_waist = waist;
  const Bone_holes holes = m_sketch.m_view.gui().get_bone_holes();
  if (holes == Bone_holes::None)
    return bone_try_commit_();

  bone_begin_next_edge_from_(m_bone_centers->first);
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
      if (bone_hole_radius_too_large_(*m_bone_r1, hole_r) || bone_hole_radius_too_large_(*m_bone_r2, hole_r))
        m_sketch.m_view.gui().show_message("Hole radius must be smaller than the end circle.");
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
    if (bone_hole_radius_too_large_(*m_bone_r1, hole_r))
      m_sketch.m_view.gui().show_message("Hole radius must be smaller than the end circle.");
    return false;
  }

  m_bone_hole_r1 = hole_r;
  bone_begin_next_edge_from_(m_bone_centers->second);
  return true;
}

bool Sketch_tools::bone_after_hole_b_(double hole_r)
{
  if (!m_bone_centers || !m_bone_r2 || !m_bone_waist || !m_bone_hole_r1)
    return false;

  if (!bone_hole_radius_ok_(*m_bone_r2, hole_r))
  {
    if (bone_hole_radius_too_large_(*m_bone_r2, hole_r))
      m_sketch.m_view.gui().show_message("Hole radius must be smaller than the end circle.");
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
    return false;

  const bool add_centers = m_sketch.m_view.gui().get_bone_add_center_nodes();
  m_sketch.add_bone(params.c1, params.c2, params.r1, params.r2, *m_bone_waist, add_centers, m_bone_hole_r1,
                    m_bone_hole_r2);
  clear_tmps();
  m_sketch.m_view.gui().set_parent_mode();
  return true;
}

void Sketch_tools::finalize_bone_()
{
  if (!m_bone_centers || !m_bone_r1 || !m_bone_r2 || !m_last_pt)
    return;

  if (m_tmp_edges.size() == 4)
  {
    if (m_sketch.m_view.gui().get_bone_holes() != Bone_holes::None)
      return;

    const std::optional<double> waist = bone_waist_from_pt_(*m_last_pt);
    if (waist)
      (void)bone_after_waist_(*waist);
    return;
  }

  if (m_tmp_edges.size() == 5 && m_bone_waist)
  {
    const gp_Pnt2d& c1 = m_bone_centers->first;
    (void)bone_after_hole_a_(c1.Distance(*m_last_pt));
    return;
  }

  if (m_tmp_edges.size() == 6 && m_bone_waist && m_bone_hole_r1)
  {
    const gp_Pnt2d& c2 = m_bone_centers->second;
    (void)bone_after_hole_b_(c2.Distance(*m_last_pt));
  }
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

  const double r1 = m_bone_r1.value_or(
      (m_tmp_edges.size() == 2 && m_last_pt) ? c1.Distance(*m_last_pt) : 0.0);
  const double r2 = m_bone_r2.value_or(
      (m_tmp_edges.size() == 3 && m_last_pt) ? c2.Distance(*m_last_pt) : 0.0);

  auto show_compound = [&](const TopoDS_Compound& comp)
  { show(m_sketch.m_ctx, m_tmp_shp, comp); };

  if (m_tmp_edges.size() >= 4 && m_bone_r1 && m_bone_r2)
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

        if (m_tmp_edges.size() >= 5)
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
            maybe_hole(c1, *m_bone_r1, m_bone_hole_r1, m_tmp_edges.size() == 5);
            maybe_hole(c2, *m_bone_r2, m_bone_hole_r2, m_tmp_edges.size() == 6);
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
