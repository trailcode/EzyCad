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
std::string fmt_double(const double v)
{
  char buf[64];
  std::snprintf(buf, sizeof(buf), "%.6g", v);
  return buf;
}

void add_line(std::vector<Line>& out, const char* label, const std::string& value) { out.push_back({label, value}); }

int count_subshapes(const TopoDS_Shape& shape, const TopAbs_ShapeEnum type)
{
  int n = 0;
  for (TopExp_Explorer exp(shape, type); exp.More(); exp.Next())
    ++n;

  return n;
}

std::string shape_type_name(const TopAbs_ShapeEnum type)
{
  const auto idx = static_cast<std::size_t>(type);
  if (idx < c_names_TopAbs_ShapeEnum.size())
    return std::string(c_names_TopAbs_ShapeEnum[idx]);

  return "Unknown";
}
} // namespace

std::vector<Line> collect(const TopoDS_Shape& shape, const Display_meta* display)
{
  std::vector<Line> lines;

  if (display)
  {
    add_line(lines, "Name", display->name);
    add_line(lines, "Material", display->material);
    add_line(lines, "Display", display->display_mode);
    add_line(lines, "Visible", display->visible ? "yes" : "no");
    lines.push_back({"", ""});
  }

  if (shape.IsNull())
  {
    add_line(lines, "Shape", "null");
    return lines;
  }

  const TopAbs_ShapeEnum root_type = shape.ShapeType();
  add_line(lines, "Root type", shape_type_name(root_type));

  BRepCheck_Analyzer analyzer(shape);
  add_line(lines, "Valid", analyzer.IsValid() ? "yes" : "no");

  if (!shape.Location().IsIdentity())
    add_line(lines, "Located", "yes");

  if (root_type == TopAbs_SHELL)
    add_line(lines, "Closed shell", BRep_Tool::IsClosed(shape) ? "yes" : "no");

  const int n_compound  = count_subshapes(shape, TopAbs_COMPOUND);
  const int n_compsolid = count_subshapes(shape, TopAbs_COMPSOLID);
  const int n_solid     = count_subshapes(shape, TopAbs_SOLID);
  const int n_shell     = count_subshapes(shape, TopAbs_SHELL);
  const int n_face      = count_subshapes(shape, TopAbs_FACE);
  const int n_wire      = count_subshapes(shape, TopAbs_WIRE);
  const int n_edge      = count_subshapes(shape, TopAbs_EDGE);
  const int n_vertex    = count_subshapes(shape, TopAbs_VERTEX);

  lines.push_back({"", ""});
  add_line(lines, "Compounds", std::to_string(n_compound));
  add_line(lines, "CompSolids", std::to_string(n_compsolid));
  add_line(lines, "Solids", std::to_string(n_solid));
  add_line(lines, "Shells", std::to_string(n_shell));
  add_line(lines, "Faces", std::to_string(n_face));
  add_line(lines, "Wires", std::to_string(n_wire));
  add_line(lines, "Edges", std::to_string(n_edge));
  add_line(lines, "Vertices", std::to_string(n_vertex));

  Bnd_Box bbox;
  BRepBndLib::Add(shape, bbox);
  if (!bbox.IsVoid())
  {
    double xmin, ymin, zmin, xmax, ymax, zmax;
    bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    lines.push_back({"", ""});
    add_line(lines, "BBox X", fmt_double(xmin) + " .. " + fmt_double(xmax));
    add_line(lines, "BBox Y", fmt_double(ymin) + " .. " + fmt_double(ymax));
    add_line(lines, "BBox Z", fmt_double(zmin) + " .. " + fmt_double(zmax));
    add_line(lines, "BBox size", fmt_double(xmax - xmin) + " x " + fmt_double(ymax - ymin) + " x " + fmt_double(zmax - zmin));
  }

  GProp_GProps vol_props;
  BRepGProp::VolumeProperties(shape, vol_props);
  if (vol_props.Mass() > 0.0)
  {
    const gp_Pnt com = vol_props.CentreOfMass();
    lines.push_back({"", ""});
    add_line(lines, "Volume", fmt_double(vol_props.Mass()));
    add_line(lines, "Center of mass", fmt_double(com.X()) + ", " + fmt_double(com.Y()) + ", " + fmt_double(com.Z()));
  }

  GProp_GProps surf_props;
  BRepGProp::SurfaceProperties(shape, surf_props);
  if (surf_props.Mass() > 0.0)
    add_line(lines, "Surface area", fmt_double(surf_props.Mass()));

  GProp_GProps lin_props;
  BRepGProp::LinearProperties(shape, lin_props);
  if (lin_props.Mass() > 0.0)
    add_line(lines, "Length", fmt_double(lin_props.Mass()));

  return lines;
}
} // namespace shp_info
