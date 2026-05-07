#include "sketch_json.h"

#include <AIS_InteractiveContext.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <Precision.hxx>
#include <TopoDS.hxx>
#include <nlohmann/json.hpp>
#include <optional>
#include <vector>

#include "dbg.h"
#include "sketch.h"
#include "sketch_nodes.h"
#include "sketch_underlay.h"
#include "utl_json.h"

using json = nlohmann::json;

namespace
{
std::optional<size_t> find_live_node_at_(Sketch& sketch, const gp_Pnt2d& p)
{
  const Sketch_nodes& ns = sketch.get_nodes();
  for (size_t i = 0, n = ns.size(); i < n; ++i)
    if (!ns[i].deleted && ns[i].SquareDistance(p) < Precision::SquareConfusion())
      return i;

  return std::nullopt;
}

/// Serializes one sketch node for `j["nodes"]`. Caller must only pass non-deleted nodes (compact save omits tombstones).
json node_to_json_(const Sketch_nodes::Node& nd)
{
  EZY_ASSERT(!nd.deleted);
  json o = ::to_json(static_cast<const gp_Pnt2d&>(nd));
  if (nd.midpoint)
    o["midpoint"] = true;

  if (nd.permanent)
    o["permanent"] = true;

  return o;
}

}  // namespace

void Sketch_json::load_nodes_(Sketch& sketch, const json& nodes_json)
{
  EZY_ASSERT(nodes_json.is_array());
  sketch.get_nodes().json_resize(nodes_json.size());
  for (size_t i = 0; i < nodes_json.size(); ++i)
  {
    const json& el = nodes_json[i];
    if (el.is_null())
    {
      sketch.get_nodes().json_set_node(i, gp_Pnt2d(0., 0.), true, false, false);
      continue;
    }
    EZY_ASSERT(el.is_object());
    const gp_Pnt2d pt = ::from_json_pnt2d(el);
    const bool     midpoint =
        el.contains("midpoint") && el["midpoint"].is_boolean() && el["midpoint"].get<bool>();
    const bool permanent =
        el.contains("permanent") && el["permanent"].is_boolean() && el["permanent"].get<bool>();
    sketch.get_nodes().json_set_node(i, pt, false, midpoint, permanent);
  }
}

/// Sparse index-based files (legacy): `nodes` may contain `null` tombstones; edge indices match sketch indices.
/// Compact index-based files: `nodes` has only live entries; edge indices are dense `0..nodes.size()-1`.
void Sketch_json::from_json_indexed_(Sketch& ret, const json& j,
                                     std::vector<std::pair<std::size_t, std::size_t>>* out_legacy_length_dim_endpoints)
{
  EZY_ASSERT(j.contains("nodes") && j["nodes"].is_array());
  load_nodes_(ret, j["nodes"]);

  for (const auto& edge_json : j["edges"])
  {
    EZY_ASSERT(edge_json.is_array() && edge_json.size() >= 3);
    const std::size_t idx_a   = edge_json[0].get<std::size_t>();
    const std::size_t idx_b   = edge_json[1].get<std::size_t>();
    const std::size_t idx_mid = edge_json[2].get<std::size_t>();
    ret.sketch_json_add_linear_edge_(idx_a, idx_b, idx_mid);
    if (out_legacy_length_dim_endpoints && edge_json.size() >= 4)
      if (edge_json[3].is_boolean())
        if (edge_json[3].get<bool>())
          out_legacy_length_dim_endpoints->emplace_back(idx_a, idx_b);
  }

  if (j.contains("arc_edges") && j["arc_edges"].is_array())
    for (const auto& edge_json : j["arc_edges"])
    {
      EZY_ASSERT(edge_json.is_array() && edge_json.size() == 3);
      const std::size_t ia   = edge_json[0].get<std::size_t>();
      const std::size_t iarc = edge_json[1].get<std::size_t>();
      const std::size_t ib   = edge_json[2].get<std::size_t>();
      // Matches `add_arc_circle_(pt_a, pt_b, pt_c)` -> node_idxs [a, c, b] for (json0, json1, json2) = (start, arc, end).
      ret.add_arc_circle_(std::vector<size_t> {ia, ib, iarc});
    }
}

void Sketch_json::from_json_legacy_coords_(Sketch& ret, const json& j,
                                           std::vector<std::pair<std::size_t, std::size_t>>* out_legacy_length_dim_endpoints)
{
  for (const auto& edge_json : j["edges"])
  {
    EZY_ASSERT(edge_json.is_array() && edge_json.size() >= 2);
    const gp_Pnt2d pt_a = ::from_json_pnt2d(edge_json[0]);
    const gp_Pnt2d pt_b = ::from_json_pnt2d(edge_json[1]);
    ret.add_edge_(pt_a, pt_b);
    if (out_legacy_length_dim_endpoints && edge_json.size() >= 3)
      if (edge_json[2].is_boolean() && edge_json[2].get<bool>())
        if (std::optional<size_t> ia = find_live_node_at_(ret, pt_a))
          if (std::optional<size_t> ib = find_live_node_at_(ret, pt_b))
            out_legacy_length_dim_endpoints->emplace_back(*ia, *ib);
  }

  if (j.contains("arc_edges") && j["arc_edges"].is_array())
    for (const auto& edge_json : j["arc_edges"])
    {
      EZY_ASSERT(edge_json.is_array() && edge_json.size() == 3);
      const gp_Pnt2d pt_a = ::from_json_pnt2d(edge_json[0]);
      const gp_Pnt2d pt_b = ::from_json_pnt2d(edge_json[1]);
      const gp_Pnt2d pt_c = ::from_json_pnt2d(edge_json[2]);
      ret.add_arc_circle_(pt_a, pt_b, pt_c);
    }
}

nlohmann::json Sketch_json::to_json(const Sketch& sketch)
{
  json j;
  j["isCurrent"] = sketch.is_current();
  j["name"]      = sketch.get_name();
  j["plane"]     = ::to_json(sketch.m_pln);

  if (sketch.m_originating_face)
  {
    const TopoDS_Shape& shape = sketch.m_originating_face->Shape();
    std::ostringstream  oss;
    BRepTools::Write(shape, oss, false, false, TopTools_FormatVersion_CURRENT);
    j["originating_face"] = oss.str();
  }

  json& edges_json = j["edges"] = json::array();
  json& arc_edges_json = j["arc_edges"] = json::array();

  const Sketch::Edge* last_arc_circle_edge = nullptr;

  const auto& sketch_nodes = sketch.m_nodes;

  std::vector<std::optional<size_t>> dense_of_sparse(sketch_nodes.size());
  size_t                             dense_n = 0;
  for (size_t i = 0, n = sketch_nodes.size(); i < n; ++i)
  {
    if (!sketch_nodes[i].deleted)
      dense_of_sparse[i] = dense_n++;
  }

  json& nodes_json = j["nodes"] = json::array();
  for (size_t i = 0, n = sketch_nodes.size(); i < n; ++i)
  {
    if (sketch_nodes[i].deleted)
      continue;

    nodes_json.push_back(node_to_json_(sketch_nodes[i]));
  }

  auto remap = [&](size_t sparse_idx) -> size_t
  {
    EZY_ASSERT(sparse_idx < dense_of_sparse.size());
    EZY_ASSERT(dense_of_sparse[sparse_idx].has_value());
    return *dense_of_sparse[sparse_idx];
  };

  for (const auto& edge : sketch.m_edges)
  {
    EZY_ASSERT(edge.node_idx_b.has_value());

    if (!edge.circle_arc)
    {
      EZY_ASSERT(edge.node_idx_mid.has_value());
      edges_json.push_back(
          json::array({remap(edge.node_idx_a), remap(*edge.node_idx_b), remap(*edge.node_idx_mid)}));
    }
    else
    {
      if (last_arc_circle_edge)
      {
        EZY_ASSERT(last_arc_circle_edge->circle_arc.get() == edge.circle_arc.get());
        EZY_ASSERT(last_arc_circle_edge->node_idx_arc.has_value());
        arc_edges_json.push_back(json::array({remap(last_arc_circle_edge->node_idx_a),
                                              remap(*last_arc_circle_edge->node_idx_arc),
                                              remap(*edge.node_idx_b)}));
        last_arc_circle_edge = nullptr;
      }
      else
      {
        EZY_ASSERT(edge.node_idx_arc.has_value());
        EZY_ASSERT(*edge.node_idx_b == *edge.node_idx_arc);
        last_arc_circle_edge = &edge;
      }
    }
  }

  json& len_dims_json = j["length_dimensions"] = json::array();
  for (const Sketch::Length_dimension& ld : sketch.m_length_dimensions)
  {
    json e = json::array({remap(ld.node_idx_lo), remap(ld.node_idx_hi), ld.visible});
    if (ld.flyout_offset.has_value())
      e.push_back(*ld.flyout_offset);
    len_dims_json.push_back(std::move(e));
  }

  if (sketch.m_underlay && sketch.m_underlay->has_image())
    j["underlay"] = sketch.m_underlay->to_json();

  return j;
}

std::shared_ptr<Sketch> Sketch_json::from_json(Occt_view& view, const nlohmann::json& j)
{
  EZY_ASSERT(j.contains("name") && j["name"].is_string());
  EZY_ASSERT(j.contains("edges") && j["edges"].is_array());
  EZY_ASSERT(j.contains("plane") && j["plane"].is_object());
  EZY_ASSERT(j.contains("isCurrent") && j["isCurrent"].is_boolean());

  std::shared_ptr<Sketch> ret;

  if (j.contains("originating_face"))
  {
    TopoDS_Shape       shape;
    std::istringstream iss;
    iss.str(j["originating_face"].get<std::string>());
    BRepTools::Read(shape, iss, BRep_Builder());
    EZY_ASSERT(shape.ShapeType() == TopAbs_WIRE);
    const TopoDS_Wire& face = TopoDS::Wire(shape);

    ret = std::make_shared<Sketch>(j["name"], view, from_json_pln(j["plane"]), face);
  }
  else
    ret = std::make_shared<Sketch>(j["name"], view, from_json_pln(j["plane"]));

  std::vector<std::pair<std::size_t, std::size_t>> legacy_length_dim_endpoints;

  const bool use_indices = edges_use_node_indices_(j);
  if (use_indices)
  {
    EZY_ASSERT(j.contains("nodes") && j["nodes"].is_array());
    from_json_indexed_(*ret, j, &legacy_length_dim_endpoints);
  }
  else
    from_json_legacy_coords_(*ret, j, &legacy_length_dim_endpoints);

  ret->update_faces_();

  for (const auto& ab : legacy_length_dim_endpoints)
    ret->json_add_length_dimension_(ab.first, ab.second);

  if (j.contains("length_dimensions") && j["length_dimensions"].is_array())
    for (const auto& pair_json : j["length_dimensions"])
    {
      EZY_ASSERT(pair_json.is_array() && (pair_json.size() == 2 || pair_json.size() == 3 || pair_json.size() == 4));
      const bool visible = pair_json.size() >= 3 ? pair_json[2].get<bool>() : true;
      std::optional<double> flyout_offset;
      if (pair_json.size() == 4)
      {
        const double v = pair_json[3].get<double>();
        if (v > 0.0)
          flyout_offset = v;
      }
      ret->json_add_length_dimension_(pair_json[0].get<std::size_t>(), pair_json[1].get<std::size_t>(), visible,
                                      flyout_offset);
    }

  if (j.contains("underlay") && j["underlay"].is_object())
  {
    auto ul = std::make_unique<Sketch_underlay>();
    if (ul->from_json(j["underlay"]))
    {
      ret->m_underlay = std::move(ul);
      if (ret->m_visible && ret->m_underlay->has_image())
        ret->m_underlay->rebuild_and_display(ret->m_pln, ret->m_ctx);
    }
  }

  if (j["isCurrent"])
    ret->set_current();

  return ret;
}

bool Sketch_json::edges_use_node_indices_(const json& j)
{
  const bool has_nodes = j.contains("nodes") && j["nodes"].is_array();
  if (!j.contains("edges") || !j["edges"].is_array())
    return false;

  if (j["edges"].empty())
    return has_nodes;

  const auto& e0 = j["edges"][0];
  if (!e0.is_array() || e0.empty())
    return false;

  return e0[0].is_number();
}
