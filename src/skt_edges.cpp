#include "skt_edges.h"

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <Precision.hxx>
#include <TopoDS.hxx>
#include <algorithm>
#include <cmath>
#include <optional>
#include <vector>

#include "gui_occt_view.h"
#include "skt.h"
#include "skt_op_recorder.h"
#include "skt_edge.h"
#include "skt_topo.h"
#include "utl_geom.h"
#include "utl.h"

Sketch_edges::Sketch_edges(Sketch& sketch)
    : m_sketch(sketch)
{
}

void Sketch_edges::remove_displayed()
{
  for (Sketch_edge& e : m_edges)
    m_sketch.m_ctx.Remove(e.shp, false);

  m_edges.clear();
}

void Sketch_edges::remove_by_ais(const Sketch_AIS_edge& to_remove)
{
  // Some shapes are composed with multiple edges, so we cannot break when an edge is found.
  for (std::list<Sketch_edge>::iterator itr = m_edges.begin(); itr != m_edges.end();)
    if (itr->shp.get() == &to_remove)
      itr = m_edges.erase(itr);
    else
      ++itr;
}

void Sketch_edges::add_edge_raw_(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b)
{
  Sketch_edge edge{m_sketch.m_nodes.get_node_exact(pt_a)};
  update_end_pt(edge, m_sketch.m_nodes.get_node_exact(pt_b));
  m_edges.emplace_back(edge);
  m_sketch.m_nodes.finalize();
}

void Sketch_edges::add_edge(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b) { add_edge_impl_(pt_a, pt_b, nullptr); }

void Sketch_edges::add_edge(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, Sketch_op_recorder& rec)
{
  add_edge_impl_(pt_a, pt_b, &rec);
}

void Sketch_edges::add_edge_impl_(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, Sketch_op_recorder* rec)
{
  if (!unique(pt_a, pt_b))
    return;

  // Find intersections (crossings or T-touches) between this new segment and all *currently existing* linear edges.
  // We will split the existing ones at interior intersections, and subdivide this new one as needed.
  std::vector<gp_Pnt2d> inters;
  for (const Sketch_edge& e : m_edges)
  {
    if (is_linear(e))
    {
      gp_Pnt2d qa = m_sketch.m_nodes[e.node_idx_a];
      gp_Pnt2d qb = m_sketch.m_nodes[e.node_idx_b];

      if (same_line_support_2d(pt_a, pt_b, qa, qb))
      {
        for (const gp_Pnt2d& ip : collinear_overlap_cut_points_2d(pt_a, pt_b, qa, qb))
          add_unique_point(inters, ip);
      }
      else if (auto inter = segment_intersection_2d(pt_a, pt_b, qa, qb, Segment_inclusion::Closed))
        add_unique_point(inters, *inter);
    }
    else if (sketch_edge_is_arc(e))
    {
      for (const gp_Pnt2d& ip :
           segment_arc_intersections_2d(pt_a, pt_b, TopoDS::Edge(e.shp->Shape()), m_sketch.m_pln, Segment_inclusion::Closed))
        add_unique_point(inters, ip);
    }
  }

  // Collect interior hits on existing edges for splitting.
  std::vector<gp_Pnt2d> inters_to_split;
  for (const auto& ip : inters)
  {
    bool split_here = false;
    for (const Sketch_edge& e : m_edges)
    {
      if (is_linear(e))
      {
        gp_Pnt2d qa = m_sketch.m_nodes[e.node_idx_a];
        gp_Pnt2d qb = m_sketch.m_nodes[e.node_idx_b];
        if (point_on_open_segment_2d(ip, qa, qb))
        {
          split_here = true;
          break;
        }
      }
      else if (sketch_edge_is_arc(e))
      {
        if (point_on_open_arc_interior_2d(ip, TopoDS::Edge(e.shp->Shape()), m_sketch.m_pln))
        {
          split_here = true;
          break;
        }
      }
    }
    if (split_here)
      add_unique_point(inters_to_split, ip);
  }

  std::vector<gp_Pnt2d> div_pts{pt_a, pt_b};
  div_pts.insert(div_pts.end(), inters.begin(), inters.end());

  gp_Vec2d dir(pt_a, pt_b);
  double   len2 = dir.SquareMagnitude();
  if (len2 < Precision::Confusion() * Precision::Confusion())
    return;

  std::sort(div_pts.begin(), div_pts.end(),
            [&](const gp_Pnt2d& u, const gp_Pnt2d& v) { return gp_Vec2d(pt_a, u).Dot(dir) < gp_Vec2d(pt_a, v).Dot(dir); });

  std::vector<gp_Pnt2d> pts;
  for (const auto& p : div_pts)
  {
    if (pts.empty() || pts.back().Distance(p) > Precision::Confusion())
      pts.push_back(p);
  }

  if (pts.size() < 2)
    return;

  std::vector<size_t> div_node_idxs;
  for (const auto& p : pts)
    div_node_idxs.push_back(m_sketch.m_nodes.get_node_exact(p));

  for (const auto& inter_p : inters_to_split)
  {
    const size_t nidx = m_sketch.m_nodes.get_node_exact(inter_p);
    if (rec)
      rec->note_curr_node(nidx);

    m_sketch.m_topo.split_linear_edges_at_node_if_interior(nidx, rec);
    m_sketch.m_topo.split_arcs_at_node_if_interior(nidx, rec);
  }

  bool added_any = false;
  for (size_t i = 0; i + 1 < div_node_idxs.size(); ++i)
  {
    const gp_Pnt2d& pa = m_sketch.m_nodes[div_node_idxs[i]];
    const gp_Pnt2d& pb = m_sketch.m_nodes[div_node_idxs[i + 1]];
    if (has_linear_edge_(pa, pb))
      continue;

    add_edge_raw_(pa, pb);
    added_any = true;
  }

  if (rec && (added_any || !inters_to_split.empty()))
    rec->note_curr_linear_edge(pt_a, pt_b);
}

void Sketch_edges::sketch_json_add_linear_edge(size_t idx_a, size_t idx_b, std::optional<size_t> idx_mid)
{
  EZY_ASSERT(idx_a < m_sketch.m_nodes.size() && idx_b < m_sketch.m_nodes.size());
  if (idx_mid.has_value())
    EZY_ASSERT(*idx_mid < m_sketch.m_nodes.size());

  Sketch_edge edge{idx_a};
  edge.node_idx_b      = idx_b;
  edge.node_idx_mid    = idx_mid;
  const gp_Pnt2d& pt_a = m_sketch.m_nodes[idx_a];
  const gp_Pnt2d& pt_b = m_sketch.m_nodes[idx_b];
  m_sketch.update_edge_shp_(edge, pt_a, pt_b);
  m_edges.emplace_back(edge);
  m_sketch.m_nodes.finalize();
}

void Sketch_edges::update_end_pt(Sketch_edge& edge, size_t end_pt_idx)
{
  EZY_ASSERT(end_pt_idx < m_sketch.m_nodes.size());
  edge.node_idx_b      = end_pt_idx;
  const gp_Pnt2d& pt_a = m_sketch.m_nodes[edge.node_idx_a];
  const gp_Pnt2d& pt_b = m_sketch.m_nodes[end_pt_idx];
  m_sketch.update_edge_shp_(edge, pt_a, pt_b);
  if (Sketch::get_add_mid_pt_edges())
    edge.node_idx_mid = m_sketch.m_nodes.add_new_node(get_midpoint(pt_a, pt_b), true);
  else
    edge.node_idx_mid = std::nullopt;
}

bool Sketch_edges::add_arc_circle_edges(const std::vector<size_t>& node_idxs, Sketch_op_recorder* rec)
{
  EZY_ASSERT(node_idxs.size() == 3);
  // node_idxs[2] is the user pick (may be toward the circle center); it defines the arc only.
  Geom_TrimmedCurve_ptr arc_of_circle =
      GC_MakeArcOfCircle(m_sketch.to_3d_(node_idxs[0]), m_sketch.to_3d_(node_idxs[2]), m_sketch.to_3d_(node_idxs[1]));
  if (!arc_of_circle)
    return false;

  const TopoDS_Edge new_edge = BRepBuilderAPI_MakeEdge(arc_of_circle).Edge();
  const gp_Pnt2d    pt_start = m_sketch.m_nodes[node_idxs[0]];
  const gp_Pnt2d    pt_end   = m_sketch.m_nodes[node_idxs[1]];

  // Intersections with all existing edges (split both olds and the new arc).
  std::vector<gp_Pnt2d> inters;
  for (const Sketch_edge& e : m_edges)
  {
    if (is_linear(e))
    {
      const gp_Pnt2d qa = m_sketch.m_nodes[e.node_idx_a];
      const gp_Pnt2d qb = m_sketch.m_nodes[*e.node_idx_b];
      if (point_on_open_segment_2d(pt_start, qa, qb))
        add_unique_point(inters, pt_start);

      if (point_on_open_segment_2d(pt_end, qa, qb))
        add_unique_point(inters, pt_end);

      for (const gp_Pnt2d& ip : segment_arc_intersections_2d(qa, qb, new_edge, m_sketch.m_pln, Segment_inclusion::Closed))
        add_unique_point(inters, ip);
    }
    else if (sketch_edge_is_arc(e))
    {
      const TopoDS_Edge old_arc = TopoDS::Edge(e.shp->Shape());
      if (same_circle_support_2d(old_arc, new_edge, m_sketch.m_pln))
      {
        const gp_Pnt2d ea = m_sketch.m_nodes[e.node_idx_a];
        const gp_Pnt2d eb = m_sketch.m_nodes[*e.node_idx_b];
        if (point_on_open_arc_interior_2d(ea, new_edge, m_sketch.m_pln))
          add_unique_point(inters, ea);

        if (point_on_open_arc_interior_2d(eb, new_edge, m_sketch.m_pln))
          add_unique_point(inters, eb);

        if (point_on_open_arc_interior_2d(pt_start, old_arc, m_sketch.m_pln))
          add_unique_point(inters, pt_start);

        if (point_on_open_arc_interior_2d(pt_end, old_arc, m_sketch.m_pln))
          add_unique_point(inters, pt_end);
      }
      else
      {
        for (const gp_Pnt2d& ip : arc_arc_intersections_2d(old_arc, new_edge, m_sketch.m_pln))
          add_unique_point(inters, ip);
      }
    }
  }

  std::vector<gp_Pnt2d> inters_to_split;
  for (const gp_Pnt2d& ip : inters)
  {
    bool split_here = false;
    for (const Sketch_edge& e : m_edges)
      if (is_linear(e))
      {
        const gp_Pnt2d qa = m_sketch.m_nodes[e.node_idx_a];
        const gp_Pnt2d qb = m_sketch.m_nodes[*e.node_idx_b];
        if (point_on_open_segment_2d(ip, qa, qb))
        {
          split_here = true;
          break;
        }
      }
      else if (sketch_edge_is_arc(e))
      {
        if (point_on_open_arc_interior_2d(ip, TopoDS::Edge(e.shp->Shape()), m_sketch.m_pln))
        {
          split_here = true;
          break;
        }
      }

    if (split_here)
      add_unique_point(inters_to_split, ip);
  }

  for (const gp_Pnt2d& inter_p : inters_to_split)
  {
    const size_t nidx = m_sketch.m_nodes.get_node_exact(inter_p);
    if (rec)
      rec->note_curr_node(nidx);

    m_sketch.m_topo.split_linear_edges_at_node_if_interior(nidx, rec);
    m_sketch.m_topo.split_arcs_at_node_if_interior(nidx, rec);
  }

  const BRepAdaptor_Curve curve(new_edge);
  const Geom_Curve_ptr    geom    = curve.Curve().Curve();
  const double            u_first = curve.FirstParameter();
  const double            u_last  = curve.LastParameter();

  auto param_of = [&](const gp_Pnt2d& p) -> std::optional<double>
  {
    GeomAPI_ProjectPointOnCurve proj(to_3d(m_sketch.m_pln, p), geom, u_first, u_last);
    if (proj.NbPoints() == 0 || proj.LowerDistance() > Precision::Confusion())
      return std::nullopt;

    return proj.LowerDistanceParameter();
  };

  std::vector<double> cuts{u_first, u_last};
  for (const gp_Pnt2d& ip : inters)
  {
    if (const std::optional<double> u = param_of(ip))
      cuts.push_back(*u);
  }

  std::sort(cuts.begin(), cuts.end());
  const double span   = u_last - u_first;
  const double margin = std::max(Precision::Confusion(), std::abs(span) * 1e-9);
  std::vector<double> unique_cuts;
  for (double u : cuts)
  {
    if (unique_cuts.empty() || std::abs(u - unique_cuts.back()) > margin)
      unique_cuts.push_back(u);
  }

  bool added_any = false;
  if (unique_cuts.size() == 2)
  {
    const gp_Pnt2d bulge = m_sketch.m_nodes[node_idxs[2]];
    if (!has_equivalent_arc_(pt_start, pt_end, bulge))
    {
      add_arc_raw_(node_idxs[0], node_idxs[1], node_idxs[2]);
      added_any = true;
    }
  }
  else
  {
    for (size_t i = 0; i + 1 < unique_cuts.size(); ++i)
    {
      if (unique_cuts[i + 1] - unique_cuts[i] <= margin)
        continue;

      const gp_Pnt2d p0    = to_2d(m_sketch.m_pln, curve.Value(unique_cuts[i]));
      const gp_Pnt2d p1    = to_2d(m_sketch.m_pln, curve.Value(unique_cuts[i + 1]));
      const gp_Pnt2d bulge = to_2d(m_sketch.m_pln, curve.Value(0.5 * (unique_cuts[i] + unique_cuts[i + 1])));
      if (has_equivalent_arc_(p0, p1, bulge))
        continue;

      add_arc_raw_(m_sketch.m_nodes.get_node_exact(p0), m_sketch.m_nodes.get_node_exact(p1),
                   m_sketch.m_nodes.get_node_exact(bulge));
      added_any = true;
    }
  }

  return added_any || !inters_to_split.empty();
}

void Sketch_edges::split_arc_at_node_(std::list<Sketch_edge>::iterator itr, size_t split_idx, Sketch_op_recorder* rec)
{
  EZY_ASSERT(sketch_edge_is_arc(*itr));
  EZY_ASSERT(itr->node_idx_b.has_value());

  const size_t idx_a = itr->node_idx_a;
  const size_t idx_b = *itr->node_idx_b;
  EZY_ASSERT(split_idx != idx_a && split_idx != idx_b);

  const TopoDS_Edge occ_edge = TopoDS::Edge(itr->shp->Shape());
  if (rec)
  {
    const gp_Pnt2d arc_pt = itr->node_idx_arc_pt.has_value() ? m_sketch.m_nodes[*itr->node_idx_arc_pt]
                                                             : arc_curve_midpoint_2d(occ_edge, m_sketch.m_pln);
    rec->note_prev_arc_edge(m_sketch.m_nodes[idx_a], arc_pt, m_sketch.m_nodes[idx_b]);
  }

  const BRepAdaptor_Curve curve(occ_edge);
  const Geom_Curve_ptr    geom    = curve.Curve().Curve();
  const double            u_first = curve.FirstParameter();
  const double            u_last  = curve.LastParameter();

  auto param_at = [&](size_t node_idx) -> double
  {
    GeomAPI_ProjectPointOnCurve proj(to_3d(m_sketch.m_pln, m_sketch.m_nodes[node_idx]), geom, u_first, u_last);
    EZY_ASSERT(proj.NbPoints() > 0);
    return proj.LowerDistanceParameter();
  };

  const double u_a     = param_at(idx_a);
  const double u_split = param_at(split_idx);
  const double u_b     = param_at(idx_b);

  const gp_Pnt2d bulge1 = to_2d(m_sketch.m_pln, curve.Value((u_a + u_split) * 0.5));
  const gp_Pnt2d bulge2 = to_2d(m_sketch.m_pln, curve.Value((u_split + u_b) * 0.5));

  m_sketch.m_ctx.Remove(itr->shp, false);
  m_sketch.m_edges.edges().erase(itr);

  const size_t bulge1_idx = m_sketch.m_nodes.get_node_exact(bulge1);
  const size_t bulge2_idx = m_sketch.m_nodes.get_node_exact(bulge2);

  add_arc_raw_(idx_a, split_idx, bulge1_idx);
  add_arc_raw_(split_idx, idx_b, bulge2_idx);

  if (rec)
  {
    rec->note_curr_node(split_idx);
    rec->note_curr_arc_edge(m_sketch.m_nodes[idx_a], m_sketch.m_nodes[bulge1_idx], m_sketch.m_nodes[split_idx]);
    rec->note_curr_arc_edge(m_sketch.m_nodes[split_idx], m_sketch.m_nodes[bulge2_idx], m_sketch.m_nodes[idx_b]);
  }
}

void Sketch_edges::add_arc_raw_(size_t idx_a, size_t idx_b, size_t bulge_idx)
{
  Geom_TrimmedCurve_ptr arc =
      GC_MakeArcOfCircle(m_sketch.to_3d_(idx_a), m_sketch.to_3d_(bulge_idx), m_sketch.to_3d_(idx_b));
  if (!arc)
    return;

  const TopoDS_Edge occ = BRepBuilderAPI_MakeEdge(arc).Edge();
  std::optional<size_t> arc_pt_idx;
  if (Sketch::get_add_mid_pt_edges())
    arc_pt_idx = m_sketch.m_nodes.add_new_node(arc_curve_midpoint_2d(occ, m_sketch.m_pln), true);

  Sketch_AIS_edge_ptr shp = new Sketch_AIS_edge(m_sketch, occ);
  m_sketch.update_edge_style_(shp);
  m_sketch.m_ctx.Display(shp, false);
  m_edges.push_back({idx_a, idx_b, arc_pt_idx, std::nullopt, shp});
}

bool Sketch_edges::has_linear_edge_(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b) const
{
  const double tol = Precision::Confusion();
  for (const Sketch_edge& e : m_edges)
  {
    if (!is_linear(e) || !e.node_idx_b.has_value())
      continue;

    const gp_Pnt2d ea = m_sketch.m_nodes[e.node_idx_a];
    const gp_Pnt2d eb = m_sketch.m_nodes[*e.node_idx_b];
    if ((ea.Distance(pt_a) <= tol && eb.Distance(pt_b) <= tol) || (ea.Distance(pt_b) <= tol && eb.Distance(pt_a) <= tol))
      return true;
  }

  return false;
}

bool Sketch_edges::has_equivalent_arc_(const gp_Pnt2d& start, const gp_Pnt2d& end, const gp_Pnt2d& bulge) const
{
  const double tol = Precision::Confusion();
  for (const Sketch_edge& e : m_edges)
  {
    if (!sketch_edge_is_arc(e) || !e.node_idx_b.has_value() || e.shp.IsNull())
      continue;

    const gp_Pnt2d ea = m_sketch.m_nodes[e.node_idx_a];
    const gp_Pnt2d eb = m_sketch.m_nodes[*e.node_idx_b];
    const bool     ends_fwd = ea.Distance(start) <= tol && eb.Distance(end) <= tol;
    const bool     ends_rev = ea.Distance(end) <= tol && eb.Distance(start) <= tol;
    if (!ends_fwd && !ends_rev)
      continue;

    const TopoDS_Edge occ = TopoDS::Edge(e.shp->Shape());
    if (point_on_open_arc_interior_2d(bulge, occ, m_sketch.m_pln))
      return true;

    if (arc_curve_midpoint_2d(occ, m_sketch.m_pln).Distance(bulge) <= tol)
      return true;
  }

  return false;
}

void Sketch_edges::for_each_linear(const Linear_visitor& fn) const
{
  for (const Sketch_edge& e : m_edges)
  {
    if (!is_linear(e))
      continue;

    EZY_ASSERT(e.node_idx_b.has_value());
    fn(Sketch_edge_linear{e.node_idx_a, *e.node_idx_b, e.node_idx_mid});
  }
}

void Sketch_edges::for_each_arc(const Arc_visitor& fn) const
{
  for (const Sketch_edge& e : m_edges)
  {
    if (!sketch_edge_is_arc(e))
      continue;

    EZY_ASSERT(e.node_idx_b.has_value());
    EZY_ASSERT(e.node_idx_arc_pt.has_value());
    fn(Sketch_edge_arc{e.node_idx_a, *e.node_idx_arc_pt, *e.node_idx_b, e.shp});
  }
}

std::vector<Sketch_edge> Sketch_edges::get_selected() const
{
  std::vector<Sketch_edge> ret;
  for (const AIS_Shape_ptr& selected : m_sketch.m_view.get_selected())
    for (const Sketch_edge& e : m_edges)
      if (e.shp == selected)
        ret.push_back(e);

  return ret;
}

std::list<Sketch_edge>::iterator Sketch_edges::get_at(const ScreenCoords& screen_coords)
{
  AIS_Shape_ptr shp = m_sketch.m_view.get_shape(screen_coords);
  if (auto edge = dynamic_cast<Sketch_AIS_edge*>(shp.get()); edge)
    if (&edge->owner_sketch == &m_sketch)
      for (std::list<Sketch_edge>::iterator itr = m_edges.begin(); itr != m_edges.end(); ++itr)
        if (itr->shp.get() == edge)
          return itr;

  return m_edges.end();
}
