#include "shp_info.h"

#include <cstdio>
#include <string>

#include <BRepBndLib.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <BRep_Tool.hxx>
#include <GProp_GProps.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>

#include "utl_occt.h"

namespace shp_info
{
namespace
{
std::string fmt_double_(const double v);
void        add_line_(std::vector<Line>& out, const char* label, const std::string& value);
int         count_subshapes_(const TopoDS_Shape& shape, const TopAbs_ShapeEnum type);
std::string shape_type_name_(const TopAbs_ShapeEnum type);
} // namespace

std::vector<Line> collect(const TopoDS_Shape& shape, const Display_meta* display)
{
  std::vector<Line> lines;

  if (display)
  {
    add_line_(lines, "Name", display->name);
    add_line_(lines, "Material", display->material);
    add_line_(lines, "Display", display->display_mode);
    add_line_(lines, "Visible", display->visible ? "yes" : "no");
    lines.push_back({"", ""});
  }

  if (shape.IsNull())
  {
    add_line_(lines, "Shape", "null");
    return lines;
  }

  const TopAbs_ShapeEnum root_type = shape.ShapeType();
  add_line_(lines, "Root type", shape_type_name_(root_type));

  BRepCheck_Analyzer analyzer(shape);
  add_line_(lines, "Valid", analyzer.IsValid() ? "yes" : "no");

  if (!shape.Location().IsIdentity())
    add_line_(lines, "Located", "yes");

  if (root_type == TopAbs_SHELL)
    add_line_(lines, "Closed shell", BRep_Tool::IsClosed(shape) ? "yes" : "no");

  const int n_compound  = count_subshapes_(shape, TopAbs_COMPOUND);
  const int n_compsolid = count_subshapes_(shape, TopAbs_COMPSOLID);
  const int n_solid     = count_subshapes_(shape, TopAbs_SOLID);
  const int n_shell     = count_subshapes_(shape, TopAbs_SHELL);
  const int n_face      = count_subshapes_(shape, TopAbs_FACE);
  const int n_wire      = count_subshapes_(shape, TopAbs_WIRE);
  const int n_edge      = count_subshapes_(shape, TopAbs_EDGE);
  const int n_vertex    = count_subshapes_(shape, TopAbs_VERTEX);

  lines.push_back({"", ""});
  add_line_(lines, "Compounds", std::to_string(n_compound));
  add_line_(lines, "CompSolids", std::to_string(n_compsolid));
  add_line_(lines, "Solids", std::to_string(n_solid));
  add_line_(lines, "Shells", std::to_string(n_shell));
  add_line_(lines, "Faces", std::to_string(n_face));
  add_line_(lines, "Wires", std::to_string(n_wire));
  add_line_(lines, "Edges", std::to_string(n_edge));
  add_line_(lines, "Vertices", std::to_string(n_vertex));

  Bnd_Box bbox;
  BRepBndLib::Add(shape, bbox);
  if (!bbox.IsVoid())
  {
    double xmin, ymin, zmin, xmax, ymax, zmax;
    bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    lines.push_back({"", ""});
    add_line_(lines, "BBox X", fmt_double_(xmin) + " .. " + fmt_double_(xmax));
    add_line_(lines, "BBox Y", fmt_double_(ymin) + " .. " + fmt_double_(ymax));
    add_line_(lines, "BBox Z", fmt_double_(zmin) + " .. " + fmt_double_(zmax));
    add_line_(lines, "BBox size", fmt_double_(xmax - xmin) + " x " + fmt_double_(ymax - ymin) + " x " + fmt_double_(zmax - zmin));
  }

  GProp_GProps vol_props;
  BRepGProp::VolumeProperties(shape, vol_props);
  if (vol_props.Mass() > 0.0)
  {
    const gp_Pnt com = vol_props.CentreOfMass();
    lines.push_back({"", ""});
    add_line_(lines, "Volume", fmt_double_(vol_props.Mass()));
    add_line_(lines, "Center of mass", fmt_double_(com.X()) + ", " + fmt_double_(com.Y()) + ", " + fmt_double_(com.Z()));
  }

  GProp_GProps surf_props;
  BRepGProp::SurfaceProperties(shape, surf_props);
  if (surf_props.Mass() > 0.0)
    add_line_(lines, "Surface area", fmt_double_(surf_props.Mass()));

  GProp_GProps lin_props;
  BRepGProp::LinearProperties(shape, lin_props);
  if (lin_props.Mass() > 0.0)
    add_line_(lines, "Length", fmt_double_(lin_props.Mass()));

  return lines;
}

namespace
{
std::string fmt_double_(const double v)
{
  char buf[64];
  std::snprintf(buf, sizeof(buf), "%.6g", v);

  return buf;
}

void add_line_(std::vector<Line>& out, const char* label, const std::string& value) { out.push_back({label, value}); }

int count_subshapes_(const TopoDS_Shape& shape, const TopAbs_ShapeEnum type)
{
  int n = 0;
  for (TopExp_Explorer exp(shape, type); exp.More(); exp.Next())
    ++n;

  return n;
}

std::string shape_type_name_(const TopAbs_ShapeEnum type)
{
  const auto idx = static_cast<std::size_t>(type);
  if (idx < c_names_TopAbs_ShapeEnum.size())
    return std::string(c_names_TopAbs_ShapeEnum[idx]);

  return "Unknown";
}
} // namespace
} // namespace shp_info
