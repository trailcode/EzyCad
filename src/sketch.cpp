#include "sketch.h"

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <Graphic3d_AspectFillArea3d.hxx>
#include <Precision.hxx>
#include <PrsDim_LengthDimension.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>
#include <V3d_View.hxx>
#include <cmath>
#include <functional>
#include <map>

#include "utl_geom.h"
#include "gui.h"
#include "mode.h"
#include "occt_view.h"
#include "sketch_delta.h"
#include "utl_occt.h"
#include "utl.h"

using namespace glm;

struct Sketch_AIS_node_mark;

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

Sketch::Sketch(const std::string& name, Occt_view& view, const gp_Pln& pln)
    : m_view(view)
    , m_ctx(view.ctx())
    , m_pln(pln)
    , m_name(name)
    , m_nodes(view, pln)
    , m_edges(*this)
    , m_topo(*this)
    , m_dims(*this)
    , m_underlay(view.ctx())
{
}

Sketch::Sketch(const std::string& name, Occt_view& view, const gp_Pln& pln, const TopoDS_Wire& outer_wire)
    : m_view(view)
    , m_ctx(view.ctx())
    , m_pln(pln)
    , m_name(name)
    , m_nodes(view, pln)
    , m_edges(*this)
    , m_topo(*this)
    , m_dims(*this)
    , m_underlay(view.ctx())
{
  m_originating_face = new AIS_Shape(outer_wire);
  m_ctx.Display(m_originating_face, true);
  update_originating_face_style();
}

Sketch::~Sketch()
{
  m_underlay.ctx_erase();

  m_edges.remove_displayed();

  m_dims.remove_displayed();

  m_topo.remove_displayed_faces();

  if (m_operation_axis.has_value())
    m_ctx.Remove(m_operation_axis->shp, false);

  remove_all_permanent_node_marks_();

  m_ctx.Remove(m_originating_face, true);
}

void Sketch::add_sketch_pt(const ScreenCoords& screen_coords)
{
  switch (get_mode())
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

void Sketch::sketch_pt_move(const ScreenCoords& screen_coords)
{
  switch (get_mode())
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

    case Mode::Sketch_dim_anno:
      (void)m_nodes.try_pick_existing_node(screen_coords);
      m_dims.update_len_dim_rubber_line_(screen_coords);
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

void Sketch::dimension_input(const ScreenCoords& screen_coords)
{
  m_dims.set_show_dim_input(true);
  sketch_pt_move(screen_coords);
}

void Sketch::angle_input(const ScreenCoords& screen_coords)
{
  m_dims.set_show_angle_input(true);
  sketch_pt_move(screen_coords);
}

void Sketch::remove_edge(const Sketch_AIS_edge& to_remove)
{
  m_edges.remove_by_ais(to_remove);
  update_faces_();
}

void Sketch::dbg_append_str(std::string& out) const
{
  out += "    Nodes: " + std::to_string(m_nodes.size()) + "\n";
  out += "    Edges: " + std::to_string(m_edges.size()) + "\n";
  out += "    Faces: " + std::to_string(m_topo.faces().size()) + "\n";
  out += "Tmp Nodes: " + std::to_string(m_tmp_node_idxs.size()) + "\n";
  out += "Tmp Edges: " + std::to_string(m_tmp_edges.size()) + "\n";
}

void Sketch::update_edge_style_(AIS_Shape_ptr& shp)
{
  switch (m_edge_style)
  {
  case Edge_style::Full:
    // shp->SetWidth(1.5);
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

void Sketch::update_node_mark_style_(AIS_Shape_ptr& shp)
{
  // Keep permanent node markers visually stable across sketch switching.
  shp->SetWidth(1.25);
  shp->SetColor(Quantity_NOC_RED);
  shp->SetTransparency(0.0);
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

void Sketch::update_edge_shp_(Edge& edge, const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b)
{
  EZY_ASSERT(unique(pt_a, pt_b));

  TopoDS_Shape edge_shape = BRepBuilderAPI_MakeEdge(to_3d(m_pln, pt_a), to_3d(m_pln, pt_b)).Edge();

  if (edge.shp)
  {
    edge.shp->Set(edge_shape);
    update_edge_style_(edge.shp);
    m_ctx.Redisplay(edge.shp, true);
  }
  else
  {
    edge.shp = new Sketch_AIS_edge(*this, edge_shape);
    update_edge_style_(edge.shp);
    m_ctx.Display(edge.shp, true);
  }
}

void Sketch::on_enter()
{
  switch (get_mode())
  {
    // clang-format off
    case Mode::Sketch_add_square:
    case Mode::Sketch_add_rectangle:
    case Mode::Sketch_add_rectangle_center_pt:
    case Mode::Sketch_add_circle:
    case Mode::Sketch_add_node:
      m_dims.check_dimension_rubber_();
      break;
    case Mode::Sketch_add_edge:
      m_dims.check_dimension_seg_(static_cast<int>(Linestring_type::Single));
      break;
    case Mode::Sketch_add_slot:
      m_dims.check_dimension_seg_(static_cast<int>(Linestring_type::Two));
      break;
    case Mode::Sketch_add_multi_edges:
      m_dims.check_dimension_seg_(static_cast<int>(Linestring_type::Multiple));
      break;
    // clang-format on
  default:
    break;
  }
}

// Line string related
bool Sketch::edge_from_center_active_() const
{
  return s_edge_from_center && get_mode() == Mode::Sketch_add_edge && !m_tmp_edges.empty() &&
         !m_tmp_edges.back().node_idx_b.has_value();
}

bool Sketch::complete_edge_from_center_(const ScreenCoords& screen_coords)
{
  if (!edge_from_center_active_())
    return false;

  Edge&           edge   = m_tmp_edges.back();
  const gp_Pnt2d& center = m_nodes[edge.node_idx_a];

  std::optional<Symmetric_edge_span> span;
  if (m_dims.entered_edge_len().has_value())
    span = symmetric_edge_from_center(center, m_dims.entered_edge_len()->dir, m_dims.entered_edge_len()->len);

  else if (m_dims.entered_edge_angle().has_value())
  {
    std::optional<gp_Pnt2d> pt_opt = m_view.pt_on_plane(screen_coords, m_pln);
    if (!pt_opt)
      return true;

    const double angle_rad = to_radians(*m_dims.entered_edge_angle());
    gp_Dir2d     dir(std::cos(angle_rad), std::sin(angle_rad));
    gp_Vec2d     to_click(center, *pt_opt);
    const double half = std::abs(to_click.Dot(gp_Vec2d(dir)));
    span              = symmetric_edge_from_center(center, dir, half * 2.0);
  }
  else
  {
    std::optional<gp_Pnt2d> pt_opt = m_view.pt_on_plane(screen_coords, m_pln);
    if (!pt_opt)
      return true;

    span = symmetric_edge_from_center_and_hint(center, *pt_opt);
  }

  if (!span)
    return true;

  edge.node_idx_a = m_nodes.get_node_exact(span->pt_a);
  update_edge_end_pt_(edge, m_nodes.get_node_exact(span->pt_b));
  m_dims.entered_edge_angle() = std::nullopt;
  m_dims.entered_edge_len()   = std::nullopt;
  m_dims.set_show_angle_input(false);
  m_dims.set_show_dim_input(false);
  m_view.gui().hide_angle_edit();
  finalize_elm();
  return true;
}

void Sketch::add_line_string_pt_(const ScreenCoords& screen_coords, Linestring_type linestring_type)
{
  if (edge_from_center_active_() && linestring_type == Linestring_type::Single)
  {
    complete_edge_from_center_(screen_coords);
    return;
  }

  auto on_line_string = [&](size_t node_idx)
  {
    if (m_tmp_edges.size())
    {
      Edge& last_edge = m_tmp_edges.back();
      if (node_idx == last_edge.node_idx_a)
        // We cannot have edges with zero length.
        return;

      update_edge_end_pt_(last_edge, node_idx);

      if (linestring_type == Linestring_type::Single)
      {
        finalize_elm();
        return;
      }
      else if (linestring_type == Linestring_type::Two)
        if (m_tmp_edges.size() == 2)
        {
          finalize_elm();
          return;
        }
    }

    // Start a new edge - clear constraints for fresh start (click path for multi-line)
    m_dims.entered_edge_angle() = std::nullopt;
    m_dims.entered_edge_len()   = std::nullopt;
    m_dims.set_show_angle_input(false);
    m_view.gui().hide_angle_edit();
    m_tmp_edges.push_back({node_idx});
  };

  // When angle constraint is active, finalize on the constrained line (project click onto it)
  if (m_dims.entered_edge_angle().has_value() && m_tmp_edges.size())
  {
    if (edge_from_center_active_())
    {
      complete_edge_from_center_(screen_coords);
      return;
    }

    std::optional<gp_Pnt2d> pt_opt = m_view.pt_on_plane(screen_coords, m_pln);
    if (!pt_opt)
      return;

    const gp_Pnt2d& pt_a      = m_nodes[m_tmp_edges.back().node_idx_a];
    double          angle_rad = to_radians(*m_dims.entered_edge_angle());
    gp_Dir2d        constrained_dir(std::cos(angle_rad), std::sin(angle_rad));
    gp_Vec2d        to_click(pt_opt->X() - pt_a.X(), pt_opt->Y() - pt_a.Y());
    double          dist_along = to_click.Dot(gp_Vec2d(constrained_dir));
    gp_Pnt2d        final_pt   = gp_Pnt2d(pt_a).Translated(gp_Vec2d(constrained_dir) * dist_along);
    if (!unique(pt_a, final_pt))
      return;

    // Do not run try_get_node_idx_snap here: it can move the point off the angle ray (axis snap).
    const size_t node_idx = m_nodes.get_node_exact(final_pt);
    m_tmp_node_idxs.push_back(node_idx);
    on_line_string(node_idx);
    return;
  }

  add_sketch_pt_(screen_coords, 1, on_line_string);
}

void Sketch::move_line_string_pt_(const ScreenCoords& screen_coords)
{
  if (edge_from_center_active_())
  {
    auto l = [&](const std::optional<size_t>&, const gp_Pnt2d& pt_b)
    {
      Edge&           edge   = m_tmp_edges.back();
      const gp_Pnt2d& center = m_nodes[edge.node_idx_a];

      std::optional<Symmetric_edge_span> span;
      if (m_dims.entered_edge_angle().has_value())
      {
        const double angle_rad = to_radians(*m_dims.entered_edge_angle());
        gp_Dir2d     dir(std::cos(angle_rad), std::sin(angle_rad));
        if (m_dims.entered_edge_len().has_value())
          span = symmetric_edge_from_center(center, dir, m_dims.entered_edge_len()->len);
        else
        {
          gp_Vec2d     to_mouse(center, pt_b);
          const double half = std::abs(to_mouse.Dot(gp_Vec2d(dir)));
          span              = symmetric_edge_from_center(center, dir, half * 2.0);
        }
      }
      else if (m_dims.entered_edge_len().has_value())
        span = symmetric_edge_from_center(center, m_dims.entered_edge_len()->dir, m_dims.entered_edge_len()->len);
      else
        span = symmetric_edge_from_center_and_hint(center, pt_b);

      if (!span)
      {
        if (edge.shp)
        {
          m_ctx.Remove(edge.shp, true);
          edge.shp.Nullify();
        }
        m_dims.clear_tmp_dim_anno();
        return;
      }

      const gp_Pnt2d& span_a = span->pt_a;
      const gp_Pnt2d& span_b = span->pt_b;
      update_edge_shp_(edge, span_a, span_b);

      const double dist = span->full_len / m_view.get_dimension_scale();
      m_dims.show_tmp_dim_preview(span_a, span_b);
      m_dims.offer_dist_edit_for_segment(span_a, span_b, dist);
      m_dims.offer_angle_edit_for_segment(center, pt_b,
                                          to_degrees(std::atan2(gp_Vec2d(center, pt_b).Y(), gp_Vec2d(center, pt_b).X())));
    };

    move_sketch_pt_(screen_coords, l);
    return;
  }

  auto l = [&](const std::optional<size_t>& node_idx_b, const gp_Pnt2d& pt_b)
  {
    if (m_tmp_edges.empty())
      return;

    Edge&           edge = m_tmp_edges.back();
    const gp_Pnt2d& pt_a = m_nodes[edge.node_idx_a];

    if (!unique(pt_a, pt_b))
      // Cannot have a edge with zero length
      return;

    gp_Pnt2d final_pt_b = pt_b;

    // Apply angle constraint if set - this takes priority
    if (m_dims.entered_edge_angle().has_value())
    {
      // Calculate direction based on angle (0 degrees = positive X axis, counterclockwise)
      double   angle_rad = to_radians(*m_dims.entered_edge_angle());
      gp_Dir2d constrained_dir(std::cos(angle_rad), std::sin(angle_rad));

      // If distance is also constrained, use that
      if (m_dims.entered_edge_len().has_value())
        // Use the constrained distance - angle constraint is always enforced
        final_pt_b = gp_Pnt2d(pt_a).Translated(gp_Vec2d(constrained_dir) * m_dims.entered_edge_len()->len);
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
    else if (m_dims.entered_edge_len().has_value())
    {
      final_pt_b      = gp_Pnt2d(pt_a).Translated(gp_Vec2d(m_dims.entered_edge_len()->dir) * m_dims.entered_edge_len()->len);
      edge.node_idx_b = m_nodes.try_get_node_idx_snap(final_pt_b);
    }
    else
    {
      // No constraints - check for snap points at mouse position
      edge.node_idx_b = m_nodes.try_get_node_idx_snap(final_pt_b);
      if (edge.node_idx_b.has_value())
        final_pt_b = m_nodes[edge.node_idx_b];
    }

    double dist = pt_a.Distance(final_pt_b) / m_view.get_dimension_scale();
    m_dims.show_tmp_dim_preview(pt_a, final_pt_b);
    m_dims.offer_dist_edit_for_segment(pt_a, final_pt_b, dist);

    gp_Vec2d vec(pt_a, pt_b);
    m_dims.offer_angle_edit_for_segment(pt_a, pt_b, to_degrees(std::atan2(vec.Y(), vec.X())));

    if (unique(pt_a, final_pt_b))
      update_edge_shp_(edge, pt_a, final_pt_b);

    else if (edge.shp)
    {
      m_ctx.Remove(edge.shp, true);
      edge.shp.Nullify();
    }
  };

  move_sketch_pt_(screen_coords, l);
}

void Sketch::finalize_edges_(Sketch_op_recorder& rec)
{
  if (m_nodes.empty() || m_tmp_edges.empty())
    return;

  if (!m_tmp_edges.back().node_idx_b.has_value())
  {
    // Last edge was not finalized, remove it
    m_ctx.Remove(m_tmp_edges.back().shp, false);
    m_tmp_edges.pop_back();
  }

  if (m_tmp_edges.empty())
    return;

  // Record user-intent edges before any split/append side effects.
  for (const Edge& te : m_tmp_edges)
  {
    EZY_ASSERT(te.node_idx_b.has_value());
    rec.note_curr_linear_edge(m_nodes[te.node_idx_a], m_nodes[*te.node_idx_b]);
  }

  std::vector<size_t> split_mid_points;
  for (Edge& e : m_tmp_edges)
  {
    EZY_ASSERT(e.node_idx_b.has_value());
    if (m_nodes[e.node_idx_a].midpoint)
      split_mid_points.push_back(e.node_idx_a);

    if (m_nodes[e.node_idx_b].midpoint)
      split_mid_points.push_back(*e.node_idx_b);
  }

  // Split any *existing* (pre this batch) linear edges that intersect or touch the
  // new segments being committed (the tmp ones). This fulfills the requirement that
  // adding edges from the GUI splits existing intersecting/touching edges.
  std::vector<gp_Pnt2d> batch_inters;
  {
    for (const Edge& te : m_tmp_edges)
    {
      // All remaining tmp edges must be complete at this point:
      // - The code above already popped any trailing incomplete one.
      // - The split_mid_points loop immediately before this used EZY_ASSERT on every te.
      // Therefore we can (and should) assert rather than silently continue.
      // (A defensive "if (!...) continue" would be appropriate for filtering *old* edges
      // in m_edges, which may contain arcs or other non-linear things, but not here.)
      EZY_ASSERT(te.node_idx_b.has_value());

      gp_Pnt2d pa = m_nodes[te.node_idx_a];
      gp_Pnt2d pb = m_nodes[te.node_idx_b];
      for (const Edge& oe : m_edges.edges()) // pre-batch olds only
      {
        if (!is_linear_edge_(oe))
          continue;

        gp_Pnt2d qa = m_nodes[oe.node_idx_a];
        gp_Pnt2d qb = m_nodes[oe.node_idx_b];
        if (auto inter = segment_intersection_2d(pa, pb, qa, qb, Segment_inclusion::Closed))
        {
          // The intersection point is now guaranteed (by the inclusion parameter)
          // to lie on both finite closed segments. The previous custom verification
          // has been moved into segment_intersection_2d for consistency.
          add_unique_point(batch_inters, *inter);
        }
      }
    }

    // Only split olds at interior hits (avoid snap on endpoint-touch inters from the batch, same reason as add_edge_).
    std::vector<gp_Pnt2d> batch_to_split;
    for (const auto& ip : batch_inters)
    {
      bool is_old_interior = false;
      for (const Edge& e : m_edges.edges())
      {
        if (!is_linear_edge_(e))
          continue;

        gp_Pnt2d qa = m_nodes[e.node_idx_a];
        gp_Pnt2d qb = m_nodes[e.node_idx_b];
        if (point_on_open_segment_2d(ip, qa, qb))
        {
          is_old_interior = true;
          break;
        }
      }

      if (is_old_interior)
        add_unique_point(batch_to_split, ip);
    }

    for (const auto& ip : batch_to_split)
      m_topo.split_linear_edges_at_node_if_interior(m_nodes.get_node_exact(ip), rec);
  }

  append(m_edges.edges(), m_tmp_edges);
  m_tmp_edges.clear();

  // Ensure topology is correct if snapping on a midpoint happened.
  // These edges need to be split. (Kept for compatibility with current midpoint marking during draw.)
  for (size_t mid_pt_idx : split_mid_points)
    for (auto itr = m_edges.edges().begin(), end = m_edges.edges().end(); itr != end; ++itr)
      if (itr->node_idx_mid.has_value() && *itr->node_idx_mid == mid_pt_idx)
        // Cannot split arc circles.
        if (!itr->node_idx_arc.has_value())
        {
          // Split the edge.
          Edge edge_a{itr->node_idx_a, mid_pt_idx};
          Edge edge_b{mid_pt_idx, *itr->node_idx_b};
          update_edge_shp_(edge_a, m_nodes[itr->node_idx_a], m_nodes[mid_pt_idx]);
          update_edge_shp_(edge_b, m_nodes[itr->node_idx_b], m_nodes[mid_pt_idx]);
          rec.note_prev_linear_edge(itr->node_idx_a, *itr->node_idx_b, itr->node_idx_mid, itr->name);
          // Set midpoints for the new split edges so they can be snapped to
          edge_a.node_idx_mid = m_nodes.add_new_node(get_midpoint(m_nodes[itr->node_idx_a], m_nodes[mid_pt_idx]), true);
          edge_b.node_idx_mid = m_nodes.add_new_node(get_midpoint(m_nodes[mid_pt_idx], m_nodes[itr->node_idx_b]), true);
          rec.note_curr_node(edge_a.node_idx_mid.value());
          rec.note_curr_node(edge_b.node_idx_mid.value());
          m_ctx.Remove(itr->shp, false);
          m_edges.edges().erase(itr);
          m_nodes[mid_pt_idx].midpoint = false;
          m_edges.edges().emplace_back(edge_a);
          m_edges.edges().emplace_back(edge_b);
          break;
        }

  // Subdivide any *newly committed* edges (the ones from m_tmp) at interior intersections
  // discovered in the pre-pass. The pre-pass already handled olds; re-calling split here
  // will target the news (olds' original long edges no longer exist). This ensures the
  // GUI finalize path produces the same atomic edge count as direct add_edge_ (e.g. 4
  // edges for a cross, whether or not the cross is at an existing mid).
  for (const auto& ip : batch_inters)
  {
    const size_t nidx = m_nodes.get_node_exact(ip);
    rec.note_curr_node(nidx);
    m_topo.split_linear_edges_at_node_if_interior(nidx, rec);
  }

  m_nodes.hide_snap_annos();
  update_faces_();
}

void Sketch::add_node_pt_(const ScreenCoords& screen_coords)
{
  auto commit_b = [this](size_t node_b)
  {
    Sketch_op_recorder rec(m_view, *this);
    {
      rec.note_curr_node(node_b);
      m_topo.split_linear_edges_at_node_if_interior(node_b, rec);

      // Add-node mode: the rubber band is only for placement (snap / dimension / angle). Do not create
      // a sketch edge between the anchor and the new node - only nodes and interior splits matter.
      m_tmp_node_idxs.clear();
      clear_tmps_();
      m_dims.clear_tmp_dim_anno();
      m_nodes.hide_snap_annos();
      update_faces_();
      rec.commit();
    }
  };

  if (m_dims.entered_edge_angle().has_value() && m_tmp_edges.size())
  {
    std::optional<gp_Pnt2d> pt_opt = m_view.pt_on_plane(screen_coords, m_pln);
    if (!pt_opt)
      return;

    const gp_Pnt2d& pt_a      = m_nodes[m_tmp_edges.back().node_idx_a];
    double          angle_rad = to_radians(*m_dims.entered_edge_angle());
    gp_Dir2d        constrained_dir(std::cos(angle_rad), std::sin(angle_rad));
    gp_Vec2d        to_click(pt_opt->X() - pt_a.X(), pt_opt->Y() - pt_a.Y());
    double          dist_along = to_click.Dot(gp_Vec2d(constrained_dir));
    gp_Pnt2d        final_pt   = gp_Pnt2d(pt_a).Translated(gp_Vec2d(constrained_dir) * dist_along);
    if (!unique(pt_a, final_pt))
      return;

    const size_t node_idx = m_nodes.get_node_exact(final_pt, true);
    commit_b(node_idx);
    return;
  }

  if (!m_view.pt_on_plane(screen_coords, m_pln))
    return;

  auto check_node = [&](const std::optional<size_t>& snap_to_node, const gp_Pnt2d& pt)
  {
    if (!m_tmp_edges.empty())
    {
      Edge&  last   = m_tmp_edges.back();
      size_t node_b = snap_to_node ? *snap_to_node : m_nodes.add_new_node(pt, false, true);
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
        m_dims.entered_edge_angle() = std::nullopt;
        m_dims.entered_edge_len()   = std::nullopt;
        m_dims.set_show_angle_input(false);
        m_dims.set_show_dim_input(false);
        m_view.gui().hide_angle_edit();
        m_tmp_edges.push_back({idx_a});
      };

      start_rubber_from_anchor(*snap_to_node);
      return;
    }

    Sketch_op_recorder rec(m_view, *this);
    {
      const size_t ni = m_nodes.add_new_node(pt, false, true);
      rec.note_curr_node(ni);
      m_topo.split_linear_edges_at_node_if_interior(ni, rec);
      m_dims.clear_tmp_dim_anno();
      m_nodes.hide_snap_annos();
      update_faces_();
      rec.commit();
    }
  };

  move_sketch_pt_(screen_coords, check_node);
}

void Sketch::move_add_node_pt_(const ScreenCoords& screen_coords)
{
  if (!m_tmp_edges.empty())
    move_line_string_pt_(screen_coords);
  else
    move_sketch_pt_(screen_coords, [](const std::optional<size_t>&, const gp_Pnt2d&) {});
}

void Sketch::update_faces_() { m_topo.update_faces(); }

// Arc circle related
void Sketch::add_arc_circle_pt_(const ScreenCoords& screen_coords)
{
  std::optional<size_t> node_idx = m_nodes.get_node(screen_coords);
  if (!node_idx)
    return;

  if (!m_start_arc_circle_node_idx)
  {
    EZY_ASSERT(m_tmp_node_idxs.empty());
    m_start_arc_circle_node_idx = m_nodes.size();
  }

  if (contains(m_tmp_node_idxs, node_idx))
    // Arc points must be unique.
    return;

  m_tmp_node_idxs.push_back(*node_idx);

  if (m_tmp_node_idxs.size() == 3)
  {
    Sketch_op_recorder rec(m_view, *this);
    {
      const gp_Pnt2d& pt_a = m_nodes[m_tmp_node_idxs[0]];
      const gp_Pnt2d& pt_c = m_nodes[m_tmp_node_idxs[1]];
      const gp_Pnt2d& pt_b = m_nodes[m_tmp_node_idxs[2]];
      add_arc_circle_(pt_a, pt_b, pt_c, rec);
      m_tmp_node_idxs.clear();
      update_faces_();
      rec.commit();
    }
  }
}

void Sketch::move_arc_circle_pt_(const ScreenCoords& screen_coords)
{
  std::optional<gp_Pnt2d> pt = m_nodes.snap(screen_coords);
  if (!pt)
    // View plane and sketch plane must be perpendicular.
    return;

  if (m_tmp_node_idxs.size() != 2)
    return;

  gp_Pnt a = to_3d_(m_tmp_node_idxs[0]);
  gp_Pnt b = to_3d(m_pln, *pt);
  gp_Pnt c = to_3d_(m_tmp_node_idxs[1]);

  Handle(Geom_TrimmedCurve) arc_circle = GC_MakeArcOfCircle(a, b, c);
  if (!arc_circle)
  {
    m_view.remove(m_tmp_shp);
    m_tmp_shp = nullptr;
    return;
  }

  TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(arc_circle);
  show(m_ctx, m_tmp_shp, edge);
}

void Sketch::move_square_pt_(const ScreenCoords& screen_coords)
{
  move_line_string_pt_(screen_coords);

  auto l = [&](Edge& e, const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b)
  {
    TopoDS_Wire square = make_square_wire(m_pln, pt_a, *m_last_pt);
    show(m_ctx, m_tmp_shp, square);
  };

  if_edge_pt_valid_(l);
}

void Sketch::finalize_square_(Sketch_op_recorder& rec)
{
  auto l = [this, &rec](Edge& e, const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b)
  {
    std::array<gp_Pnt2d, 4> corners = square_corners(pt_a, pt_b);
    add_edge_(corners[0], corners[1], rec);
    add_edge_(corners[1], corners[2], rec);
    add_edge_(corners[2], corners[3], rec);
    add_edge_(corners[3], corners[0], rec);
    clear_tmps_();
    update_faces_();
  };

  if_edge_pt_valid_(l);
}

void Sketch::move_rectangle_pt_(const ScreenCoords& screen_coords)
{
  move_line_string_pt_(screen_coords);

  auto l = [&](Edge& e, const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b)
  {
    EZY_ASSERT(m_tmp_edges.size());

    if (std::abs(pt_a.X() - pt_b.X()) > Precision::Confusion() && std::abs(pt_a.Y() - pt_b.Y()) > Precision::Confusion())
    {
      TopoDS_Wire rectangle = make_rectangle_wire(m_pln, pt_a, pt_b);
      show(m_ctx, m_tmp_shp, rectangle);
    }
    else
    {
      m_ctx.Remove(m_tmp_shp, true);
      m_tmp_shp.Nullify();
    }
  };

  if_edge_pt_valid_(l);
}

void Sketch::finalize_rectangle_(Sketch_op_recorder& rec)
{
  auto l = [this, &rec](Edge& e, const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b)
  {
    if (std::abs(pt_a.X() - pt_b.X()) > Precision::Confusion() && std::abs(pt_a.Y() - pt_b.Y()) > Precision::Confusion())
    {
      std::array<gp_Pnt2d, 4> corners = rectangle_corners(pt_a, pt_b);
      add_edge_(corners[0], corners[1], rec);
      add_edge_(corners[1], corners[2], rec);
      add_edge_(corners[2], corners[3], rec);
      add_edge_(corners[3], corners[0], rec);
      clear_tmps_();
      update_faces_();
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

// Slot related
void Sketch::move_slot_pt_(const ScreenCoords& screen_coords)
{
  move_line_string_pt_(screen_coords);
  auto l = [&](Edge& e, const gp_Pnt2d& pt_b, const gp_Pnt2d& pt_c)
  {
    if (m_tmp_edges.size() != 2)
      return;

    const gp_Pnt2d& pt_a = m_nodes[m_tmp_edges.front().node_idx_a];
    if (unique(pt_a, pt_b, pt_c))
      show(m_ctx, m_tmp_shp, make_slot_wire(m_pln, pt_a, pt_b, pt_c));
  };
  if_edge_pt_valid_(l);
}

void Sketch::finalize_slot_(Sketch_op_recorder& rec)
{
  EZY_ASSERT(m_tmp_edges.size() == 2);
  EZY_ASSERT(m_tmp_edges[1].node_idx_b.has_value());

  Slot_pnts pnts = get_slot_points(m_nodes[m_tmp_edges[0].node_idx_a], m_nodes[m_tmp_edges[1].node_idx_a],
                                   m_nodes[m_tmp_edges[1].node_idx_b]);

  add_edge_(pnts.a_top_2d, pnts.b_top_2d, rec);
  add_edge_(pnts.a_bottom_2d, pnts.b_bottom_2d, rec);
  add_arc_circle_(pnts.a_bottom_2d, pnts.a_mid_2d, pnts.a_top_2d, rec);
  add_arc_circle_(pnts.b_bottom_2d, pnts.b_mid_2d, pnts.b_top_2d, rec);
  clear_tmps_();
  update_faces_();
}

// Operation axis related
void Sketch::add_operation_axis_pt_(const ScreenCoords& screen_coords)
{
  // Axis definition only happens when !has_operation_axis() (guarded at the call site in GUI).
  // To redefine, the caller must first clear via the Options "Clear axis" button.
  add_line_string_pt_(screen_coords, Linestring_type::Single);
}

void Sketch::finalize_operation_axis_(Sketch_op_recorder& rec)
{
  m_operation_axis = std::move(m_tmp_edges.back());
  if (m_operation_axis->node_idx_b.has_value())
    rec.note_curr_operation_axis(m_nodes[m_operation_axis->node_idx_a], m_nodes[*m_operation_axis->node_idx_b]);

  cancel_elm(); // Reset state, we have the operation axis
  sync_operation_axis_display_();
  sync_permanent_node_annos_();
  m_nodes.hide_snap_annos();
}

void Sketch::clear_operation_axis()
{
  if (m_operation_axis.has_value())
    m_ctx.Remove(m_operation_axis->shp, false);

  m_operation_axis = std::nullopt;
  sync_permanent_node_annos_();
  m_nodes.hide_snap_annos();
}

bool Sketch::has_operation_axis() const { return m_operation_axis.has_value(); }

void Sketch::mirror_selected_edges()
{
  EZY_ASSERT(m_operation_axis.has_value());
  Sketch_op_recorder rec(m_view, *this);

  const std::vector<Edge> mirror_edges = get_selected_edges_();
  if (mirror_edges.empty())
  {
    rec.cancel();
    m_view.gui().show_message(ERROR_NO_EDGES_SELECTED);
    return;
  }

  EZY_ASSERT(!m_operation_axis->shp.IsNull());
  const auto [mirror_pt_a, mirror_pt_b] = get_edge_endpoints(m_pln, TopoDS::Edge(m_operation_axis->shp->Shape()));

  std::vector<std::pair<gp_Pnt2d, gp_Pnt2d>>     mirrored_edges;
  std::map<AIS_Shape_ptr, std::set<const Edge*>> arc_circles;

  for (const Edge& e : m_edges.edges())
    for (const Edge& selected : mirror_edges)
      if (e.shp == selected.shp || e.circle_arc == selected.circle_arc)
      {
        if (e.circle_arc)
        {
          std::set<const Edge*>& arc_circle_edges = arc_circles[e.shp];
          arc_circle_edges.insert(&e);
          if (arc_circle_edges.size() == 2)
            break;
        }
        else
        {
          const gp_Pnt2d a = mirror_point(mirror_pt_a, mirror_pt_b, m_nodes[e.node_idx_a]);
          const gp_Pnt2d b = mirror_point(mirror_pt_a, mirror_pt_b, m_nodes[e.node_idx_b]);
          mirrored_edges.push_back({a, b});
          break;
        }
      }

  for (const auto& [a, b] : mirrored_edges)
    add_edge_(a, b, rec);

  for (auto& [_, arc_circle_edges] : arc_circles)
  {
    EZY_ASSERT(arc_circle_edges.size() == 2);
    const Edge* a = nullptr;
    const Edge* b = nullptr;
    for (const Edge* e : arc_circle_edges)
      if (e->node_idx_arc.has_value())
        a = e;
      else
        b = e;

    EZY_ASSERT(a && b);
    EZY_ASSERT(a->node_idx_arc.has_value());
    EZY_ASSERT(b->node_idx_b);
    const gp_Pnt2d pt_a = mirror_point(mirror_pt_a, mirror_pt_b, m_nodes[a->node_idx_a]);
    const gp_Pnt2d pt_b = mirror_point(mirror_pt_a, mirror_pt_b, m_nodes[a->node_idx_arc]);
    const gp_Pnt2d pt_c = mirror_point(mirror_pt_a, mirror_pt_b, m_nodes[b->node_idx_b]);
    add_arc_circle_(pt_a, pt_b, pt_c, rec);
  }

  m_nodes.hide_snap_annos();
  update_faces_();
  rec.commit();
}

Shp_rslt Sketch::revolve_selected(const double angle)
{
  EZY_ASSERT_MSG(m_operation_axis.has_value(), "No defined operation axis.");

  TopoDS_Compound compound;
  BRep_Builder    builder;
  builder.MakeCompound(compound);

  const std::vector<Sketch_face_shp_ptr> faces          = get_selected_faces_();
  const std::vector<Edge>                selected_edges = get_selected_edges_();
  if (selected_edges.size())
  {
    std::set<AIS_Shape*> seen;
    for (const Edge& e : selected_edges)
      // Arc circles share the same shape
      if (!seen.count(e.shp.get()))
      {
        seen.insert(e.shp.get());
        builder.Add(compound, e.shp->Shape());
      }
  }
  else if (faces.size())
    for (const Sketch_face_shp_ptr& face : faces)
      builder.Add(compound, face->Shape());

  else
    return Shp_rslt(Result_status::User_error, "No selected faces or edges.");

  try
  {
    const auto [pt_a, pt_b] = get_edge_endpoints(TopoDS::Edge(m_operation_axis->shp->Shape()));

    const gp_Dir direction((pt_b.XYZ() - pt_a.XYZ()).Normalized());
    const gp_Ax1 axis(pt_a, direction);

    BRepPrimAPI_MakeRevol revolMaker(compound, axis, angle);
    return Shp_rslt(new Shp(m_ctx, try_make_solid(revolMaker.Shape())));
  }
  catch (const Standard_Failure& e)
  {
    // Catch OCCT exception and return error with message
    std::string error_msg = "Revolution failed: ";
    const char* msg       = e.GetMessageString();
    error_msg += msg ? msg : "Unknown OCCT error";
    return Shp_rslt(Result_status::Topo_error, error_msg);
  }
  catch (const std::exception& e)
  {
    // Catch other unexpected errors
    return Shp_rslt(Result_status::User_error, "Unexpected error: " + std::string(e.what()));
  }
}

// General sketch point related
template <typename Callback>
void Sketch::add_sketch_pt_(const ScreenCoords& screen_coords, size_t required_num_pts, Callback&& callback)
{
  auto l = [&](const std::optional<size_t>& node_idx, const gp_Pnt2d& pt)
  {
    if (node_idx)
      m_tmp_node_idxs.push_back(*node_idx);
    else
      m_tmp_node_idxs.push_back(m_nodes.add_new_node(pt));

    if (m_tmp_node_idxs.size() >= required_num_pts)
      callback(m_tmp_node_idxs.back());
  };
  move_sketch_pt_(screen_coords, l);
}

template <typename Callback> void Sketch::move_sketch_pt_(const ScreenCoords& screen_coords, Callback&& callback)
{
  m_last_pt = m_view.pt_on_plane(screen_coords, m_pln);
  if (!m_last_pt)
    // View plane and sketch plane must be perpendicular.
    return;

  std::optional<size_t> node_idx = m_nodes.try_get_node_idx_snap(*m_last_pt);

  callback(node_idx, *m_last_pt);
}

/// Invokes callback(e, pt_a, pt_b) with the last tmp edge only when it exists and is non-degenerate.
template <typename Callback> void Sketch::if_edge_pt_valid_(Callback&& callback)
{
  if (m_tmp_edges.empty())
    return;

  Edge&           e    = m_tmp_edges.back();
  const gp_Pnt2d& pt_a = m_nodes[e.node_idx_a];
  if (m_last_pt.has_value())
    if (unique(pt_a, *m_last_pt))
      callback(e, pt_a, *m_last_pt);
}

void Sketch::finalize_add_node_elm_cleanup_()
{
  if (!m_tmp_edges.empty() && !m_tmp_edges.back().node_idx_b.has_value())
  {
    m_ctx.Remove(m_tmp_edges.back().shp, false);
    m_tmp_edges.pop_back();
  }

  m_nodes.hide_snap_annos();
  update_faces_();
}

void Sketch::add_edge_(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b) { m_edges.add_edge(pt_a, pt_b); }

void Sketch::add_edge_(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, Sketch_op_recorder& rec)
{
  m_edges.add_edge(pt_a, pt_b, rec);
}

void Sketch::add_edge_raw_(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b) { m_edges.add_edge_raw_(pt_a, pt_b); }

void Sketch::sketch_json_add_linear_edge_(size_t idx_a, size_t idx_b, std::optional<size_t> idx_mid)
{
  m_edges.sketch_json_add_linear_edge(idx_a, idx_b, idx_mid);
}

void Sketch::sketch_json_set_operation_axis_(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b)
{
  if (!unique(pt_a, pt_b))
    return;

  clear_operation_axis();

  Edge axis{m_nodes.get_node_exact(pt_a)};
  axis.node_idx_b = m_nodes.get_node_exact(pt_b);
  update_edge_shp_(axis, pt_a, pt_b);
  m_operation_axis = std::move(axis);

  sync_operation_axis_display_();
}

std::vector<Sketch::Edge> Sketch::get_selected_edges_() const { return m_edges.get_selected(); }

std::vector<Sketch_face_shp_ptr> Sketch::get_selected_faces_() const
{
  std::vector<Sketch_face_shp_ptr> ret;
  for (const AIS_Shape_ptr& selected : m_view.get_selected())
    for (const Sketch_face_shp_ptr& face : m_topo.faces())
      if (face == selected)
        ret.push_back(face);

  return ret;
}

std::list<Sketch::Edge>::iterator Sketch::get_edge_at_(const ScreenCoords& screen_coords)
{
  return m_edges.get_at(screen_coords);
}
bool Sketch::try_remove_length_dimension(PrsDim_LengthDimension* dim) { return m_dims.try_remove_length_dimension(dim); }

void Sketch::toggle_edge_dim_anno(const ScreenCoords& screen_coords) { m_dims.toggle_edge_dim_anno(screen_coords); }

void Sketch::json_add_length_dimension_(size_t node_a, size_t node_b, const bool visible,
                                        const std::optional<double> flyout_offset, const std::string& name)
{
  m_dims.json_add_length_dimension_(node_a, node_b, visible, flyout_offset, name);
}

void Sketch::remove_length_dimensions_referencing_node_(size_t node_idx)
{
  m_dims.remove_length_dimensions_referencing_node_(node_idx);
}

void Sketch::set_show_dims(bool show) { m_dims.set_show_dims(show); }

bool Sketch::shows_dimensions() const { return m_dims.shows_dimensions(); }

bool Sketch::dimension_visible(size_t dim_index) const { return m_dims.dimension_visible(dim_index); }

void Sketch::set_dimension_visible(size_t dim_index, bool visible) { m_dims.set_dimension_visible(dim_index, visible); }

size_t Sketch::dimension_node_lo(size_t dim_index) const { return m_dims.dimension_node_lo(dim_index); }

size_t Sketch::dimension_node_hi(size_t dim_index) const { return m_dims.dimension_node_hi(dim_index); }

double Sketch::dimension_offset(size_t dim_index) const { return m_dims.dimension_offset(dim_index); }

void Sketch::set_dimension_offset(size_t dim_index, const double offset) { m_dims.set_dimension_offset(dim_index, offset); }

std::string Sketch::dimension_name(size_t dim_index) const { return m_dims.dimension_name(dim_index); }

void Sketch::set_dimension_name(size_t dim_index, const std::string& name) { m_dims.set_dimension_name(dim_index, name); }

PrsDim_LengthDimension_ptr Sketch::length_dimension_handle(const size_t dim_index) const
{
  return m_dims.length_dimension_handle(dim_index);
}

void Sketch::refresh_annotations(const Sketch_annotation_refresh& refresh)
{
  if (refresh.length_dimensions)
    m_dims.refresh_all_length_dimensions();

  if (refresh.permanent_node_marks)
    sync_permanent_node_annos_();
}

size_t Sketch::length_dimension_count() const { return m_dims.length_dimension_count(); }

std::vector<std::string> Sketch::inspector_dimension_labels() const { return m_dims.inspector_dimension_labels(); }

std::vector<std::string> Sketch::inspector_node_labels() const
{
  std::vector<std::string> labels;
  for (size_t i = 0; i < m_nodes.size(); ++i)
  {
    const Sketch_nodes::Node& n = m_nodes[i];
    if (n.permanent && !n.deleted)
    {
      std::string lbl = n.name.empty() ? ("N" + std::to_string(i)) : n.name;
      labels.push_back(std::move(lbl));
    }
  }

  return labels;
}

void Sketch::finalize_elm()
{
  Sketch_op_recorder rec(m_view, *this);
  {
    m_dims.on_finalize_elm_start();

    switch (get_mode())
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

// Cancel related
bool Sketch::cancel_elm()
{
  bool operation_canceled = clear_tmps_();
  m_dims.clear_pick_state();
  m_dims.clear_tmp_dim_anno();
  m_nodes.hide_snap_annos();
  m_nodes.cancel();

  trim_trailing_permanent_node_marks_();

  return operation_canceled;
}
bool Sketch::s_add_mid_pt_edges = false;
bool Sketch::s_edge_from_center = false;

void Sketch::set_add_mid_pt_edges(bool on) { s_add_mid_pt_edges = on; }

bool Sketch::get_add_mid_pt_edges() { return s_add_mid_pt_edges; }

void Sketch::set_edge_from_center(bool on) { s_edge_from_center = on; }

bool Sketch::get_edge_from_center() { return s_edge_from_center; }

void Sketch::update_edge_end_pt_(Edge& edge, size_t end_pt_idx) { m_edges.update_end_pt(edge, end_pt_idx); }

void Sketch::add_arc_circle_(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, const gp_Pnt2d& pt_c)
{
  const size_t node_idx_a = m_nodes.get_node_exact(pt_a);
  const size_t node_idx_c = m_nodes.get_node_exact(pt_c);
  const size_t node_idx_b = m_nodes.get_node_exact(pt_b);
  add_arc_circle_(std::vector<size_t>{node_idx_a, node_idx_c, node_idx_b});
}

void Sketch::add_arc_circle_(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, const gp_Pnt2d& pt_c, Sketch_op_recorder& rec)
{
  const size_t node_idx_a = m_nodes.get_node_exact(pt_a);
  const size_t node_idx_c = m_nodes.get_node_exact(pt_c);
  const size_t node_idx_b = m_nodes.get_node_exact(pt_b);

  rec.note_curr_arc_edge(pt_a, pt_b, pt_c);
  rec.note_curr_node(node_idx_a);
  rec.note_curr_node(node_idx_c);
  rec.note_curr_node(node_idx_b);

  add_arc_circle_(std::vector<size_t>{node_idx_a, node_idx_c, node_idx_b});
}

void Sketch::add_arc_circle_(const std::vector<size_t>& node_idxs) { m_edges.add_arc_circle_edges(node_idxs); }

// Circle related
void Sketch::move_circle_pt_(const ScreenCoords& screen_coords)
{
  move_line_string_pt_(screen_coords);
  auto l = [&](Edge& e, const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b)
  {
    TopoDS_Wire circle = make_circle_wire(m_pln, pt_a, *m_last_pt);
    show(m_ctx, m_tmp_shp, circle);
  };
  if_edge_pt_valid_(l);
}

void Sketch::finalize_circle_(Sketch_op_recorder& rec)
{
  auto l = [this, &rec](Edge& e, const gp_Pnt2d& center, const gp_Pnt2d& edge_pt)
  {
    std::array<gp_Pnt2d, 4> points = xy_stencil_pnts(center, edge_pt);
    add_arc_circle_(points[0], points[2], points[1], rec);
    add_arc_circle_(points[0], points[3], points[1], rec);
    clear_tmps_();
    update_faces_();
  };
  if_edge_pt_valid_(l);
}

bool Sketch::clear_tmps_()
{
  m_view.remove(m_tmp_shp);
  for (Edge& e : m_tmp_edges)
    m_view.remove(e.shp);

  if (m_tmp_shp)
  {
    m_ctx.Remove(m_tmp_shp, true);
    m_tmp_shp = nullptr;
  }

  bool operation_canceled = m_tmp_edges.size();
  clear_all(m_tmp_node_idxs, m_tmp_shp, m_tmp_edges);
  m_dims.on_clear_tmps();

  return operation_canceled;
}

void Sketch::on_mode()
{
  // Reset state
  cancel_elm();
  sync_operation_axis_display_();
  sync_permanent_node_annos_();
}

bool Sketch::show_operation_axis_() const
{
  return m_operation_axis.has_value() && m_visible && is_current() && get_mode() == Mode::Sketch_operation_axis;
}

bool Sketch::operation_axis_suppresses_sketch_snap_() const
{
  return get_mode() == Mode::Sketch_operation_axis && m_operation_axis.has_value();
}

void Sketch::sync_operation_axis_display_()
{
  if (!m_operation_axis.has_value())
    return;

  if (show_operation_axis_())
    m_ctx.Display(m_operation_axis->shp, AIS_WireFrame, 0, false);
  else
    m_ctx.Erase(m_operation_axis->shp, false);
}

Mode Sketch::get_mode() const { return m_view.get_mode(); }

const gp_Pln& Sketch::get_plane() const { return m_pln; }

Sketch_nodes& Sketch::get_nodes() { return m_nodes; }

void Sketch::get_snap_pts_3d_(std::vector<gp_Pnt>& out)
{
  out.clear();
  get_originating_face_snp_pts_3d_(out);
  m_nodes.get_snap_pts_3d(out);
}

void Sketch::get_originating_face_snp_pts_3d_(std::vector<gp_Pnt>& out)
{
  if (!m_originating_face)
    return;

  std::vector<gp_Pnt> snp_pts;
  const TopoDS_Shape& shape = m_originating_face->Shape();
  EZY_ASSERT(shape.ShapeType() == TopAbs_WIRE);
  const TopoDS_Wire& wire = TopoDS::Wire(shape);
  TopExp_Explorer    edgeExplorer(wire, TopAbs_EDGE);
  while (edgeExplorer.More())
  {
    double             first, last;
    const TopoDS_Edge& edge  = TopoDS::Edge(edgeExplorer.Current());
    Geom_Curve_ptr     curve = BRep_Tool::Curve(edge, first, last);
    EZY_ASSERT(curve);

    TopoDS_Vertex v1, v2;
    TopExp::Vertices(edge, v1, v2);
    gp_Pnt p1 = BRep_Tool::Pnt(v1);
    gp_Pnt pc = curve->Value((first + last) * 0.5);
    gp_Pnt p2 = BRep_Tool::Pnt(v2);

    if (snp_pts.empty() || !snp_pts.back().IsEqual(p1, Precision::Confusion()))
      snp_pts.push_back(p1);

    snp_pts.push_back(pc);

    if (!snp_pts.front().IsEqual(p2, Precision::Confusion()))
      snp_pts.push_back(p2);

    edgeExplorer.Next();
  }

  append(out, snp_pts);
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
    sync_permanent_node_annos_();
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

    erase_permanent_node_marks_();
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
bool Sketch::is_current() const { return this == &m_view.curr_sketch(); }

void Sketch::set_current()
{
  m_ctx.CurrentViewer()->SetPrivilegedPlane(m_pln.Position());
  set_edge_style(Edge_style::Full);
  m_nodes.clear_outside_snap_pnts();

  m_tmp_pts_3d.clear();
  get_originating_face_snp_pts_3d_(m_tmp_pts_3d);

  for (const gp_Pnt& pt3d : m_tmp_pts_3d)
    // Insert 2D points on this `m_pln`, points will be considered the same using `Precision::Confusion()`
    m_nodes.add_outside_snap_pnt(pt3d);

  for (Sketch_ptr& sketch : m_view.get_sketches())
    if (sketch.get() != this)
    {
      sketch->set_edge_style(Edge_style::Background);

      // Hide operation axis from other sketches
      if (sketch->m_operation_axis.has_value())
        m_ctx.Erase(sketch->m_operation_axis->shp, false);

      if (sketch->is_visible())
      {
        // Add snap points from other sketches, but only if they are visible.
        sketch->get_snap_pts_3d_(m_tmp_pts_3d);
        for (const gp_Pnt& pt3d : m_tmp_pts_3d)
          // Insert 2D points on this `m_pln`, points will be considered the same using `Precision::Confusion()`
          m_nodes.add_outside_snap_pnt(pt3d);
      }
    }

  sync_operation_axis_display_();

  // DBG_MSG("m_outside_snap_pts " << m_nodes.m_outside_snap_pts.size());
}

void Sketch::set_edge_style(Edge_style style)
{
  m_edge_style = style;

  for (Edge& e : m_edges.edges())
    update_edge_style_(e.shp);

  // Permanent node marker style/visibility is managed elsewhere; avoid
  // rebuilding marker geometry during sketch switching.
  update_originating_face_style();
}

gp_Pnt Sketch::to_3d_(size_t node_idx) const { return to_3d(m_pln, m_nodes[node_idx]); }

gp_Pnt Sketch::to_3d_(const std::optional<size_t>& node_idx) const
{
  EZY_ASSERT(node_idx.has_value());
  return to_3d_(*node_idx);
}

const std::string& Sketch::get_name() const { return m_name; }

void Sketch::set_name(const std::string& name) { m_name = name; }

bool Sketch::has_edges() const { return !m_edges.empty(); }

size_t Sketch::edge_count() const { return m_edges.size(); }

size_t Sketch::face_count() const { return m_topo.faces().size(); }

std::vector<std::string> Sketch::inspector_edge_labels() const
{
  std::vector<std::string> labels;
  labels.reserve(m_edges.size());
  size_t idx = 0;
  for (const Edge& e : m_edges.edges())
  {
    std::string lbl = e.name.empty() ? ("E" + std::to_string(idx)) : e.name;
    labels.push_back(std::move(lbl));
    ++idx;
  }

  return labels;
}

std::vector<std::string> Sketch::inspector_face_labels() const
{
  std::vector<std::string> labels;
  labels.reserve(m_topo.faces().size());
  for (size_t i = 0; i < m_topo.faces().size(); ++i)
  {
    const Sketch_face_shp_ptr& f   = m_topo.faces()[i];
    std::string                lbl = (f && !f->name.empty()) ? f->name : ("F" + std::to_string(i));
    labels.push_back(std::move(lbl));
  }

  return labels;
}

/// Selectable "+" marker for user-placed permanent sketch nodes (add-node tool).
struct Sketch_AIS_node_mark : public AIS_Shape
{
  Sketch_AIS_node_mark(Sketch& owner, size_t node_idx, const TopoDS_Shape& shp);
  Sketch& owner_sketch;
  size_t  node_idx{};
};

bool try_remove_sketch_permanent_node_mark(AIS_Shape* shp)
{
  if (!shp)
    return false;

  if (auto* mark = dynamic_cast<Sketch_AIS_node_mark*>(shp))
  {
    mark->owner_sketch.remove_permanent_node_mark(*mark);
    return true;
  }

  return false;
}

void Sketch::sync_permanent_node_annos_()
{
  if (m_permanent_node_marks.size() < m_nodes.size())
    m_permanent_node_marks.resize(m_nodes.size());

  const double size_scale = std::max(static_cast<double>(m_view.gui().permanent_node_anno_scale()), 0.0);
  const double half_arm   = std::max(m_topo.plane_pick_snap_radius_world() * 0.45 * size_scale, Precision::Confusion() * 50.0);

  const Mode mode = get_mode();
  // Show "+" markers for permanent user nodes in sketch modes and polar duplicate (which snaps to sketch nodes).
  // Hide during face extrude so face selection is unobstructed.
  const bool show_permanent_marks =
      mode != Mode::Sketch_face_extrude && (is_sketch_mode(mode) || mode == Mode::Shape_polar_duplicate);
  const bool hide_for_operation_axis = operation_axis_suppresses_sketch_snap_();

  for (size_t i = 0, n = m_nodes.size(); i < n; ++i)
  {
    const Sketch_nodes::Node& node = m_nodes[i];
    const bool show = m_visible && node.permanent && !node.deleted && show_permanent_marks && !hide_for_operation_axis;

    if (!show)
    {
      if (m_permanent_node_marks[i])
      {
        m_ctx.Remove(m_permanent_node_marks[i], false);
        m_permanent_node_marks[i].Nullify();
      }
      continue;
    }

    const gp_Pnt2d     p2(node.X(), node.Y());
    const gp_Pnt       c3    = to_3d(m_pln, p2);
    const TopoDS_Shape cross = create_plus_cross_shape(m_pln, c3, half_arm);

    if (m_permanent_node_marks[i])
    {
      m_permanent_node_marks[i]->Set(cross);
      update_node_mark_style_(m_permanent_node_marks[i]);
      m_ctx.Redisplay(m_permanent_node_marks[i], true);
    }
    else
    {
      Sketch_AIS_node_mark_ptr mk = new Sketch_AIS_node_mark(*this, i, cross);
      update_node_mark_style_(mk);
      m_permanent_node_marks[i] = mk;
      m_ctx.Display(mk, AIS_WireFrame, 0, false);
    }
  }
}

void Sketch::remove_permanent_node_mark(Sketch_AIS_node_mark& mark)
{
  EZY_ASSERT(&mark.owner_sketch == this);
  const size_t i = mark.node_idx;
  EZY_ASSERT(i < m_nodes.size());
  m_nodes[i].deleted = true;
  remove_length_dimensions_referencing_node_(i);
  if (i < m_permanent_node_marks.size() && m_permanent_node_marks[i].get() == &mark)
  {
    m_ctx.Remove(m_permanent_node_marks[i], false);
    m_permanent_node_marks[i].Nullify();
  }

  m_ctx.UpdateCurrentViewer();
}

void Sketch::remove_permanent_node_mark_ais_at_(size_t node_idx)
{
  if (node_idx >= m_permanent_node_marks.size() || m_permanent_node_marks[node_idx].IsNull())
    return;

  m_ctx.Remove(m_permanent_node_marks[node_idx], false);
  m_permanent_node_marks[node_idx].Nullify();
}

void Sketch::remove_all_permanent_node_marks_()
{
  for (Sketch_AIS_node_mark_ptr& mk : m_permanent_node_marks)
    if (mk)
      m_ctx.Remove(mk, false);

  m_permanent_node_marks.clear();
}

void Sketch::erase_permanent_node_marks_()
{
  for (Sketch_AIS_node_mark_ptr& mk : m_permanent_node_marks)
    if (mk)
      m_ctx.Erase(mk, false);
}

void Sketch::trim_trailing_permanent_node_marks_()
{
  while (m_permanent_node_marks.size() > m_nodes.size())
  {
    const size_t last = m_permanent_node_marks.size() - 1;
    if (m_permanent_node_marks[last])
      m_ctx.Remove(m_permanent_node_marks[last], false);

    m_permanent_node_marks.pop_back();
  }
}

Sketch_AIS_edge::Sketch_AIS_edge(Sketch& owner, const TopoDS_Shape& shp)
    : AIS_Shape(shp)
    , owner_sketch(owner)
{
}

Sketch_AIS_node_mark::Sketch_AIS_node_mark(Sketch& owner, size_t node_idx, const TopoDS_Shape& shp)
    : AIS_Shape(shp)
    , owner_sketch(owner)
    , node_idx(node_idx)
{
}

Sketch_face_shp::Sketch_face_shp(Sketch& owner, const TopoDS_Shape& face)
    : owner_sketch(owner)
    , AIS_Shape(face)
{
}