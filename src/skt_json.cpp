#include "skt_json.h"

#include <AIS_InteractiveContext.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <Precision.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <nlohmann/json.hpp>
#include <optional>
#include <vector>

#include "utl_dbg.h"
#include "utl_geom.h"
#include "sketch.h"
#include "skt_edge.h"
#include "skt_nodes.h"
#include "skt_underlay.h"
#include "utl_asset_store.h"
#include "gui_occt_view.h"
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

  if (nd.origin)
    o["origin"] = true;

  if (!nd.name.empty())
    o["name"] = nd.name;

  return o;
}

} // namespace

void Sketch_json::load_nodes_(Sketch& sketch, const json& nodes_json)
{
  EZY_ASSERT(nodes_json.is_array());
  sketch.get_nodes().resize(nodes_json.size());
  for (size_t i = 0; i < nodes_json.size(); ++i)
  {
    const json& el = nodes_json[i];
    if (el.is_null())
    {
      sketch.get_nodes().set_node(i, gp_Pnt2d(0., 0.), true, false, false, false, "");
      continue;
    }

    EZY_ASSERT(el.is_object());
    const gp_Pnt2d pt        = ::from_json_pnt2d(el);
    const bool     midpoint  = el.contains("midpoint") && el["midpoint"].is_boolean() && el["midpoint"].get<bool>();
    const bool     permanent = el.contains("permanent") && el["permanent"].is_boolean() && el["permanent"].get<bool>();
    const bool     origin    = el.contains("origin") && el["origin"].is_boolean() && el["origin"].get<bool>();
    sketch.get_nodes().set_node(i, pt, false, midpoint, permanent, origin,
                                el.contains("name") && el["name"].is_string() ? el["name"].get<std::string>() : "");
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
    EZY_ASSERT(edge_json.is_array() && edge_json.size() >= 2);
    const std::size_t          idx_a = edge_json[0].get<std::size_t>();
    const std::size_t          idx_b = edge_json[1].get<std::size_t>();
    std::optional<std::size_t> idx_mid;
    if (edge_json.size() >= 3)
      idx_mid = edge_json[2].get<std::size_t>();

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
      const std::size_t ib   = edge_json[2].get<std::size_t>();
      const std::size_t iarc = edge_json[1].is_number_unsigned()
                                   ? edge_json[1].get<std::size_t>()
                                   : ret.get_nodes().get_node_exact(::from_json_pnt2d(edge_json[1]));
      // Matches `add_arc_circle_(pt_a, pt_b, pt_c)` -> node_idxs [a, c, b] for (json0, json1, json2) = (start, arc, end).
      ret.add_arc_circle_(std::vector<size_t>{ia, ib, iarc});
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

nlohmann::json Sketch_json::to_json(const Sketch& sketch, const Ezy_asset_store& assets)
{
  json j;
  j["isCurrent"] = sketch.is_current();
  j["id"]        = sketch.get_id();
  j["name"]      = sketch.get_name();
  j["plane"]     = ::to_json(sketch.m_pln);

  if (!sketch.m_show_origin_marker)
    j["show_origin_marker"] = false;

  if (sketch.m_originating_face)
  {
    const TopoDS_Shape& shape = sketch.m_originating_face->Shape();
    std::ostringstream  oss;
    BRepTools::Write(shape, oss, false, false, TopTools_FormatVersion_CURRENT);
    j["originating_face"] = oss.str();
  }

  json& edges_json = j["edges"] = json::array();
  json& arc_edges_json = j["arc_edges"] = json::array();

  const auto& sketch_nodes = sketch.m_nodes;

  std::vector<std::optional<size_t>> dense_of_sparse(sketch_nodes.size());
  size_t                             dense_n = 0;
  for (size_t i = 0, n = sketch_nodes.size(); i < n; ++i)
    if (!sketch_nodes[i].deleted)
      dense_of_sparse[i] = dense_n++;

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

  sketch.m_edges.for_each_linear(
      [&](const Sketch_edge_linear& e)
      {
        if (e.node_mid.has_value())
          edges_json.push_back(json::array({remap(e.node_a), remap(e.node_b), remap(*e.node_mid)}));
        else
          edges_json.push_back(json::array({remap(e.node_a), remap(e.node_b)}));
      });

  for (const Sketch_edge& e : sketch.m_edges.edges())
  {
    if (!sketch_edge_is_arc(e) || !e.node_idx_b.has_value() || e.shp.IsNull())
      continue;

    json arc_mid_json;
    if (e.node_idx_arc_pt.has_value())
      arc_mid_json = remap(*e.node_idx_arc_pt);
    else
      arc_mid_json = ::to_json(arc_curve_midpoint_2d(TopoDS::Edge(e.shp->Shape()), sketch.m_pln));

    arc_edges_json.push_back(json::array({remap(e.node_idx_a), arc_mid_json, remap(*e.node_idx_b)}));
  }

  json& len_dims_json = j["length_dimensions"] = json::array();
  for (const Sketch_dims::Length_dimension& ld : sketch.m_dims.dimensions())
  {
    json e = json::array({remap(ld.node_idx_lo), remap(ld.node_idx_hi), ld.visible});
    if (ld.flyout_offset.has_value())
      e.push_back(*ld.flyout_offset);

    if (!ld.name.empty())
      e.push_back(ld.name);

    len_dims_json.push_back(std::move(e));
  }

  if (sketch.m_underlay.has_image())
    j["underlay"] = sketch.m_underlay.to_json(assets);

  if (sketch.m_operation_axis.has_value())
  {
    const Sketch::Edge& axis = *sketch.m_operation_axis;
    EZY_ASSERT(axis.node_idx_b.has_value());
    EZY_ASSERT(!axis.shp.IsNull());
    const auto [pt_a, pt_b] = get_edge_endpoints(sketch.m_pln, TopoDS::Edge(axis.shp->Shape()));
    j["operation_axis"]     = json::array({::to_json(pt_a), ::to_json(pt_b)});
  }

  return j;
}

Sketch::sptr Sketch_json::from_json(Occt_view& view, const nlohmann::json& j)
{
  EZY_ASSERT(j.contains("name") && j["name"].is_string());
  EZY_ASSERT(j.contains("edges") && j["edges"].is_array());
  EZY_ASSERT(j.contains("plane") && j["plane"].is_object());
  EZY_ASSERT(j.contains("isCurrent") && j["isCurrent"].is_boolean());

  Sketch::sptr ret;

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
    ret->json_add_length_dimension_(ab.first, ab.second, true, std::nullopt, "");

  if (j.contains("length_dimensions") && j["length_dimensions"].is_array())
    for (const auto& pair_json : j["length_dimensions"])
    {
      EZY_ASSERT(pair_json.is_array() && (pair_json.size() >= 2 && pair_json.size() <= 5));
      const bool            visible = pair_json.size() >= 3 ? pair_json[2].get<bool>() : true;
      std::optional<double> flyout_offset;
      std::string           dim_name;
      if (pair_json.size() >= 4)
      {
        if (pair_json[3].is_number())
        {
          const double v = pair_json[3].get<double>();
          if (v > 0.0)
            flyout_offset = v;
        }
        else if (pair_json[3].is_string())
          dim_name = pair_json[3].get<std::string>();
      }

      if (pair_json.size() == 5 && pair_json[4].is_string())
        dim_name = pair_json[4].get<std::string>();

      ret->json_add_length_dimension_(pair_json[0].get<std::size_t>(), pair_json[1].get<std::size_t>(), visible, flyout_offset,
                                      dim_name);
    }

  if (j.contains("underlay") && j["underlay"].is_object())
    if (ret->m_underlay.from_json(j["underlay"], view.asset_store()))
      if (ret->m_visible && ret->m_underlay.has_image())
        ret->m_underlay.rebuild_and_display(ret->m_pln);

  if (j.contains("operation_axis") && j["operation_axis"].is_array() && j["operation_axis"].size() >= 2)
  {
    const gp_Pnt2d pt_a = ::from_json_pnt2d(j["operation_axis"][0]);
    const gp_Pnt2d pt_b = ::from_json_pnt2d(j["operation_axis"][1]);
    ret->sketch_json_set_operation_axis_(pt_a, pt_b);
  }

  if (j.contains("id") && j["id"].is_number_unsigned())
  {
    ret->m_id = j["id"].get<size_t>();
    view.adopt_sketch_id(ret->m_id);
  }

  if (j.contains("show_origin_marker") && j["show_origin_marker"].is_boolean())
    ret->m_show_origin_marker = j["show_origin_marker"].get<bool>();
  ret->m_nodes.set_origin_snap_enabled(ret->m_show_origin_marker);

  if (j["isCurrent"])
    ret->set_current();

  ret->ensure_origin_node_();

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
