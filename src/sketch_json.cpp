#include "sketch_json.h"

#include <AIS_InteractiveContext.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <TopoDS.hxx>
#include <nlohmann/json.hpp>

#include "sketch.h"
#include "utl_json.h"

using json = nlohmann::json;  // Alias for convenience

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
    BRepTools::Write(shape, oss, false, false, TopTools_FormatVersion_CURRENT);  // Write BREP data to the stream
    j["originating_face"] = oss.str();
  }

  json& edges_json = j["edges"] = json::array();
  json& arc_edges_json = j["arc_edges"] = json::array();

  const Sketch::Edge* last_arc_circle_edge = nullptr;

  const auto& sketch_nodes = sketch.m_nodes;
  for (const auto& edge : sketch.m_edges)
  {
    DO_ASSERT(edge.node_idx_b.has_value());
    if (!edge.circle_arc)
      edges_json.push_back({::to_json(sketch_nodes[edge.node_idx_a]),
                            ::to_json(sketch_nodes[edge.node_idx_b]),
                            edge.dim.IsNull() ? false : true});
    else
    {
      if (last_arc_circle_edge)
      {
        DO_ASSERT(last_arc_circle_edge->circle_arc.get() == edge.circle_arc.get());
        arc_edges_json.push_back({::to_json(sketch_nodes[last_arc_circle_edge->node_idx_a]),
                                  ::to_json(sketch_nodes[last_arc_circle_edge->node_idx_arc]),
                                  ::to_json(sketch_nodes[edge.node_idx_b])});
        last_arc_circle_edge = nullptr;
      }
      else
      {
        // This is the first part of the circle arc.
        DO_ASSERT(edge.node_idx_arc);
        DO_ASSERT(*edge.node_idx_b == *edge.node_idx_arc);
        last_arc_circle_edge = &edge;
      }
    }
  }

  return j;
}

std::shared_ptr<Sketch> Sketch_json::from_json(Occt_view& view, const nlohmann::json& j)
{
  DO_ASSERT(j.contains("name") && j["name"].is_string());
  DO_ASSERT(j.contains("edges") && j["edges"].is_array());
  DO_ASSERT(j.contains("arc_edges") && j["arc_edges"].is_array());
  DO_ASSERT(j.contains("plane") && j["plane"].is_object());
  DO_ASSERT(j.contains("isCurrent") && j["isCurrent"].is_boolean());

  std::shared_ptr<Sketch> ret;

  if (j.contains("originating_face"))
  {
    TopoDS_Shape       shape;
    std::istringstream iss;
    iss.str(j["originating_face"]);
    BRepTools::Read(shape, iss, BRep_Builder());
    DO_ASSERT(shape.ShapeType() == TopAbs_WIRE);
    // Cast to TopoDS_Wire
    const TopoDS_Wire& face = TopoDS::Wire(shape);

    ret = std::make_shared<Sketch>(j["name"], view, from_json_pln(j["plane"]), face);
  }
  else
    ret = std::make_shared<Sketch>(j["name"], view, from_json_pln(j["plane"]));

  // Process each edge in the JSON array
  for (const auto& edge_json : j["edges"])
  {
    DO_ASSERT(edge_json.is_array() && edge_json.size() == 3);

    // Extract the two points
    gp_Pnt2d pt_a = ::from_json_pnt2d(edge_json[0]);
    gp_Pnt2d pt_b = ::from_json_pnt2d(edge_json[1]);

    ret->add_edge_(pt_a, pt_b, edge_json[2]);
  }

  for (const auto& edge_json : j["arc_edges"])
  {
    DO_ASSERT(edge_json.is_array() && edge_json.size() == 3);
    // Extract the three points
    gp_Pnt2d pt_a = ::from_json_pnt2d(edge_json[0]);
    gp_Pnt2d pt_b = ::from_json_pnt2d(edge_json[1]);
    gp_Pnt2d pt_c = ::from_json_pnt2d(edge_json[2]);

    ret->add_arc_circle_(pt_a, pt_b, pt_c);
  }

  ret->update_faces_();
  if (j["isCurrent"])
    ret->set_current();

  return ret;
}