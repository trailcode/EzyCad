#include "sketch_tools.h"

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <Precision.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>
#include <array>
#include <cmath>
#include <functional>

#include "gui.h"
#include "mode.h"
#include "occt_view.h"
#include "sketch.h"
#include "sketch_delta.h"
#include "sketch_edge.h"
#include "utl_geom.h"
#include "utl_occt.h"
#include "utl.h"

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

std::optional<Symmetric_edge_span> symmetric_edge_from_center_and_hint(const gp_Pnt2d& center, const gp_Pnt2d& dir_hint_pt)
{
  gp_Vec2d     v(center, dir_hint_pt);
  const double half = v.Magnitude();
  if (half <= Precision::Confusion())
    return std::nullopt;

  return symmetric_edge_from_center(center, gp_Dir2d(v), half * 2.0);
}
} // namespace

Sketch_tools::Sketch_tools(Sketch& sketch)
    : m_sketch(sketch)
{
}

void Sketch_tools::on_click(const ScreenCoords& screen_coords)
{
  switch (m_sketch.get_mode())
  {
    // clang-format off
    case Mode::Sketch_add_square:
    case Mode::Sketch_add_circle:
    case Mode::Sketch_add_rectangle:
    case Mode::Sketch_add_rectangle_center_pt:
      add_line_string_pt_(screen_coords, Linestring_type::Single);
      break;

    case Mode::Sketch_add_node:           add_node_pt_          (screen_coords);                            break;
    case Mode::Sketch_add_edge:           add_line_string_pt_   (screen_coords, Linestring_type::Single);   break;
    case Mode::Sketch_add_slot:           add_line_string_pt_   (screen_coords, Linestring_type::Two);      break;
    case Mode::Sketch_add_multi_edges:    add_line_string_pt_   (screen_coords, Linestring_type::Multiple); break;
    case Mode::Sketch_add_seg_circle_arc: add_arc_circle_pt_    (screen_coords);                            break;
    case Mode::Sketch_operation_axis:     add_operation_axis_pt_(screen_coords);                            break;
    default:
      EZY_ASSERT(false);
    // clang-format on
  }
}

void Sketch_tools::on_move(const ScreenCoords& screen_coords)
{
  switch (m_sketch.get_mode())
  {
    // clang-format off
    case Mode::Sketch_add_node:            move_add_node_pt_    (screen_coords); break;
    case Mode::Sketch_add_seg_circle_arc:  move_arc_circle_pt_  (screen_coords); break;
    case Mode::Sketch_add_square:          move_square_pt_      (screen_coords); break;
    case Mode::Sketch_add_circle:          move_circle_pt_      (screen_coords); break;
    case Mode::Sketch_add_slot:            move_slot_pt_        (screen_coords); break;

    case Mode::Sketch_add_edge:
    case Mode::Sketch_operation_axis:
    case Mode::Sketch_add_multi_edges:
      move_line_string_pt_ (screen_coords);
      break;

    case Mode::Sketch_add_rectangle:
    case Mode::Sketch_add_rectangle_center_pt:
      move_rectangle_pt_(screen_coords);
      break;
    // clang-format on
  default:
    break;
  }
}

void Sketch_tools::on_enter()
{
  switch (m_sketch.get_mode())
  {
    // clang-format off
    case Mode::Sketch_add_square:
    case Mode::Sketch_add_rectangle:
    case Mode::Sketch_add_rectangle_center_pt:
    case Mode::Sketch_add_circle:
    case Mode::Sketch_add_node:
      m_sketch.m_dims.check_dimension_rubber_();
      break;

    case Mode::Sketch_add_edge:
      m_sketch.m_dims.check_dimension_seg_(static_cast<int>(Linestring_type::Single));
      break;

    case Mode::Sketch_add_slot:
      m_sketch.m_dims.check_dimension_seg_(static_cast<int>(Linestring_type::Two));
      break;

    case Mode::Sketch_add_multi_edges:
      m_sketch.m_dims.check_dimension_seg_(static_cast<int>(Linestring_type::Multiple));
      break;
    // clang-format on
  default:
    break;
  }
}

void Sketch_tools::finalize()
{
  Sketch_op_recorder rec(m_sketch.m_view, m_sketch);
  {
    m_sketch.m_dims.on_finalize_elm_start();

    switch (m_sketch.get_mode())
    {
      // clang-format off
    case Mode::Sketch_add_edge:
    case Mode::Sketch_add_multi_edges:
      finalize_edges_(rec);
      break;

    case Mode::Sketch_add_rectangle:
    case Mode::Sketch_add_rectangle_center_pt:
      finalize_rectangle_(rec);
      break;

    case Mode::Sketch_add_square:       finalize_square_(rec);            break;
    case Mode::Sketch_add_circle:       finalize_circle_(rec);            break;
    case Mode::Sketch_add_node:         finalize_add_node_elm_cleanup_(); break;
    case Mode::Sketch_add_slot:         finalize_slot_(rec);              break;
    case Mode::Sketch_operation_axis:   finalize_operation_axis_(rec);    break;
      // clang-format on
    default:
      EZY_ASSERT(false);
    }

    rec.commit();
  }
}

bool Sketch_tools::cancel() { return clear_tmps(); }

bool Sketch_tools::edge_from_center_active() const
{
  return Sketch::get_edge_from_center() && m_sketch.get_mode() == Mode::Sketch_add_edge && !m_tmp_edges.empty() &&
         !m_tmp_edges.back().node_idx_b.has_value();
}

bool Sketch_tools::complete_edge_from_center_(const ScreenCoords& screen_coords)
{
  if (!edge_from_center_active())
    return false;

  Sketch_edge&    edge   = m_tmp_edges.back();
  const gp_Pnt2d& center = m_sketch.m_nodes[edge.node_idx_a];

  std::optional<Symmetric_edge_span> span;
  if (m_sketch.m_dims.entered_edge_len().has_value())
    span = symmetric_edge_from_center(center, m_sketch.m_dims.entered_edge_len()->dir, m_sketch.m_dims.entered_edge_len()->len);

  else if (m_sketch.m_dims.entered_edge_angle().has_value())
  {
    std::optional<gp_Pnt2d> pt_opt = m_sketch.m_view.pt_on_plane(screen_coords, m_sketch.m_pln);
    if (!pt_opt)
      return true;

    const double angle_rad = to_radians(*m_sketch.m_dims.entered_edge_angle());
    gp_Dir2d     dir(std::cos(angle_rad), std::sin(angle_rad));
    gp_Vec2d     to_click(center, *pt_opt);
    const double half = std::abs(to_click.Dot(gp_Vec2d(dir)));
    span              = symmetric_edge_from_center(center, dir, half * 2.0);
  }
  else
  {
    std::optional<gp_Pnt2d> pt_opt = m_sketch.m_view.pt_on_plane(screen_coords, m_sketch.m_pln);
    if (!pt_opt)
      return true;

    span = symmetric_edge_from_center_and_hint(center, *pt_opt);
  }

  if (!span)
    return true;

  edge.node_idx_a = m_sketch.m_nodes.get_node_exact(span->pt_a);
  m_sketch.update_edge_end_pt_(edge, m_sketch.m_nodes.get_node_exact(span->pt_b));
  m_sketch.m_dims.entered_edge_angle() = std::nullopt;
  m_sketch.m_dims.entered_edge_len()   = std::nullopt;
  m_sketch.m_dims.set_show_angle_input(false);
  m_sketch.m_dims.set_show_dim_input(false);
  m_sketch.m_view.gui().hide_angle_edit();
  finalize();
  return true;
}

void Sketch_tools::add_line_string_pt_(const ScreenCoords& screen_coords, Sketch_tools::Linestring_type linestring_type)
{
  if (edge_from_center_active() && linestring_type == Sketch_tools::Linestring_type::Single)
  {
    complete_edge_from_center_(screen_coords);
    return;
  }

  auto on_line_string = [&](size_t node_idx)
  {
    if (m_tmp_edges.size())
    {
      Sketch_edge& last_edge = m_tmp_edges.back();
      if (node_idx == last_edge.node_idx_a)
        // We cannot have edges with zero length.
        return;

      m_sketch.update_edge_end_pt_(last_edge, node_idx);

      if (linestring_type == Sketch_tools::Linestring_type::Single)
      {
        finalize();
        return;
      }
      else if (linestring_type == Sketch_tools::Linestring_type::Two)
        if (m_tmp_edges.size() == 2)
        {
          finalize();
          return;
        }
    }

    // Start a new edge - clear constraints for fresh start (click path for multi-line)
    m_sketch.m_dims.entered_edge_angle() = std::nullopt;
    m_sketch.m_dims.entered_edge_len()   = std::nullopt;
    m_sketch.m_dims.set_show_angle_input(false);
    m_sketch.m_view.gui().hide_angle_edit();
    m_tmp_edges.push_back({node_idx});
  };

  // When angle constraint is active, finalize on the constrained line (project click onto it)
  if (m_sketch.m_dims.entered_edge_angle().has_value() && m_tmp_edges.size())
  {
    if (edge_from_center_active())
    {
      complete_edge_from_center_(screen_coords);
      return;
    }

    std::optional<gp_Pnt2d> pt_opt = m_sketch.m_view.pt_on_plane(screen_coords, m_sketch.m_pln);
    if (!pt_opt)
      return;

    const gp_Pnt2d& pt_a      = m_sketch.m_nodes[m_tmp_edges.back().node_idx_a];
    double          angle_rad = to_radians(*m_sketch.m_dims.entered_edge_angle());
    gp_Dir2d        constrained_dir(std::cos(angle_rad), std::sin(angle_rad));
    gp_Vec2d        to_click(pt_opt->X() - pt_a.X(), pt_opt->Y() - pt_a.Y());
    double          dist_along = to_click.Dot(gp_Vec2d(constrained_dir));
    gp_Pnt2d        final_pt   = gp_Pnt2d(pt_a).Translated(gp_Vec2d(constrained_dir) * dist_along);
    if (!unique(pt_a, final_pt))
      return;

    // Do not run try_get_node_idx_snap here: it can move the point off the angle ray (axis snap).
    const size_t node_idx = m_sketch.m_nodes.get_node_exact(final_pt);
    m_tmp_node_idxs.push_back(node_idx);
    on_line_string(node_idx);
    return;
  }

  add_sketch_pt_(screen_coords, 1, on_line_string);
}

void Sketch_tools::move_line_string_pt_(const ScreenCoords& screen_coords)
{
  if (edge_from_center_active())
  {
    auto l = [&](const std::optional<size_t>&, const gp_Pnt2d& pt_b)
    {
      Sketch_edge&    edge   = m_tmp_edges.back();
      const gp_Pnt2d& center = m_sketch.m_nodes[edge.node_idx_a];

      std::optional<Symmetric_edge_span> span;
      if (m_sketch.m_dims.entered_edge_angle().has_value())
      {
        const double angle_rad = to_radians(*m_sketch.m_dims.entered_edge_angle());
        gp_Dir2d     dir(std::cos(angle_rad), std::sin(angle_rad));
        if (m_sketch.m_dims.entered_edge_len().has_value())
          span = symmetric_edge_from_center(center, dir, m_sketch.m_dims.entered_edge_len()->len);
        else
        {
          gp_Vec2d     to_mouse(center, pt_b);
          const double half = std::abs(to_mouse.Dot(gp_Vec2d(dir)));
          span              = symmetric_edge_from_center(center, dir, half * 2.0);
        }
      }
      else if (m_sketch.m_dims.entered_edge_len().has_value())
        span = symmetric_edge_from_center(center, m_sketch.m_dims.entered_edge_len()->dir,
                                          m_sketch.m_dims.entered_edge_len()->len);
      else
        span = symmetric_edge_from_center_and_hint(center, pt_b);

      if (!span)
      {
        if (edge.shp)
        {
          m_sketch.m_ctx.Remove(edge.shp, true);
          edge.shp.Nullify();
        }
        m_sketch.m_dims.clear_tmp_dim_anno();
        return;
      }

      const gp_Pnt2d& span_a = span->pt_a;
      const gp_Pnt2d& span_b = span->pt_b;
      m_sketch.update_edge_shp_(edge, span_a, span_b);

      const double dist = span->full_len / m_sketch.m_view.get_dimension_scale();
      m_sketch.m_dims.show_tmp_dim_preview(span_a, span_b);
      m_sketch.m_dims.offer_dist_edit_for_segment(span_a, span_b, dist);
      m_sketch.m_dims.offer_angle_edit_for_segment(
          center, pt_b, to_degrees(std::atan2(gp_Vec2d(center, pt_b).Y(), gp_Vec2d(center, pt_b).X())));
    };

    move_sketch_pt_(screen_coords, l);
    return;
  }

  auto l = [&](const std::optional<size_t>& node_idx_b, const gp_Pnt2d& pt_b)
  {
    if (m_tmp_edges.empty())
      return;

    Sketch_edge&    edge = m_tmp_edges.back();
    const gp_Pnt2d& pt_a = m_sketch.m_nodes[edge.node_idx_a];

    if (!unique(pt_a, pt_b))
      // Cannot have a edge with zero length
      return;

    gp_Pnt2d final_pt_b = pt_b;

    // Apply angle constraint if set - this takes priority
    if (m_sketch.m_dims.entered_edge_angle().has_value())
    {
      // Calculate direction based on angle (0 degrees = positive X axis, counterclockwise)
      double   angle_rad = to_radians(*m_sketch.m_dims.entered_edge_angle());
      gp_Dir2d constrained_dir(std::cos(angle_rad), std::sin(angle_rad));

      // If distance is also constrained, use that
      if (m_sketch.m_dims.entered_edge_len().has_value())
        // Use the constrained distance - angle constraint is always enforced
        final_pt_b = gp_Pnt2d(pt_a).Translated(gp_Vec2d(constrained_dir) * m_sketch.m_dims.entered_edge_len()->len);
      else
      {
        // Project the mouse point onto the angle-constrained line
        // Find the distance along the constrained direction from pt_a to mouse position
        gp_Vec2d to_mouse(pt_b.X() - pt_a.X(), pt_b.Y() - pt_a.Y());
        double   dist_along_constrained = to_mouse.Dot(gp_Vec2d(constrained_dir));

        // Calculate the point on the constrained line at this distance
        // This ensures the angle is ALWAYS maintained
        final_pt_b = gp_Pnt2d(pt_a).Translated(gp_Vec2d(constrained_dir) * dist_along_constrained);
      }

      // Disable snapping when angle constraint is active - angle takes priority
      edge.node_idx_b = std::nullopt;
    }
    // Apply distance constraint if set (and angle is not set)
    else if (m_sketch.m_dims.entered_edge_len().has_value())
    {
      final_pt_b      = gp_Pnt2d(pt_a).Translated(gp_Vec2d(m_sketch.m_dims.entered_edge_len()->dir) *
                                                  m_sketch.m_dims.entered_edge_len()->len);
      edge.node_idx_b = m_sketch.m_nodes.try_get_node_idx_snap(final_pt_b);
    }
    else
    {
      // No constraints - check for snap points at mouse position
      edge.node_idx_b = m_sketch.m_nodes.try_get_node_idx_snap(final_pt_b);
      if (edge.node_idx_b.has_value())
        final_pt_b = m_sketch.m_nodes[edge.node_idx_b];
    }

    double dist = pt_a.Distance(final_pt_b) / m_sketch.m_view.get_dimension_scale();
    m_sketch.m_dims.show_tmp_dim_preview(pt_a, final_pt_b);
    m_sketch.m_dims.offer_dist_edit_for_segment(pt_a, final_pt_b, dist);

    gp_Vec2d vec(pt_a, pt_b);
    m_sketch.m_dims.offer_angle_edit_for_segment(pt_a, pt_b, to_degrees(std::atan2(vec.Y(), vec.X())));

    if (unique(pt_a, final_pt_b))
      m_sketch.update_edge_shp_(edge, pt_a, final_pt_b);

    else if (edge.shp)
    {
      m_sketch.m_ctx.Remove(edge.shp, true);
      edge.shp.Nullify();
    }
  };

  move_sketch_pt_(screen_coords, l);
}

void Sketch_tools::finalize_edges_(Sketch_op_recorder& rec)
{
  if (m_sketch.m_nodes.empty() || m_tmp_edges.empty())
    return;

  if (!m_tmp_edges.back().node_idx_b.has_value())
  {
    // Last edge was not finalized, remove it
    m_sketch.m_ctx.Remove(m_tmp_edges.back().shp, false);
    m_tmp_edges.pop_back();
  }

  if (m_tmp_edges.empty())
    return;

  // Record user-intent edges before any split/append side effects.
  for (const Sketch_edge& te : m_tmp_edges)
  {
    EZY_ASSERT(te.node_idx_b.has_value());
    rec.note_curr_linear_edge(m_sketch.m_nodes[te.node_idx_a], m_sketch.m_nodes[*te.node_idx_b]);
  }

  std::vector<size_t> split_mid_points;
  for (Sketch_edge& e : m_tmp_edges)
  {
    EZY_ASSERT(e.node_idx_b.has_value());
    if (m_sketch.m_nodes[e.node_idx_a].midpoint)
      split_mid_points.push_back(e.node_idx_a);

    if (m_sketch.m_nodes[e.node_idx_b].midpoint)
      split_mid_points.push_back(*e.node_idx_b);
  }

  // Split any *existing* (pre this batch) linear edges that intersect or touch the
  // new segments being committed (the tmp ones). This fulfills the requirement that
  // adding edges from the GUI splits existing intersecting/touching edges.
  std::vector<gp_Pnt2d> batch_inters;
  {
    for (const Sketch_edge& te : m_tmp_edges)
    {
      // All remaining tmp edges must be complete at this point:
      // - The code above already popped any trailing incomplete one.
      // - The split_mid_points loop immediately before this used EZY_ASSERT on every te.
      // Therefore we can (and should) assert rather than silently continue.
      // (A defensive "if (!...) continue" would be appropriate for filtering *old* edges
      // in m_sketch.m_edges, which may contain arcs or other non-linear things, but not here.)
      EZY_ASSERT(te.node_idx_b.has_value());

      gp_Pnt2d pa = m_sketch.m_nodes[te.node_idx_a];
      gp_Pnt2d pb = m_sketch.m_nodes[te.node_idx_b];
      for (const Sketch_edge& oe : m_sketch.m_edges.edges()) // pre-batch olds only
      {
        if (Sketch::is_linear_edge_(oe))
        {
          gp_Pnt2d qa = m_sketch.m_nodes[oe.node_idx_a];
          gp_Pnt2d qb = m_sketch.m_nodes[oe.node_idx_b];
          if (auto inter = segment_intersection_2d(pa, pb, qa, qb, Segment_inclusion::Closed))
            add_unique_point(batch_inters, *inter);
        }
        else if (sketch_edge_is_arc(oe))
        {
          for (const gp_Pnt2d& inter :
               segment_arc_intersections_2d(pa, pb, TopoDS::Edge(oe.shp->Shape()), m_sketch.m_pln, Segment_inclusion::Closed))
            add_unique_point(batch_inters, inter);
        }
      }
    }

    // Only split olds at interior hits (avoid snap on endpoint-touch inters from the batch, same reason as add_edge_).
    std::vector<gp_Pnt2d> batch_to_split;
    for (const auto& ip : batch_inters)
    {
      bool is_old_interior = false;
      for (const Sketch_edge& e : m_sketch.m_edges.edges())
      {
        if (Sketch::is_linear_edge_(e))
        {
          gp_Pnt2d qa = m_sketch.m_nodes[e.node_idx_a];
          gp_Pnt2d qb = m_sketch.m_nodes[e.node_idx_b];
          if (point_on_open_segment_2d(ip, qa, qb))
          {
            is_old_interior = true;
            break;
          }
        }
        else if (sketch_edge_is_arc(e))
        {
          if (point_on_open_arc_interior_2d(ip, TopoDS::Edge(e.shp->Shape()), m_sketch.m_pln))
          {
            is_old_interior = true;
            break;
          }
        }
      }

      if (is_old_interior)
        add_unique_point(batch_to_split, ip);
    }

    for (const auto& ip : batch_to_split)
    {
      const size_t nidx = m_sketch.m_nodes.get_node_exact(ip);
      m_sketch.m_topo.split_linear_edges_at_node_if_interior(nidx, rec);
      m_sketch.m_topo.split_arcs_at_node_if_interior(nidx, rec);
    }
  }

  append(m_sketch.m_edges.edges(), m_tmp_edges);
  m_tmp_edges.clear();

  // Ensure topology is correct if snapping on a midpoint happened.
  // These edges need to be split. (Kept for compatibility with current midpoint marking during draw.)
  for (size_t mid_pt_idx : split_mid_points)
    for (auto itr = m_sketch.m_edges.edges().begin(), end = m_sketch.m_edges.edges().end(); itr != end; ++itr)
      if (itr->node_idx_mid.has_value() && *itr->node_idx_mid == mid_pt_idx)
        if (sketch_edge_is_linear(*itr))
        {
          // Split the edge.
          Sketch_edge edge_a{itr->node_idx_a, mid_pt_idx};
          Sketch_edge edge_b{mid_pt_idx, *itr->node_idx_b};
          m_sketch.update_edge_shp_(edge_a, m_sketch.m_nodes[itr->node_idx_a], m_sketch.m_nodes[mid_pt_idx]);
          m_sketch.update_edge_shp_(edge_b, m_sketch.m_nodes[itr->node_idx_b], m_sketch.m_nodes[mid_pt_idx]);
          rec.note_prev_linear_edge(itr->node_idx_a, *itr->node_idx_b, itr->node_idx_mid, itr->name);
          // Set midpoints for the new split edges so they can be snapped to
          edge_a.node_idx_mid = m_sketch.m_nodes.add_new_node(
              get_midpoint(m_sketch.m_nodes[itr->node_idx_a], m_sketch.m_nodes[mid_pt_idx]), true);
          edge_b.node_idx_mid = m_sketch.m_nodes.add_new_node(
              get_midpoint(m_sketch.m_nodes[mid_pt_idx], m_sketch.m_nodes[itr->node_idx_b]), true);
          rec.note_curr_node(edge_a.node_idx_mid.value());
          rec.note_curr_node(edge_b.node_idx_mid.value());
          m_sketch.m_ctx.Remove(itr->shp, false);
          m_sketch.m_edges.edges().erase(itr);
          m_sketch.m_nodes[mid_pt_idx].midpoint = false;
          m_sketch.m_edges.edges().emplace_back(edge_a);
          m_sketch.m_edges.edges().emplace_back(edge_b);
          break;
        }

  // Subdivide any *newly committed* edges (the ones from m_tmp) at interior intersections
  // discovered in the pre-pass. The pre-pass already handled olds; re-calling split here
  // will target the news (olds' original long edges no longer exist). This ensures the
  // GUI finalize path produces the same atomic edge count as direct add_edge_ (e.g. 4
  // edges for a cross, whether or not the cross is at an existing mid).
  for (const auto& ip : batch_inters)
  {
    const size_t nidx = m_sketch.m_nodes.get_node_exact(ip);
    rec.note_curr_node(nidx);
    m_sketch.m_topo.split_linear_edges_at_node_if_interior(nidx, rec);
    m_sketch.m_topo.split_arcs_at_node_if_interior(nidx, rec);
  }

  m_sketch.m_nodes.hide_snap_annos();
  m_sketch.update_faces_();
}

void Sketch_tools::add_node_pt_(const ScreenCoords& screen_coords)
{
  auto commit_b = [this](size_t node_b)
  {
    Sketch_op_recorder rec(m_sketch.m_view, m_sketch);
    {
      rec.note_curr_node(node_b);
      m_sketch.m_topo.split_linear_edges_at_node_if_interior(node_b, rec);
      m_sketch.m_topo.split_arcs_at_node_if_interior(node_b, rec);

      // Add-node mode: the rubber band is only for placement (snap / dimension / angle). Do not create
      // a sketch edge between the anchor and the new node - only nodes and interior splits matter.
      m_tmp_node_idxs.clear();
      clear_tmps();
      m_sketch.m_dims.clear_tmp_dim_anno();
      m_sketch.m_nodes.hide_snap_annos();
      m_sketch.update_faces_();
      rec.commit();
    }
  };

  if (m_sketch.m_dims.entered_edge_angle().has_value() && m_tmp_edges.size())
  {
    std::optional<gp_Pnt2d> pt_opt = m_sketch.m_view.pt_on_plane(screen_coords, m_sketch.m_pln);
    if (!pt_opt)
      return;

    const gp_Pnt2d& pt_a      = m_sketch.m_nodes[m_tmp_edges.back().node_idx_a];
    double          angle_rad = to_radians(*m_sketch.m_dims.entered_edge_angle());
    gp_Dir2d        constrained_dir(std::cos(angle_rad), std::sin(angle_rad));
    gp_Vec2d        to_click(pt_opt->X() - pt_a.X(), pt_opt->Y() - pt_a.Y());
    double          dist_along = to_click.Dot(gp_Vec2d(constrained_dir));
    gp_Pnt2d        final_pt   = gp_Pnt2d(pt_a).Translated(gp_Vec2d(constrained_dir) * dist_along);
    if (!unique(pt_a, final_pt))
      return;

    const size_t node_idx = m_sketch.m_nodes.get_node_exact(final_pt, true);
    commit_b(node_idx);
    return;
  }

  if (!m_sketch.m_view.pt_on_plane(screen_coords, m_sketch.m_pln))
    return;

  auto check_node = [&](const std::optional<size_t>& snap_to_node, const gp_Pnt2d& pt)
  {
    if (!m_tmp_edges.empty())
    {
      Sketch_edge& last   = m_tmp_edges.back();
      size_t       node_b = snap_to_node ? *snap_to_node : m_sketch.m_nodes.add_new_node(pt, false, true);
      if (node_b == last.node_idx_a)
        return;

      commit_b(node_b);
      return;
    }

    // No tmp edge yet: snapping to an existing node starts a rubber-band segment from that anchor;
    // the next placement finishes the edge. Clear angle/dimension UI from any prior mode.
    if (snap_to_node)
    {
      auto start_rubber_from_anchor = [this](size_t idx_a)
      {
        m_sketch.m_dims.entered_edge_angle() = std::nullopt;
        m_sketch.m_dims.entered_edge_len()   = std::nullopt;
        m_sketch.m_dims.set_show_angle_input(false);
        m_sketch.m_dims.set_show_dim_input(false);
        m_sketch.m_view.gui().hide_angle_edit();
        m_tmp_edges.push_back({idx_a});
      };

      start_rubber_from_anchor(*snap_to_node);
      return;
    }

    Sketch_op_recorder rec(m_sketch.m_view, m_sketch);
    {
      const size_t ni = m_sketch.m_nodes.add_new_node(pt, false, true);
      rec.note_curr_node(ni);
      m_sketch.m_topo.split_linear_edges_at_node_if_interior(ni, rec);
      m_sketch.m_topo.split_arcs_at_node_if_interior(ni, rec);
      m_sketch.m_dims.clear_tmp_dim_anno();
      m_sketch.m_nodes.hide_snap_annos();
      m_sketch.update_faces_();
      rec.commit();
    }
  };

  move_sketch_pt_(screen_coords, check_node);
}

void Sketch_tools::move_add_node_pt_(const ScreenCoords& screen_coords)
{
  if (!m_tmp_edges.empty())
    move_line_string_pt_(screen_coords);
  else
    move_sketch_pt_(screen_coords, [](const std::optional<size_t>&, const gp_Pnt2d&) {});
}

// Arc circle related
void Sketch_tools::add_arc_circle_pt_(const ScreenCoords& screen_coords)
{
  std::optional<size_t> node_idx = m_sketch.m_nodes.get_node(screen_coords);
  if (!node_idx)
    return;

  if (!m_start_arc_circle_node_idx)
  {
    EZY_ASSERT(m_tmp_node_idxs.empty());
    m_start_arc_circle_node_idx = m_sketch.m_nodes.size();
  }

  if (contains(m_tmp_node_idxs, node_idx))
    // Arc points must be unique.
    return;

  m_tmp_node_idxs.push_back(*node_idx);

  if (m_tmp_node_idxs.size() == 3)
  {
    Sketch_op_recorder rec(m_sketch.m_view, m_sketch);
    {
      const gp_Pnt2d& pt_a = m_sketch.m_nodes[m_tmp_node_idxs[0]];
      const gp_Pnt2d& pt_c = m_sketch.m_nodes[m_tmp_node_idxs[1]];
      const gp_Pnt2d& pt_b = m_sketch.m_nodes[m_tmp_node_idxs[2]];
      m_sketch.add_arc_circle_(pt_a, pt_b, pt_c, rec);
      m_tmp_node_idxs.clear();
      m_sketch.update_faces_();
      rec.commit();
    }
  }
}

void Sketch_tools::move_arc_circle_pt_(const ScreenCoords& screen_coords)
{
  std::optional<gp_Pnt2d> pt = m_sketch.m_nodes.snap(screen_coords);
  if (!pt)
    // View plane and sketch plane must be perpendicular.
    return;

  if (m_tmp_node_idxs.size() != 2)
    return;

  gp_Pnt a = m_sketch.to_3d_(m_tmp_node_idxs[0]);
  gp_Pnt b = to_3d(m_sketch.m_pln, *pt);
  gp_Pnt c = m_sketch.to_3d_(m_tmp_node_idxs[1]);

  Handle(Geom_TrimmedCurve) arc_circle = GC_MakeArcOfCircle(a, b, c);
  if (!arc_circle)
  {
    m_sketch.m_view.remove(m_tmp_shp);
    m_tmp_shp = nullptr;
    return;
  }

  TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(arc_circle);
  show(m_sketch.m_ctx, m_tmp_shp, edge);
}

void Sketch_tools::move_square_pt_(const ScreenCoords& screen_coords)
{
  move_line_string_pt_(screen_coords);

  auto l = [&](Sketch_edge& e, const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b)
  {
    TopoDS_Wire square = make_square_wire(m_sketch.m_pln, pt_a, *m_last_pt);
    show(m_sketch.m_ctx, m_tmp_shp, square);
  };

  if_edge_pt_valid_(l);
}

void Sketch_tools::finalize_square_(Sketch_op_recorder& rec)
{
  auto l = [this, &rec](Sketch_edge& e, const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b)
  {
    std::array<gp_Pnt2d, 4> corners = square_corners(pt_a, pt_b);
    m_sketch.add_edge_(corners[0], corners[1], rec);
    m_sketch.add_edge_(corners[1], corners[2], rec);
    m_sketch.add_edge_(corners[2], corners[3], rec);
    m_sketch.add_edge_(corners[3], corners[0], rec);
    clear_tmps();
    m_sketch.update_faces_();
  };

  if_edge_pt_valid_(l);
}

void Sketch_tools::move_rectangle_pt_(const ScreenCoords& screen_coords)
{
  move_line_string_pt_(screen_coords);

  auto l = [&](Sketch_edge& e, const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b)
  {
    EZY_ASSERT(m_tmp_edges.size());

    if (std::abs(pt_a.X() - pt_b.X()) > Precision::Confusion() && std::abs(pt_a.Y() - pt_b.Y()) > Precision::Confusion())
    {
      TopoDS_Wire rectangle = make_rectangle_wire(m_sketch.m_pln, pt_a, pt_b);
      show(m_sketch.m_ctx, m_tmp_shp, rectangle);
    }
    else
    {
      m_sketch.m_ctx.Remove(m_tmp_shp, true);
      m_tmp_shp.Nullify();
    }
  };

  if_edge_pt_valid_(l);
}

void Sketch_tools::finalize_rectangle_(Sketch_op_recorder& rec)
{
  auto l = [this, &rec](Sketch_edge& e, const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b)
  {
    if (std::abs(pt_a.X() - pt_b.X()) > Precision::Confusion() && std::abs(pt_a.Y() - pt_b.Y()) > Precision::Confusion())
    {
      std::array<gp_Pnt2d, 4> corners = rectangle_corners(pt_a, pt_b);
      m_sketch.add_edge_(corners[0], corners[1], rec);
      m_sketch.add_edge_(corners[1], corners[2], rec);
      m_sketch.add_edge_(corners[2], corners[3], rec);
      m_sketch.add_edge_(corners[3], corners[0], rec);
      clear_tmps();
      m_sketch.update_faces_();
    }
    else
    {
      // Start a new edge
      EZY_ASSERT(e.node_idx_b.has_value());
      m_tmp_edges.push_back({*e.node_idx_b});
    }
  };

  if_edge_pt_valid_(l);
}

void Sketch_tools::move_circle_pt_(const ScreenCoords& screen_coords)
{
  move_line_string_pt_(screen_coords);
  auto l = [&](Sketch_edge& e, const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b)
  {
    TopoDS_Wire circle = make_circle_wire(m_sketch.m_pln, pt_a, *m_last_pt);
    show(m_sketch.m_ctx, m_tmp_shp, circle);
  };
  if_edge_pt_valid_(l);
}

void Sketch_tools::finalize_circle_(Sketch_op_recorder& rec)
{
  auto l = [this, &rec](Sketch_edge& e, const gp_Pnt2d& center, const gp_Pnt2d& edge_pt)
  {
    std::array<gp_Pnt2d, 4> points = xy_stencil_pnts(center, edge_pt);
    m_sketch.add_arc_circle_(points[0], points[2], points[1], rec);
    m_sketch.add_arc_circle_(points[0], points[3], points[1], rec);
    clear_tmps();
    m_sketch.update_faces_();
  };
  if_edge_pt_valid_(l);
}

// Slot related
void Sketch_tools::move_slot_pt_(const ScreenCoords& screen_coords)
{
  move_line_string_pt_(screen_coords);
  auto l = [&](Sketch_edge& e, const gp_Pnt2d& pt_b, const gp_Pnt2d& pt_c)
  {
    if (m_tmp_edges.size() != 2)
      return;

    const gp_Pnt2d& pt_a = m_sketch.m_nodes[m_tmp_edges.front().node_idx_a];
    if (unique(pt_a, pt_b, pt_c))
      show(m_sketch.m_ctx, m_tmp_shp, make_slot_wire(m_sketch.m_pln, pt_a, pt_b, pt_c));
  };
  if_edge_pt_valid_(l);
}

void Sketch_tools::finalize_slot_(Sketch_op_recorder& rec)
{
  EZY_ASSERT(m_tmp_edges.size() == 2);
  EZY_ASSERT(m_tmp_edges[1].node_idx_b.has_value());

  Slot_pnts pnts = get_slot_points(m_sketch.m_nodes[m_tmp_edges[0].node_idx_a], m_sketch.m_nodes[m_tmp_edges[1].node_idx_a],
                                   m_sketch.m_nodes[m_tmp_edges[1].node_idx_b]);

  m_sketch.add_edge_(pnts.a_top_2d, pnts.b_top_2d, rec);
  m_sketch.add_edge_(pnts.a_bottom_2d, pnts.b_bottom_2d, rec);
  m_sketch.add_arc_circle_(pnts.a_bottom_2d, pnts.a_mid_2d, pnts.a_top_2d, rec);
  m_sketch.add_arc_circle_(pnts.b_bottom_2d, pnts.b_mid_2d, pnts.b_top_2d, rec);
  clear_tmps();
  m_sketch.update_faces_();
}

// Operation axis related
void Sketch_tools::add_operation_axis_pt_(const ScreenCoords& screen_coords)
{
  // Axis definition only happens when !has_operation_axis() (guarded at the call site in GUI).
  // To redefine, the caller must first clear via the Options "Clear axis" button.
  add_line_string_pt_(screen_coords, Sketch_tools::Linestring_type::Single);
}

void Sketch_tools::finalize_operation_axis_(Sketch_op_recorder& rec)
{
  m_sketch.m_operation_axis = std::move(m_tmp_edges.back());
  if (m_sketch.m_operation_axis->node_idx_b.has_value())
    rec.note_curr_operation_axis(m_sketch.m_nodes[m_sketch.m_operation_axis->node_idx_a],
                                 m_sketch.m_nodes[*m_sketch.m_operation_axis->node_idx_b]);

  m_sketch.cancel_elm(); // Reset state, we have the operation axis
  m_sketch.sync_operation_axis_display_();
  m_sketch.m_node_marks.sync();
  m_sketch.m_nodes.hide_snap_annos();
}

bool Sketch_tools::clear_tmps()
{
  m_sketch.m_view.remove(m_tmp_shp);
  for (Sketch_edge& e : m_tmp_edges)
    m_sketch.m_view.remove(e.shp);

  if (m_tmp_shp)
  {
    m_sketch.m_ctx.Remove(m_tmp_shp, true);
    m_tmp_shp = nullptr;
  }

  const bool operation_canceled = !m_tmp_edges.empty();
  clear_all(m_tmp_node_idxs, m_tmp_shp, m_tmp_edges);
  m_sketch.m_dims.on_clear_tmps();

  return operation_canceled;
}

// General sketch point related
template <typename Callback>
void Sketch_tools::add_sketch_pt_(const ScreenCoords& screen_coords, size_t required_num_pts, Callback&& callback)
{
  auto l = [&](const std::optional<size_t>& node_idx, const gp_Pnt2d& pt)
  {
    if (node_idx)
      m_tmp_node_idxs.push_back(*node_idx);
    else
      m_tmp_node_idxs.push_back(m_sketch.m_nodes.add_new_node(pt));

    if (m_tmp_node_idxs.size() >= required_num_pts)
      callback(m_tmp_node_idxs.back());
  };
  move_sketch_pt_(screen_coords, l);
}

template <typename Callback> void Sketch_tools::move_sketch_pt_(const ScreenCoords& screen_coords, Callback&& callback)
{
  m_last_pt = m_sketch.m_view.pt_on_plane(screen_coords, m_sketch.m_pln);
  if (!m_last_pt)
    // View plane and sketch plane must be perpendicular.
    return;

  std::optional<size_t> node_idx = m_sketch.m_nodes.try_get_node_idx_snap(*m_last_pt);

  callback(node_idx, *m_last_pt);
}

/// Invokes callback(e, pt_a, pt_b) with the last tmp edge only when it exists and is non-degenerate.
template <typename Callback> void Sketch_tools::if_edge_pt_valid_(Callback&& callback)
{
  if (m_tmp_edges.empty())
    return;

  Sketch_edge&    e    = m_tmp_edges.back();
  const gp_Pnt2d& pt_a = m_sketch.m_nodes[e.node_idx_a];
  if (m_last_pt.has_value())
    if (unique(pt_a, *m_last_pt))
      callback(e, pt_a, *m_last_pt);
}

void Sketch_tools::finalize_add_node_elm_cleanup_()
{
  if (!m_tmp_edges.empty() && !m_tmp_edges.back().node_idx_b.has_value())
  {
    m_sketch.m_ctx.Remove(m_tmp_edges.back().shp, false);
    m_tmp_edges.pop_back();
  }

  m_sketch.m_nodes.hide_snap_annos();
  m_sketch.update_faces_();
}
