#include "sketch_edges.h"

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <Precision.hxx>
#include <algorithm>
#include <vector>

#include "occt_view.h"
#include "sketch.h"
#include "sketch_delta.h"
#include "sketch_topo.h"
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
  rec.note_curr_linear_edge(pt_a, pt_b);
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
    if (!is_linear(e))
      continue;

    gp_Pnt2d qa = m_sketch.m_nodes[e.node_idx_a];
    gp_Pnt2d qb = m_sketch.m_nodes[e.node_idx_b];

    if (auto inter = segment_intersection_2d(pt_a, pt_b, qa, qb, Segment_inclusion::Closed))
      add_unique_point(inters, *inter);
  }

  // Collect only the *interior* hits on existing edges for splitting.
  std::vector<gp_Pnt2d> inters_to_split;
  for (const auto& ip : inters)
  {
    bool is_old_interior = false;
    for (const Sketch_edge& e : m_edges)
    {
      if (!is_linear(e))
        continue;

      gp_Pnt2d qa = m_sketch.m_nodes[e.node_idx_a];
      gp_Pnt2d qb = m_sketch.m_nodes[e.node_idx_b];
      if (point_on_open_segment_2d(ip, qa, qb))
      {
        is_old_interior = true;
        break;
      }
    }
    if (is_old_interior)
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
  }

  for (size_t i = 0; i + 1 < div_node_idxs.size(); ++i)
  {
    const gp_Pnt2d& pa = m_sketch.m_nodes[div_node_idxs[i]];
    const gp_Pnt2d& pb = m_sketch.m_nodes[div_node_idxs[i + 1]];
    add_edge_raw_(pa, pb);
  }
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

void Sketch_edges::add_arc_circle_edges(const std::vector<size_t>& node_idxs)
{
  EZY_ASSERT(node_idxs.size() == 3);
  Geom_TrimmedCurve_ptr arc_of_circle =
      GC_MakeArcOfCircle(m_sketch.to_3d_(node_idxs[0]), m_sketch.to_3d_(node_idxs[2]), m_sketch.to_3d_(node_idxs[1]));

  Sketch_AIS_edge_ptr shp = new Sketch_AIS_edge(m_sketch, BRepBuilderAPI_MakeEdge(arc_of_circle));
  m_sketch.update_edge_style_(shp);
  m_sketch.m_ctx.Display(shp, false);

  // Split into two edges for valid planer graph topology.
  m_edges.push_back({node_idxs[0], node_idxs[2], node_idxs[2], std::nullopt, arc_of_circle, shp});
  m_edges.push_back({node_idxs[2], node_idxs[1], std::nullopt, std::nullopt, arc_of_circle, shp});
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
  const Sketch_edge* last_arc_half = nullptr;

  for (const Sketch_edge& e : m_edges)
  {
    if (!e.circle_arc)
      continue;

    EZY_ASSERT(e.node_idx_b.has_value());

    if (last_arc_half)
    {
      EZY_ASSERT(last_arc_half->circle_arc.get() == e.circle_arc.get());
      EZY_ASSERT(last_arc_half->node_idx_arc.has_value());
      fn(Sketch_edge_arc{last_arc_half->node_idx_a, *last_arc_half->node_idx_arc, *e.node_idx_b, last_arc_half->shp});
      last_arc_half = nullptr;
    }
    else
    {
      EZY_ASSERT(e.node_idx_arc.has_value());
      EZY_ASSERT(*e.node_idx_b == *e.node_idx_arc);
      last_arc_half = &e;
    }
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
