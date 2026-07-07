#include "sketch_topo.h"

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepTools.hxx>
#include <Graphic3d_AspectFillArea3d.hxx>
#include <Precision.hxx>
#include <Quantity_Color.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>
#include <algorithm>
#include <functional>
#include <limits>
#include <map>
#include <unordered_map>
#include <unordered_set>

#include <glm/glm.hpp>

#include "gui_occt_view.h"
#include "sketch.h"
#include "sketch_delta.h"
#include "sketch_edge.h"
#include "utl.h"
#include "utl_geom.h"

using namespace glm;

struct Sketch_topo::Face_edge
{
  const Sketch::Edge& edge;
  bool                reversed;

  size_t start_nd_idx() const;
  size_t end_nd_idx() const;
};

Sketch_topo::Sketch_topo(Sketch& sketch)
    : m_sketch(sketch)
{
}

void Sketch_topo::remove_displayed_faces()
{
  for (AIS_Shape_ptr& f : m_faces)
    m_sketch.m_ctx.Remove(f, false);

  m_faces.clear();
  m_dim_classifier_faces.clear();
}

double Sketch_topo::plane_pick_snap_radius_world() const
{
  const double px = Sketch_nodes::get_snap_dist();
  if (m_sketch.m_view.is_headless())
    return std::max(px, Precision::Confusion() * 1e9);

  const gp_Pnt2d          ref2(0.0, 0.0);
  const ScreenCoords      s0   = m_sketch.m_view.get_screen_coords(to_3d(m_sketch.m_pln, ref2));
  const dvec2             base = s0.unsafe_get();
  std::optional<gp_Pnt2d> p1   = m_sketch.m_view.pt_on_plane(ScreenCoords(dvec2(base.x + px, base.y)), m_sketch.m_pln);
  if (!p1)
    return std::max(px, Precision::Confusion() * 1e9);

  return ref2.Distance(*p1);
}

void Sketch_topo::snap_placed_node_to_closest_linear_edge_interior_(size_t node_idx)
{
  EZY_ASSERT(node_idx < m_sketch.m_nodes.size());

  const gp_Pnt2d p(m_sketch.m_nodes[node_idx].X(), m_sketch.m_nodes[node_idx].Y());
  const double   max_d = plane_pick_snap_radius_world();

  double                  best_err = std::numeric_limits<double>::infinity();
  std::optional<gp_Pnt2d> best_proj;

  for (const auto& e : m_sketch.m_edges.edges())
  {
    if (!sketch_edge_is_linear(e))
      continue;

    if (!e.node_idx_b.has_value())
      continue;

    const size_t    a  = e.node_idx_a;
    const size_t    b  = *e.node_idx_b;
    const gp_Pnt2d& pa = m_sketch.m_nodes[a];
    const gp_Pnt2d& pb = m_sketch.m_nodes[b];
    if (node_idx == a || node_idx == b)
      continue;

    std::optional<gp_Pnt2d> proj = snap_foot_to_open_segment_interior_if_close(p, pa, pb, max_d);
    if (!proj)
      continue;

    const double err = p.Distance(*proj);
    if (err < best_err)
    {
      best_err  = err;
      best_proj = proj;
    }
  }

  if (best_proj)
  {
    Sketch_nodes::Node& n = m_sketch.m_nodes[node_idx];
    n.SetX(best_proj->X());
    n.SetY(best_proj->Y());
    n.midpoint = false;
  }
}

void Sketch_topo::split_linear_edges_at_node_if_interior(size_t node_idx)
{
  split_linear_edges_at_node_if_interior(node_idx, static_cast<Sketch_op_recorder*>(nullptr));
}

void Sketch_topo::split_linear_edges_at_node_if_interior(size_t node_idx, Sketch_op_recorder& rec)
{
  split_linear_edges_at_node_if_interior(node_idx, &rec);
}

void Sketch_topo::split_linear_edges_at_node_if_interior(size_t node_idx, Sketch_op_recorder* rec)
{
  EZY_ASSERT(node_idx < m_sketch.m_nodes.size());
  snap_placed_node_to_closest_linear_edge_interior_(node_idx);
  const gp_Pnt2d& p = m_sketch.m_nodes[node_idx];

  bool progress = true;
  while (progress)
  {
    progress = false;
    for (auto itr = m_sketch.m_edges.edges().begin(); itr != m_sketch.m_edges.edges().end(); ++itr)
    {
      if (!sketch_edge_is_linear(*itr))
        continue;

      if (!itr->node_idx_b.has_value())
        continue;

      const size_t    a  = itr->node_idx_a;
      const size_t    b  = *itr->node_idx_b;
      const gp_Pnt2d& pa = m_sketch.m_nodes[a];
      const gp_Pnt2d& pb = m_sketch.m_nodes[b];

      if (node_idx == a || node_idx == b)
        continue;

      if (!point_on_open_segment_2d(p, pa, pb))
        continue;

      Sketch::Edge edge_a{a};
      m_sketch.update_edge_end_pt_(edge_a, node_idx);
      Sketch::Edge edge_b{node_idx};
      m_sketch.update_edge_end_pt_(edge_b, b);

      if (rec)
        rec->note_prev_linear_edge(itr->node_idx_a, *itr->node_idx_b, itr->node_idx_mid, itr->name);

      m_sketch.m_ctx.Remove(itr->shp, false);
      m_sketch.m_edges.edges().erase(itr);
      m_sketch.m_nodes[node_idx].midpoint = false;
      m_sketch.m_edges.edges().push_back(std::move(edge_a));
      m_sketch.m_edges.edges().push_back(std::move(edge_b));

      progress = true;
      break;
    }
  }
}

void Sketch_topo::split_arcs_at_node_if_interior(size_t node_idx)
{
  split_arcs_at_node_if_interior(node_idx, static_cast<Sketch_op_recorder*>(nullptr));
}

void Sketch_topo::split_arcs_at_node_if_interior(size_t node_idx, Sketch_op_recorder& rec)
{
  split_arcs_at_node_if_interior(node_idx, &rec);
}

void Sketch_topo::split_arcs_at_node_if_interior(size_t node_idx, Sketch_op_recorder* rec)
{
  EZY_ASSERT(node_idx < m_sketch.m_nodes.size());
  const gp_Pnt2d& p = m_sketch.m_nodes[node_idx];

  bool progress = true;
  while (progress)
  {
    progress = false;
    for (auto itr = m_sketch.m_edges.edges().begin(); itr != m_sketch.m_edges.edges().end(); ++itr)
    {
      if (!sketch_edge_is_arc(*itr))
        continue;

      EZY_ASSERT(itr->node_idx_b.has_value());
      const size_t idx_a = itr->node_idx_a;
      const size_t idx_b = *itr->node_idx_b;
      if (node_idx == idx_a || node_idx == idx_b)
        continue;

      if (!point_on_open_arc_interior_2d(p, TopoDS::Edge(itr->shp->Shape()), m_sketch.m_pln))
        continue;

      m_sketch.m_edges.split_arc_at_node_(itr, node_idx, rec);
      m_sketch.m_nodes[node_idx].midpoint = false;
      progress                            = true;
      break;
    }
  }
}

// Function to extract faces from the planar graph
void Sketch_topo::update_faces()
{
  m_sketch.m_nodes.finalize();

  m_sketch.m_view.remove(m_faces);
  m_faces.clear();
  m_dim_classifier_faces.clear();

  // Used to cleanup dangling nodes;
  std::vector<bool> used_nodes(m_sketch.m_nodes.size());

  // Build adjacency list
  std::unordered_map<size_t, std::vector<std::pair<size_t, const Sketch::Edge*>>> adj_list;

  for (const auto& edge : m_sketch.m_edges.edges())
  {
    EZY_ASSERT(edge.node_idx_b.has_value());

    size_t a = edge.node_idx_a;
    size_t b = edge.node_idx_b.value();
    adj_list[a].push_back({b, &edge});
    adj_list[b].push_back({a, &edge});

    // Keep track of used nodes.
    used_nodes[a] = true;
    used_nodes[b] = true;
    if (edge.node_idx_mid.has_value())
      used_nodes[*edge.node_idx_mid] = true;

    if (edge.node_idx_arc_pt.has_value())
      used_nodes[*edge.node_idx_arc_pt] = true;
  }

  if (m_sketch.m_operation_axis.has_value())
  {
    used_nodes[m_sketch.m_operation_axis->node_idx_a] = true;
    if (m_sketch.m_operation_axis->node_idx_b.has_value())
      used_nodes[*m_sketch.m_operation_axis->node_idx_b] = true;
  }

  // Remove dangling edges (edges with degree-1 endpoints) iteratively.
  // These edges cannot form closed faces, so we exclude them from face detection.
  std::unordered_set<const Sketch::Edge*> excluded_edges;

  bool changed = true;
  while (changed)
  {
    changed = false;
    std::unordered_set<const Sketch::Edge*> to_exclude;

    // Find edges where at least one endpoint has degree 1 (considering only non-excluded edges)
    for (const auto& edge : m_sketch.m_edges.edges())
    {
      if (excluded_edges.count(&edge))
        continue;

      EZY_ASSERT(edge.node_idx_b.has_value());
      size_t a = edge.node_idx_a;
      size_t b = edge.node_idx_b.value();

      // Count degree for each endpoint (excluding already excluded edges)
      int degree_a = 0;
      int degree_b = 0;
      for (const auto& [neighbor, e] : adj_list[a])
        if (!excluded_edges.count(e))
          ++degree_a;

      for (const auto& [neighbor, e] : adj_list[b])
        if (!excluded_edges.count(e))
          ++degree_b;

      // If either endpoint has degree 1, this edge is dangling
      if (degree_a == 1 || degree_b == 1)
      {
        to_exclude.insert(&edge);
        changed = true;
      }
    }

    excluded_edges.insert(to_exclude.begin(), to_exclude.end());
  }

  // Bridge edge removal logic (active). Excludes edges that purely connect separate cycles
  // (both ends degree >=3 and no alternate path) from the adjacency used by the face walker.
  // This is required for sketches like bridge.ezy that contain a "bridge" touching an inner
  // triangular loop so that we obtain exactly two faces, one of which has the triangle as a
  // hole. Dangling removal alone is not sufficient. The logic was previously disabled because
  // an earlier version produced bad results on some nested rect holes; current state + other
  // fixes (cw break, split rules, etc.) are being validated with the new regression test.
  // Remove bridge edges that connect two separate cycles.
  // A bridge edge connects two cycles and has both endpoints with degree >= 3.
  // We detect this by checking if the edge is the only connection between two cycles.
  std::unordered_set<const Sketch::Edge*> bridge_edges;
  for (const auto& edge : m_sketch.m_edges.edges())
  {
    if (excluded_edges.count(&edge))
      continue;

    EZY_ASSERT(edge.node_idx_b.has_value());
    size_t a = edge.node_idx_a;
    size_t b = edge.node_idx_b.value();

    // Count degree for each endpoint (excluding already excluded edges)
    int degree_a = 0;
    int degree_b = 0;
    for (const auto& [neighbor, e] : adj_list[a])
      if (!excluded_edges.count(e))
        ++degree_a;

    for (const auto& [neighbor, e] : adj_list[b])
      if (!excluded_edges.count(e))
        ++degree_b;

    // Bridge edges have both endpoints with degree >= 3
    // They connect two separate cycles. We detect this by checking if removing the edge
    // creates two disconnected components, each containing a cycle.
    if (degree_a >= 3 && degree_b >= 3)
    {
      // Check if removing this edge disconnects the graph
      // by seeing if we can reach 'b' from 'a' without using this edge
      std::unordered_set<size_t> visited;
      std::vector<size_t>        queue;
      queue.push_back(a);
      visited.insert(a);
      bool can_reach_b = false;

      for (size_t i = 0; i < queue.size() && !can_reach_b; ++i)
      {
        size_t curr = queue[i];
        for (const auto& [neighbor, e] : adj_list[curr])
        {
          if (excluded_edges.count(e) || e == &edge)
            continue;

          if (neighbor == b)
          {
            can_reach_b = true;
            break;
          }

          if (!visited.count(neighbor))
          {
            visited.insert(neighbor);
            queue.push_back(neighbor);
          }
        }
      }

      // If we can't reach b from a without this edge, it's a bridge
      // But we also need to verify both sides have cycles
      if (!can_reach_b)
      {
        // Check if component containing 'a' has a cycle
        auto has_cycle_in_component = [&](size_t start, const Sketch::Edge* exclude_edge) -> bool
        {
          std::unordered_set<size_t>          comp_visited;
          std::function<bool(size_t, size_t)> dfs = [&](size_t curr, size_t parent) -> bool
          {
            comp_visited.insert(curr);
            for (const auto& [neighbor, e] : adj_list[curr])
            {
              if (excluded_edges.count(e) || e == exclude_edge)
                continue;

              if (neighbor == parent)
                continue;

              if (comp_visited.count(neighbor))
                // Found a back edge = cycle
                return true;

              if (dfs(neighbor, curr))
                return true;
            }
            return false;
          };

          // Try starting from each neighbor
          for (const auto& [neighbor, e] : adj_list[start])
          {
            if (excluded_edges.count(e) || e == exclude_edge)
              continue;

            comp_visited.clear();
            comp_visited.insert(start);
            if (dfs(neighbor, start))
              return true;
          }

          return false;
        };

        bool a_has_cycle = has_cycle_in_component(a, &edge);
        bool b_has_cycle = has_cycle_in_component(b, &edge);

        // If both components have cycles, this edge is a bridge
        if (a_has_cycle && b_has_cycle)
          bridge_edges.insert(&edge);
      }
    }
  }

  // Add bridge edges to excluded set
  excluded_edges.insert(bridge_edges.begin(), bridge_edges.end());

  // Rebuild adjacency list excluding dangling and bridge edges
  adj_list.clear();
  for (const auto& edge : m_sketch.m_edges.edges())
  {
    if (excluded_edges.count(&edge))
      continue;

    EZY_ASSERT(edge.node_idx_b.has_value());
    size_t a = edge.node_idx_a;
    size_t b = edge.node_idx_b.value();
    adj_list[a].push_back({b, &edge});
    adj_list[b].push_back({a, &edge});
  }

  // Extract faces by walking the planar graph one half-edge at a time.
  //
  // Every undirected edge has two directed "half-edges": a->b and b->a. Under a
  // consistent leftmost-turn rule, each half-edge borders exactly one face - the
  // region immediately to its left. So we seed a walk from every half-edge and, by
  // marking each traversed half-edge as visited, we discover every face exactly
  // once. Cycles wound counter-clockwise (positive signed area) bound the real
  // regions; the single clockwise cycle is the outer, unbounded region and is
  // discarded (it is still marked so it is never walked again).
  //
  // Keying the visited set on the half-edge (edge pointer + direction) - rather
  // than on the endpoint node pair - is what lets fully enclosed faces be found: a
  // face whose every boundary edge is shared with an already-discovered neighbor
  // still owns its own, opposite-direction half-edges.
  struct Half_edge
  {
    const Sketch::Edge* edge;
    bool                reversed;
    bool                operator==(const Half_edge& o) const { return edge == o.edge && reversed == o.reversed; }
  };

  struct Half_edge_hash
  {
    size_t operator()(const Half_edge& h) const noexcept
    {
      return std::hash<const void*>{}(h.edge) ^ (static_cast<size_t>(h.reversed) << 1);
    }
  };

  std::unordered_set<Half_edge, Half_edge_hash> visited;

  // A face walk consumes a distinct half-edge each step, so a valid walk cannot
  // exceed the total number of half-edges. The cap is a safety net against
  // degenerate geometry (e.g. parallel edges with identical tangents) that could
  // otherwise produce a non-terminating successor cycle; such partial walks are
  // discarded.
  const size_t max_walk_steps = 2 * m_sketch.m_edges.size() + 2;

  for (auto& [a_idx, edges] : adj_list)
    for (auto& [b_idx, start_edge] : edges)
    {
      const Half_edge seed{start_edge, start_edge->reversed(a_idx, b_idx)};
      if (visited.count(seed))
        continue;

      size_t              prev_idx  = a_idx;
      size_t              curr_idx  = b_idx;
      const Sketch::Edge* curr_edge = start_edge;
      Face_edges          face;
      bool                closed = false;

      for (size_t step = 0; step < max_walk_steps; ++step)
      {
        const bool reversed = curr_edge->reversed(prev_idx, curr_idx);
        face.push_back({*curr_edge, reversed});
        visited.insert({curr_edge, reversed});

        // Choose the next edge around curr_idx: the one turning as far left as
        // possible relative to the direction we arrived from (smallest signed
        // angle). This keeps the face interior on our left for the whole walk.
        const gp_Vec2d incoming_dir =
            sketch_edge_outgoing_dir_2d(*curr_edge, m_sketch.m_nodes[prev_idx], m_sketch.m_nodes[curr_idx], m_sketch.m_pln);

        double              min_angle = std::numeric_limits<double>::max();
        size_t              next_idx  = 0;
        const Sketch::Edge* next_edge = nullptr;
        for (auto& [cand_idx, cand_edge] : adj_list[curr_idx])
        {
          // Never immediately backtrack along the edge we just traversed, but do
          // allow other parallel edges between the same node pair.
          if (cand_idx == prev_idx && cand_edge == curr_edge)
            continue;

          const gp_Vec2d outgoing_dir =
              sketch_edge_outgoing_dir_2d(*cand_edge, m_sketch.m_nodes[curr_idx], m_sketch.m_nodes[cand_idx], m_sketch.m_pln);
          const double angle = std::atan2(outgoing_dir.Crossed(incoming_dir), outgoing_dir.Dot(incoming_dir));
          if (angle < min_angle)
          {
            min_angle = angle;
            next_idx  = cand_idx;
            next_edge = cand_edge;
          }
        }

        if (!next_edge)
          // Dead end: only possible with invalid/dangling topology.
          break;

        prev_idx  = curr_idx;
        curr_idx  = next_idx;
        curr_edge = next_edge;

        // The face closes when the walk is about to re-traverse the seed
        // half-edge (the standard face-cycle termination condition).
        if (Half_edge{curr_edge, curr_edge->reversed(prev_idx, curr_idx)} == seed)
        {
          closed = true;
          break;
        }
      }

      // Keep only closed cycles that bound a region (CCW / positive area). The CW
      // outer cycle and any discarded partial walk are skipped; their half-edges
      // stay marked so they are not walked again.
      if (!closed || face.size() < 2 || !is_face_ccw_(face))
        continue;

      Sketch_face_shp_ptr f = create_face_shape_(face);
      f->SetColor(Quantity_NOC_GRAY7);       // Base color
      f->SetTransparency(0.5);               // 0.5 = 50% transparent (0.0 = opaque, 1.0 = invisible)
      f->SetMaterial(Graphic3d_NOM_PLASTIC); // Optional: material for shading

      m_sketch.m_ctx.Display(f, AIS_Shaded, AIS_Shape::SelectionMode(TopAbs_FACE), true);
      m_sketch.m_ctx.Activate(f, AIS_Shape::SelectionMode(TopAbs_FACE));
      m_faces.push_back(f);
    }

  // Mark unused nodes as deleted so they're excluded from snapping operations.
  // Nodes become unused when all edges that reference them are removed.
  // Permanent add-node points are never auto-tombstoned when unused; user delete sets `deleted` and it stays.
  for (size_t idx = 0, num = m_sketch.m_nodes.size(); idx < num; ++idx)
    if (!m_sketch.m_nodes[idx].permanent && !m_sketch.m_nodes[idx].origin)
      m_sketch.m_nodes[idx].deleted = !used_nodes[idx];

  // Book keeping
  struct Face_meta
  {
    Sketch_face_shp_ptr           shp;
    double                        area;
    int                           parent_idx;
    std::vector<const Face_meta*> holes;
  };

  std::vector<Face_meta> face_metas;
  face_metas.reserve(m_faces.size());
  for (Sketch_face_shp_ptr& face : m_faces)
    face_metas.push_back({face, compute_face_area(face), -1});

  // Sort faces by area (descending) to check larger faces first
  std::sort(face_metas.begin(), face_metas.end(),
            [](const Face_meta& a, const Face_meta& b)
            {
              return a.area > b.area; // Descending by area
            });

  // Classify faces
  for (size_t face_i = 0, num = face_metas.size(); face_i < num; ++face_i)
  {
    Face_meta& face_a = face_metas[face_i];
    for (size_t face_j = face_i + 1; face_j < num; ++face_j)
    {
      Face_meta& face_b = face_metas[face_j];
      if (is_face_contained(face_b.shp->Shape(), face_a.shp->Shape()))
        // Check if face_a is better (smaller) parent than the current one
        if (face_b.parent_idx == -1 || face_a.area < face_metas[face_a.parent_idx].area)
          face_b.parent_idx = static_cast<int>(face_i);
    }
  }

  // Assign holes
  for (const Face_meta& face : face_metas)
    if (face.parent_idx != -1)
      face_metas[face.parent_idx].holes.push_back(&face);

  // Rebuild face shapes with their holes
  for (Face_meta& face : face_metas)
    if (face.holes.size())
    {
      EZY_ASSERT(face.shp->Shape().ShapeType() == TopAbs_FACE);
      BRepBuilderAPI_MakeFace face_maker(TopoDS::Face(face.shp->Shape()));
      for (const Face_meta* hole : face.holes)
      {
        EZY_ASSERT(hole->shp->Shape().ShapeType() == TopAbs_FACE);
        TopoDS_Wire hole_wire = BRepTools::OuterWire(TopoDS::Face(hole->shp->Shape()));
        // Winding order is important
        hole_wire.Reverse();
        face_maker.Add(hole_wire);
      }
      EZY_ASSERT(face_maker.IsDone());
      // auto dbg_face = to_boost(face_maker.Face(), m_pln);
      face.shp->SetShape(face_maker.Face());
    }

  // Use the nesting depth of the faces to define the face selection sensitivity
  for (const Face_meta& face : face_metas)
  {
    int              nesting_depth = 0;
    const Face_meta* curr_face     = &face;
    for (; curr_face->parent_idx != -1; ++nesting_depth)
      curr_face = &face_metas[curr_face->parent_idx];

    m_sketch.m_ctx.SetSelectionSensitivity(face.shp, 0, nesting_depth + 1);
  }

  rebuild_dim_classifier_face_cache_();
  m_sketch.m_dims.purge_stale_length_dimensions();
  m_sketch.m_node_marks.sync();
  m_sketch.m_dims.refresh_all_length_dimensions();

  if (m_sketch.is_current())
    m_sketch.m_view.refresh_active_sketch_grid();
}

void Sketch_topo::rebuild_dim_classifier_face_cache_()
{
  m_dim_classifier_faces.clear();
  m_dim_classifier_faces.reserve(m_faces.size());
  for (const Sketch_face_shp_ptr& fp : m_faces)
    m_dim_classifier_faces.push_back(TopoDS::Face(fp->Shape()));
}

size_t Sketch_topo::Face_edge::start_nd_idx() const
{
  if (!reversed)
    return edge.node_idx_a;

  EZY_ASSERT(edge.node_idx_b);
  return *edge.node_idx_b;
}

size_t Sketch_topo::Face_edge::end_nd_idx() const
{
  if (reversed)
    return edge.node_idx_a;

  EZY_ASSERT(edge.node_idx_b);
  return *edge.node_idx_b;
}

// Returns true when the traversal winds counter-clockwise (positive signed area),
// i.e. it bounds a real region rather than the outer/unbounded one.
bool Sketch_topo::is_face_ccw_(const Face_edges& face) const
{
  double signed_area = 0.0;
  for (const Face_edge& e : face)
  {
    const gp_Pnt2d& p1 = m_sketch.m_nodes[e.start_nd_idx()];
    const gp_Pnt2d& p2 = m_sketch.m_nodes[e.end_nd_idx()];

    if (sketch_edge_is_arc(e.edge) && !e.edge.shp.IsNull())
    {
      const gp_Pnt2d pm    = e.edge.node_idx_arc_pt.has_value()
                                 ? m_sketch.m_nodes[*e.edge.node_idx_arc_pt]
                                 : arc_curve_midpoint_2d(TopoDS::Edge(e.edge.shp->Shape()), m_sketch.m_pln);
      const double   wedge = (p1.X() * pm.Y()) - (pm.X() * p1.Y()) + (pm.X() * p2.Y()) - (p2.X() * pm.Y());
      if (std::abs(wedge) > Precision::Confusion())
      {
        signed_area += wedge;
        continue;
      }

      const TopoDS_Edge       occ_edge = TopoDS::Edge(e.edge.shp->Shape());
      const BRepAdaptor_Curve curve(occ_edge);
      const double            u0       = curve.FirstParameter();
      const double            u1       = curve.LastParameter();
      constexpr int           segments = 12;

      auto sample_arc = [&](double u_from, double u_to)
      {
        gp_Pnt2d prev = to_2d(m_sketch.m_pln, curve.Value(u_from));
        for (int i = 1; i <= segments; ++i)
        {
          const double t   = static_cast<double>(i) / static_cast<double>(segments);
          const double u   = u_from + (u_to - u_from) * t;
          gp_Pnt2d     cur = to_2d(m_sketch.m_pln, curve.Value(u));
          signed_area += (prev.X() * cur.Y()) - (cur.X() * prev.Y());
          prev = cur;
        }
      };

      if (e.reversed)
        sample_arc(u1, u0);
      else
        sample_arc(u0, u1);
    }
    else
      signed_area += (p1.X() * p2.Y()) - (p2.X() * p1.Y());
  }

  return signed_area > 0;
}

Sketch_face_shp_ptr Sketch_topo::create_face_shape_(const Face_edges& face)
{
  EZY_ASSERT(face.size() >= 2);

  BRepBuilderAPI_MakeWire wire_maker;
  std::vector<gp_Pnt>     node_verts;

  for (const Face_edge& e : face)
  {
    EZY_ASSERT(e.edge.node_idx_b);

    auto add_node_vert_unique = [&](const gp_Pnt& pt)
    {
      if (node_verts.empty())
        node_verts.push_back(pt);
      else if (!node_verts.back().IsEqual(pt, Precision::Confusion()))
        node_verts.push_back(pt);
    };

    if (sketch_edge_is_arc(e.edge))
    {
      gp_Pnt a = m_sketch.to_3d_(e.start_nd_idx());
      gp_Pnt b = m_sketch.to_3d_(e.end_nd_idx());

      add_node_vert_unique(a);
      add_node_vert_unique(b);

      TopoDS_Edge occ_edge = TopoDS::Edge(e.edge.shp->Shape());
      if (e.reversed)
        occ_edge.Reverse();

      wire_maker.Add(occ_edge);
    }
    else
    {
      gp_Pnt a = m_sketch.to_3d_(e.start_nd_idx());
      gp_Pnt b = m_sketch.to_3d_(e.end_nd_idx());

      add_node_vert_unique(a);
      add_node_vert_unique(b);

      BRepBuilderAPI_MakeEdge edge(a, b);
      EZY_ASSERT(edge.IsDone());
      wire_maker.Add(edge.Edge());
    }
  }

  EZY_ASSERT(wire_maker.IsDone());
  TopoDS_Wire wire = wire_maker.Wire();

  BRepBuilderAPI_MakeFace face_maker(wire);
  EZY_ASSERT(face_maker.IsDone());

  Sketch_face_shp* ret = new Sketch_face_shp(m_sketch, face_maker.Face());
  std::swap(node_verts, ret->verts_3d);
  return ret;
}
