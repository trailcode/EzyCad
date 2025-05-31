#include "utl_json.h"

#include <gp_Pnt.hxx>
#include <gp_Pln.hxx>

using json = nlohmann::json;  // Alias for convenience

// Helper function to serialize a gp_Pnt (3D point)
json to_json(const gp_Pnt& point)
{
  return {
      {"x", point.X()},
      {"y", point.Y()},
      {"z", point.Z()}
  };
}

// Helper function to serialize a gp_Dir (direction)
json to_json(const gp_Dir& direction)
{
  return {
      {"x", direction.X()},
      {"y", direction.Y()},
      {"z", direction.Z()}
  };
}

// Serialize a gp_Pln to JSON with origin and normal
json to_json(const gp_Pln& pln)
{
  return {
      {"origin",         to_json(pln.Location())}, // Plane’s origin point
      {"normal", to_json(pln.Axis().Direction())}, // Plane’s normal direction
      { "xAxis", to_json(pln.XAxis().Direction())}  // Plane’s X axis
  };
}

json to_json(const gp_Pnt2d& point)
{
  // Create JSON object for the point
  json point_json = {
      {"x", point.X()},
      {"y", point.Y()}
  };

  return point_json;
}

// Deserialize a gp_Pnt from JSON
gp_Pnt from_json_pnt(const json& j)
{
  return gp_Pnt(
      j.at("x").get<double>(),
      j.at("y").get<double>(),
      j.at("z").get<double>());
}

// Deserialize a gp_Dir from JSON
gp_Dir from_json_dir(const json& j)
{
  return gp_Dir(
      j.at("x").get<double>(),
      j.at("y").get<double>(),
      j.at("z").get<double>());
}

// Deserialize a gp_Pln from JSON
gp_Pln from_json_pln(const json& j)
{
  gp_Pnt origin = from_json_pnt(j.at("origin"));
  gp_Dir normal = from_json_dir(j.at("normal"));
  gp_Dir x_axis = from_json_dir(j.at("xAxis"));
  return gp_Pln(gp_Ax3(origin, normal, x_axis));
}

// Deserializes a JSON object into a gp_Pnt2d
gp_Pnt2d from_json_pnt2d(const json& j)
{
  return gp_Pnt2d(j.at("x").get<double>(), j.at("y").get<double>());
}
