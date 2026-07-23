#include "utl_cad_file_info.h"

#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <sstream>
#include <string>

#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <IGESControl_Reader.hxx>
#include <Interface_Static.hxx>
#include <NCollection_Sequence.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <STEPControl_Reader.hxx>
#include <TCollection_AsciiString.hxx>
#include <TCollection_ExtendedString.hxx>
#include <TDataStd_Name.hxx>
#include <TDF_Label.hxx>
#include <TDocStd_Document.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Shape.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>

#include "utl_occt.h"
#include "utl_types.h"

namespace utl_cad_file_info
{
namespace
{
void add_line(std::vector<Line>& out, const char* label, const std::string& value) { out.push_back({label, value}); }

void add_blank(std::vector<Line>& out) { out.push_back({"", ""}); }

std::string to_lower_ext(const std::string& path)
{
  std::string ext = std::filesystem::path(path).extension().string();
  for (char& c : ext)
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

  return ext;
}

std::string fmt_bytes(const size_t n)
{
  char buf[64];
  if (n < 1024)
    std::snprintf(buf, sizeof(buf), "%zu B", n);
  else if (n < 1024ull * 1024ull)
    std::snprintf(buf, sizeof(buf), "%.1f KB", static_cast<double>(n) / 1024.0);
  else
    std::snprintf(buf, sizeof(buf), "%.2f MB", static_cast<double>(n) / (1024.0 * 1024.0));

  return buf;
}

std::string fmt_double(const double v)
{
  char buf[64];
  std::snprintf(buf, sizeof(buf), "%.6g", v);
  return buf;
}

bool starts_with_ci(const std::string& s, const char* prefix)
{
  const size_t n = std::strlen(prefix);
  if (s.size() < n)
    return false;

  for (size_t i = 0; i < n; ++i)
    if (std::tolower(static_cast<unsigned char>(s[i])) != std::tolower(static_cast<unsigned char>(prefix[i])))
      return false;

  return true;
}

bool looks_like_ply(const std::string& bytes) { return starts_with_ci(bytes, "ply"); }

bool looks_like_step(const std::string& bytes)
{
  // ISO-10303-21 exchange files typically start with "ISO-10303-21;"
  return bytes.find("ISO-10303-21") != std::string::npos;
}

bool looks_like_iges(const std::string& bytes)
{
  // Start / global / directory / parameter / terminate section markers (cols 73-80).
  if (bytes.size() < 80)
    return false;

  const char c73 = bytes[72];
  return c73 == 'S' || c73 == 'G' || c73 == 'D' || c73 == 'P' || c73 == 'T';
}

bool is_binary_stl(const std::string& bytes)
{
  if (bytes.size() < 84)
    return false;

  uint32_t tri_count = 0;
  std::memcpy(&tri_count, bytes.data() + 80, sizeof(tri_count));
  const size_t expected = 84ull + 50ull * static_cast<size_t>(tri_count);
  return expected == bytes.size();
}

bool looks_like_stl(const std::string& bytes)
{
  if (is_binary_stl(bytes))
    return true;

  return starts_with_ci(bytes, "solid");
}

int count_subshapes(const TopoDS_Shape& shape, const TopAbs_ShapeEnum type)
{
  int n = 0;
  for (TopExp_Explorer exp(shape, type); exp.More(); exp.Next())
    ++n;

  return n;
}

void append_shape_summary(std::vector<Line>& lines, const TopoDS_Shape& shape)
{
  if (shape.IsNull())
  {
    add_line(lines, "Shape", "null");
    return;
  }

  add_line(lines, "Solids", std::to_string(count_subshapes(shape, TopAbs_SOLID)));
  add_line(lines, "Shells", std::to_string(count_subshapes(shape, TopAbs_SHELL)));
  add_line(lines, "Faces", std::to_string(count_subshapes(shape, TopAbs_FACE)));
  add_line(lines, "Wires", std::to_string(count_subshapes(shape, TopAbs_WIRE)));
  add_line(lines, "Edges", std::to_string(count_subshapes(shape, TopAbs_EDGE)));
  add_line(lines, "Vertices", std::to_string(count_subshapes(shape, TopAbs_VERTEX)));

  Bnd_Box bbox;
  BRepBndLib::Add(shape, bbox);
  if (bbox.IsVoid())
    return;

  double xmin, ymin, zmin, xmax, ymax, zmax;
  bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
  add_blank(lines);
  add_line(lines, "BBox X", fmt_double(xmin) + " .. " + fmt_double(xmax));
  add_line(lines, "BBox Y", fmt_double(ymin) + " .. " + fmt_double(ymax));
  add_line(lines, "BBox Z", fmt_double(zmin) + " .. " + fmt_double(zmax));
  add_line(lines, "BBox size", fmt_double(xmax - xmin) + " x " + fmt_double(ymax - ymin) + " x " + fmt_double(zmax - zmin));
}

void append_common(std::vector<Line>& lines, const std::string& file_path, const std::string& file_bytes, Format fmt)
{
  const std::string name = std::filesystem::path(file_path).filename().string();
  add_line(lines, "File", name.empty() ? file_path : name);
  if (!file_path.empty() && (file_path.find('/') != std::string::npos || file_path.find('\\') != std::string::npos))
    add_line(lines, "Path", file_path);

  add_line(lines, "Format", format_label(fmt));
  add_line(lines, "Size", fmt_bytes(file_bytes.size()));
  add_line(lines, "Importable", can_import(fmt) ? "yes (File -> Import)" : "no (export-only)");
  add_line(lines, "Exportable", "yes (File -> Export)");
}

std::vector<Line> collect_step(const std::string& file_path, const std::string& file_bytes)
{
  std::vector<Line> lines;
  append_common(lines, file_path, file_bytes, Format::Step);
  add_blank(lines);

  Interface_Static::SetCVal("xstep.cascade.unit", "MM");

  STEPControl_Reader          reader;
  std::istringstream          stream(file_bytes);
  const IFSelect_ReturnStatus read_st = reader.ReadStream("", stream);
  if (read_st != IFSelect_RetDone)
  {
    add_line(lines, "Status", "could not read STEP data");
    return lines;
  }

  const int nb_roots = reader.NbRootsForTransfer();
  add_line(lines, "Roots", std::to_string(nb_roots));

  const int transferred = reader.TransferRoots();
  add_line(lines, "Transferred", std::to_string(transferred));
  add_line(lines, "Shapes", std::to_string(reader.NbShapes()));

  std::vector<TopoDS_Shape> bodies;
  for (int i = 1; i <= reader.NbShapes(); ++i)
    append_cad_import_bodies(reader.Shape(i), bodies);
  add_line(lines, "Import bodies", std::to_string(bodies.size()));

  int named_count = 0;
  {
    std::vector<Named_body> named;
    if (read_step_named_bodies(file_bytes, named).is_ok())
    {
      for (const Named_body& b : named)
        if (!b.name.empty())
          ++named_count;
    }
  }
  add_line(lines, "Named bodies", std::to_string(named_count));

  const char* cascade = Interface_Static::CVal("xstep.cascade.unit");
  if (cascade && cascade[0] != '\0')
    add_line(lines, "Cascade unit", cascade);

  add_blank(lines);
  append_shape_summary(lines, reader.OneShape());
  return lines;
}

std::vector<Line> collect_iges(const std::string& file_path, const std::string& file_bytes)
{
  std::vector<Line> lines;
  append_common(lines, file_path, file_bytes, Format::Iges);
  add_blank(lines);

  IGESControl_Reader          reader;
  std::istringstream          stream(file_bytes);
  const IFSelect_ReturnStatus read_st = reader.ReadStream("", stream);
  if (read_st != IFSelect_RetDone)
  {
    add_line(lines, "Status", "could not read IGES data");
    return lines;
  }

  const int nb_roots = reader.NbRootsForTransfer();
  add_line(lines, "Roots", std::to_string(nb_roots));

  const int transferred = reader.TransferRoots();
  add_line(lines, "Transferred", std::to_string(transferred));
  add_line(lines, "Shapes", std::to_string(reader.NbShapes()));

  add_blank(lines);
  append_shape_summary(lines, reader.OneShape());
  return lines;
}

std::vector<Line> collect_stl(const std::string& file_path, const std::string& file_bytes)
{
  std::vector<Line> lines;
  append_common(lines, file_path, file_bytes, Format::Stl);
  add_blank(lines);

  if (is_binary_stl(file_bytes))
  {
    uint32_t tri_count = 0;
    std::memcpy(&tri_count, file_bytes.data() + 80, sizeof(tri_count));
    std::string header(file_bytes.data(), 80);
    while (!header.empty() && (header.back() == '\0' || std::isspace(static_cast<unsigned char>(header.back()))))
      header.pop_back();

    add_line(lines, "Encoding", "binary");
    if (!header.empty())
      add_line(lines, "Header", header);

    add_line(lines, "Triangles", std::to_string(tri_count));
    return lines;
  }

  add_line(lines, "Encoding", "ASCII");
  size_t facets = 0;
  size_t pos    = 0;
  while (pos < file_bytes.size())
  {
    const size_t end = file_bytes.find('\n', pos);
    const size_t len = (end == std::string::npos ? file_bytes.size() : end) - pos;
    std::string  line(file_bytes, pos, len);
    pos = (end == std::string::npos) ? file_bytes.size() : end + 1;

    // Trim leading spaces
    size_t i = 0;
    while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i])))
      ++i;

    if (line.compare(i, 5, "facet") == 0)
      ++facets;
    else if (facets == 0 && line.compare(i, 5, "solid") == 0)
    {
      std::string name = line.substr(i + 5);
      size_t      j    = 0;
      while (j < name.size() && std::isspace(static_cast<unsigned char>(name[j])))
        ++j;

      name.erase(0, j);
      if (!name.empty())
        add_line(lines, "Solid name", name);
    }
  }

  add_line(lines, "Triangles", std::to_string(facets));
  return lines;
}

std::vector<Line> collect_ply(const std::string& file_path, const std::string& file_bytes)
{
  std::vector<Line> lines;
  append_common(lines, file_path, file_bytes, Format::Ply);
  add_blank(lines);

  if (!looks_like_ply(file_bytes))
  {
    add_line(lines, "Status", "not a PLY file (missing ply magic)");
    return lines;
  }

  std::istringstream in(file_bytes);
  std::string        line;
  if (!std::getline(in, line))
  {
    add_line(lines, "Status", "empty file");
    return lines;
  }

  std::string format = "unknown";
  int         n_vert = -1;
  int         n_face = -1;
  bool        ended  = false;

  while (std::getline(in, line))
  {
    if (!line.empty() && line.back() == '\r')
      line.pop_back();

    if (line == "end_header")
    {
      ended = true;
      break;
    }

    if (line.rfind("format ", 0) == 0)
      format = line.substr(7);
    else if (line.rfind("element vertex ", 0) == 0)
      n_vert = std::atoi(line.c_str() + 15);
    else if (line.rfind("element face ", 0) == 0)
      n_face = std::atoi(line.c_str() + 13);
  }

  add_line(lines, "Encoding", format);
  if (n_vert >= 0)
    add_line(lines, "Vertices", std::to_string(n_vert));

  if (n_face >= 0)
    add_line(lines, "Faces", std::to_string(n_face));

  if (!ended)
    add_line(lines, "Status", "header incomplete (no end_header)");

  return lines;
}
} // namespace

Format detect(const std::string& file_path, const std::string& file_bytes)
{
  const std::string ext = to_lower_ext(file_path);
  if (ext == ".step" || ext == ".stp")
    return Format::Step;

  if (ext == ".igs" || ext == ".iges")
    return Format::Iges;

  if (ext == ".stl")
    return Format::Stl;

  if (ext == ".ply")
    return Format::Ply;

  // Content sniff when extension is missing or wrong (e.g. browser basename only).
  if (looks_like_ply(file_bytes))
    return Format::Ply;

  if (looks_like_step(file_bytes))
    return Format::Step;

  if (looks_like_stl(file_bytes))
    return Format::Stl;

  if (looks_like_iges(file_bytes))
    return Format::Iges;

  return Format::Unknown;
}

bool can_import(Format fmt) { return fmt == Format::Step || fmt == Format::Ply; }

const char* format_label(Format fmt)
{
  switch (fmt)
  {
  case Format::Step:
    return "STEP";
  case Format::Iges:
    return "IGES";
  case Format::Stl:
    return "STL";
  case Format::Ply:
    return "PLY";
  default:
    return "Unknown";
  }
}

std::vector<Line> collect(const std::string& file_path, const std::string& file_bytes)
{
  const Format fmt = detect(file_path, file_bytes);
  switch (fmt)
  {
  case Format::Step:
    return collect_step(file_path, file_bytes);
  case Format::Iges:
    return collect_iges(file_path, file_bytes);
  case Format::Stl:
    return collect_stl(file_path, file_bytes);
  case Format::Ply:
    return collect_ply(file_path, file_bytes);
  default:
  {
    std::vector<Line> lines;
    append_common(lines, file_path, file_bytes, Format::Unknown);
    add_blank(lines);
    add_line(lines, "Status", "unsupported or unrecognized format");
    add_line(lines, "Hint", "Open STEP, IGES, STL, or PLY");
    return lines;
  }
  }
}

namespace
{
std::string trim_name(std::string s)
{
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
    s.pop_back();

  size_t i = 0;
  while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i])))
    ++i;

  s.erase(0, i);
  return s;
}

std::string xcaf_label_name(const TDF_Label& lab)
{
  TDataStd_Name_ptr attr;
  if (!lab.FindAttribute(TDataStd_Name::GetID(), attr) || attr.IsNull())
    return {};

  const TCollection_AsciiString ascii(attr->Get(), '?');
  return trim_name(std::string(ascii.ToCString()));
}

std::string xcaf_product_name(const XCAFDoc_ShapeTool_ptr& shapes, const TDF_Label& lab)
{
  TDF_Label ref;
  if (shapes->GetReferredShape(lab, ref))
  {
    const std::string referred = xcaf_label_name(ref);
    if (!referred.empty())
      return referred;
  }

  return xcaf_label_name(lab);
}

void append_named_from_label(const XCAFDoc_ShapeTool_ptr& shapes, const TDF_Label& lab, std::vector<Named_body>& out)
{
  if (shapes->IsAssembly(lab))
  {
    NCollection_Sequence<TDF_Label> comps;
    shapes->GetComponents(lab, comps, false);
    for (int i = 1; i <= comps.Length(); ++i)
      append_named_from_label(shapes, comps.Value(i), out);
    return;
  }

  const std::string name = xcaf_product_name(shapes, lab);
  TopoDS_Shape      shape;
  if (!XCAFDoc_ShapeTool::GetShape(lab, shape) || shape.IsNull())
    return;

  std::vector<TopoDS_Shape> bodies;
  append_cad_import_bodies(shape, bodies);
  for (TopoDS_Shape& body : bodies)
    out.push_back(Named_body{std::move(body), name});
}

void append_tree_from_label(const XCAFDoc_ShapeTool_ptr& shapes, const TDF_Label& lab, int parent_index,
                            std::vector<Named_node>& out)
{
  const std::string name = xcaf_product_name(shapes, lab);

  if (shapes->IsAssembly(lab))
  {
    Named_node grp;
    grp.name         = name.empty() ? "Assembly" : name;
    grp.is_group     = true;
    grp.parent_index = parent_index;
    const int grp_i  = static_cast<int>(out.size());
    out.push_back(std::move(grp));

    NCollection_Sequence<TDF_Label> comps;
    shapes->GetComponents(lab, comps, false);
    for (int i = 1; i <= comps.Length(); ++i)
      append_tree_from_label(shapes, comps.Value(i), grp_i, out);
    return;
  }

  TopoDS_Shape shape;
  if (!XCAFDoc_ShapeTool::GetShape(lab, shape) || shape.IsNull())
    return;

  std::vector<TopoDS_Shape> bodies;
  append_cad_import_bodies(shape, bodies);
  if (bodies.empty())
    return;

  // Multiple solids under one product: wrap in a group when more than one body.
  int leaf_parent = parent_index;
  if (bodies.size() > 1)
  {
    Named_node grp;
    grp.name         = name.empty() ? "Compound" : name;
    grp.is_group     = true;
    grp.parent_index = parent_index;
    leaf_parent      = static_cast<int>(out.size());
    out.push_back(std::move(grp));
  }

  for (size_t bi = 0; bi < bodies.size(); ++bi)
  {
    Named_node leaf;
    leaf.shape        = std::move(bodies[bi]);
    leaf.name         = name;
    leaf.is_group     = false;
    leaf.parent_index = leaf_parent;
    if (bodies.size() > 1 && !name.empty())
      leaf.name = name + "." + std::to_string(bi + 1);
    out.push_back(std::move(leaf));
  }
}

Status read_step_named_bodies_xcaf(const std::string& file_bytes, std::vector<Named_body>& out)
{
  Interface_Static::SetCVal("xstep.cascade.unit", "MM");

  STEPCAFControl_Reader reader;
  reader.SetNameMode(true);
  reader.SetColorMode(false);
  reader.SetLayerMode(false);

  std::istringstream          stream(file_bytes);
  const IFSelect_ReturnStatus read_st = reader.ReadStream("", stream);
  if (read_st != IFSelect_RetDone)
    return Status::user_error("STEP: could not read file (invalid or corrupt STEP data).");

  XCAFApp_Application_ptr app = XCAFApp_Application::GetApplication();
  if (app.IsNull())
    return Status::user_error("STEP: XCAF application unavailable.");

  TDocStd_Document_ptr doc;
  app->NewDocument("MDTV-XCAF", doc);
  if (doc.IsNull())
    return Status::user_error("STEP: could not create XCAF document.");

  if (!reader.Transfer(doc))
    return Status::user_error("STEP: no geometry was transferred from the file.");

  XCAFDoc_ShapeTool_ptr shapes = XCAFDoc_DocumentTool::ShapeTool(doc->Main());
  if (shapes.IsNull())
    return Status::user_error("STEP: missing XCAF shape tool.");

  NCollection_Sequence<TDF_Label> free_shapes;
  shapes->GetFreeShapes(free_shapes);
  if (free_shapes.IsEmpty())
    return Status::user_error("STEP: no valid shapes in file.");

  for (int i = 1; i <= free_shapes.Length(); ++i)
    append_named_from_label(shapes, free_shapes.Value(i), out);

  if (out.empty())
    return Status::user_error("STEP: no valid shapes in file.");

  return Status::ok();
}

Status read_step_named_bodies_plain(const std::string& file_bytes, std::vector<Named_body>& out)
{
  Interface_Static::SetCVal("xstep.cascade.unit", "MM");

  STEPControl_Reader reader;
  std::istringstream stream(file_bytes);
  if (reader.ReadStream("", stream) != IFSelect_RetDone)
    return Status::user_error("STEP: could not read file (invalid or corrupt STEP data).");

  if (reader.TransferRoots() == 0)
    return Status::user_error("STEP: no geometry was transferred from the file.");

  for (int i = 1; i <= reader.NbShapes(); ++i)
  {
    TopoDS_Shape shape = reader.Shape(i);
    if (shape.IsNull())
      continue;

    std::vector<TopoDS_Shape> bodies;
    append_cad_import_bodies(shape, bodies);
    for (TopoDS_Shape& body : bodies)
      out.push_back(Named_body{std::move(body), {}});
  }

  if (out.empty())
    return Status::user_error("STEP: no valid shapes in file.");

  return Status::ok();
}
} // namespace

Status read_step_named_bodies(const std::string& file_bytes, std::vector<Named_body>& out)
{
  out.clear();
  const Status xcaf = read_step_named_bodies_xcaf(file_bytes, out);
  if (xcaf.is_ok())
    return xcaf;

  out.clear();
  return read_step_named_bodies_plain(file_bytes, out);
}

Status read_step_named_tree_xcaf(const std::string& file_bytes, std::vector<Named_node>& out)
{
  Interface_Static::SetCVal("xstep.cascade.unit", "MM");

  STEPCAFControl_Reader reader;
  reader.SetNameMode(true);
  reader.SetColorMode(false);
  reader.SetLayerMode(false);

  std::istringstream          stream(file_bytes);
  const IFSelect_ReturnStatus read_st = reader.ReadStream("", stream);
  if (read_st != IFSelect_RetDone)
    return Status::user_error("STEP: could not read file (invalid or corrupt STEP data).");

  XCAFApp_Application_ptr app = XCAFApp_Application::GetApplication();
  if (app.IsNull())
    return Status::user_error("STEP: XCAF application unavailable.");

  TDocStd_Document_ptr doc;
  app->NewDocument("MDTV-XCAF", doc);
  if (doc.IsNull())
    return Status::user_error("STEP: could not create XCAF document.");

  if (!reader.Transfer(doc))
    return Status::user_error("STEP: no geometry was transferred from the file.");

  XCAFDoc_ShapeTool_ptr shapes = XCAFDoc_DocumentTool::ShapeTool(doc->Main());
  if (shapes.IsNull())
    return Status::user_error("STEP: missing XCAF shape tool.");

  NCollection_Sequence<TDF_Label> free_shapes;
  shapes->GetFreeShapes(free_shapes);
  if (free_shapes.IsEmpty())
    return Status::user_error("STEP: no valid shapes in file.");

  for (int i = 1; i <= free_shapes.Length(); ++i)
    append_tree_from_label(shapes, free_shapes.Value(i), -1, out);

  bool any_leaf = false;
  for (const Named_node& n : out)
    if (!n.is_group)
    {
      any_leaf = true;
      break;
    }

  if (!any_leaf)
    return Status::user_error("STEP: no valid shapes in file.");

  return Status::ok();
}

Status read_step_named_tree(const std::string& file_bytes, std::vector<Named_node>& out)
{
  out.clear();
  const Status xcaf = read_step_named_tree_xcaf(file_bytes, out);
  if (xcaf.is_ok())
    return xcaf;

  out.clear();
  std::vector<Named_body> flat;
  const Status            plain = read_step_named_bodies_plain(file_bytes, flat);
  if (!plain.is_ok())
    return plain;

  for (Named_body& b : flat)
  {
    Named_node n;
    n.shape        = std::move(b.shape);
    n.name         = std::move(b.name);
    n.is_group     = false;
    n.parent_index = -1;
    out.push_back(std::move(n));
  }
  return Status::ok();
}
} // namespace utl_cad_file_info
