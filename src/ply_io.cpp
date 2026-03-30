#include "ply_io.h"

#include <cctype>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>

#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <Poly_Triangulation.hxx>
#include <Poly_Triangle.hxx>
#include <Precision.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Pnt.hxx>

namespace
{
void trim_inplace(std::string& s)
{
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
    s.pop_back();
  size_t i = 0;
  while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i])))
    ++i;
  s.erase(0, i);
}

bool iequals(const std::string& a, const std::string& b)
{
  if (a.size() != b.size())
    return false;
  for (size_t i = 0; i < a.size(); ++i)
    if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i])))
      return false;
  return true;
}

enum class ScalarType
{
  Int8,
  UInt8,
  Int16,
  UInt16,
  Int32,
  UInt32,
  Float32,
  Float64,
  Unknown
};

ScalarType scalar_from_token(const std::string& t)
{
  if (t == "char" || t == "int8")
    return ScalarType::Int8;
  if (t == "uchar" || t == "uint8")
    return ScalarType::UInt8;
  if (t == "short" || t == "int16")
    return ScalarType::Int16;
  if (t == "ushort" || t == "uint16")
    return ScalarType::UInt16;
  if (t == "int" || t == "int32")
    return ScalarType::Int32;
  if (t == "uint" || t == "uint32")
    return ScalarType::UInt32;
  if (t == "float" || t == "float32")
    return ScalarType::Float32;
  if (t == "double" || t == "float64")
    return ScalarType::Float64;
  return ScalarType::Unknown;
}

int size_of_scalar(ScalarType t)
{
  switch (t)
  {
    case ScalarType::Int8:
    case ScalarType::UInt8:
      return 1;
    case ScalarType::Int16:
    case ScalarType::UInt16:
      return 2;
    case ScalarType::Int32:
    case ScalarType::UInt32:
    case ScalarType::Float32:
      return 4;
    case ScalarType::Float64:
      return 8;
    default:
      return 0;
  }
}

struct ScalarProp
{
  ScalarType  type {ScalarType::Unknown};
  std::string name;
};

struct ListProp
{
  ScalarType  count_type {ScalarType::Unknown};
  ScalarType  value_type {ScalarType::Unknown};
  std::string name;
};

struct ElementDesc
{
  std::string             name;
  int                     count {0};
  std::vector<ScalarProp> scalars;
  std::vector<ListProp>   lists;
};

bool read_scalar_bin_at(const unsigned char* base,
                        size_t               off,
                        const unsigned char* endbuf,
                        ScalarType           t,
                        double&              out)
{
  const unsigned char* p = base + off;
  if (p >= endbuf)
    return false;
  auto need = [&](size_t n) -> bool { return static_cast<size_t>(endbuf - p) >= n; };
  switch (t)
  {
    case ScalarType::Int8:
    {
      if (!need(1))
        return false;
      out = static_cast<double>(*reinterpret_cast<const std::int8_t*>(p));
      return true;
    }
    case ScalarType::UInt8:
    {
      if (!need(1))
        return false;
      out = static_cast<double>(*p);
      return true;
    }
    case ScalarType::Int16:
    {
      if (!need(2))
        return false;
      std::int16_t v;
      std::memcpy(&v, p, 2);
      out = static_cast<double>(v);
      return true;
    }
    case ScalarType::UInt16:
    {
      if (!need(2))
        return false;
      std::uint16_t v;
      std::memcpy(&v, p, 2);
      out = static_cast<double>(v);
      return true;
    }
    case ScalarType::Int32:
    {
      if (!need(4))
        return false;
      std::int32_t v;
      std::memcpy(&v, p, 4);
      out = static_cast<double>(v);
      return true;
    }
    case ScalarType::UInt32:
    {
      if (!need(4))
        return false;
      std::uint32_t v;
      std::memcpy(&v, p, 4);
      out = static_cast<double>(v);
      return true;
    }
    case ScalarType::Float32:
    {
      if (!need(4))
        return false;
      float v;
      std::memcpy(&v, p, 4);
      out = static_cast<double>(v);
      return true;
    }
    case ScalarType::Float64:
    {
      if (!need(8))
        return false;
      std::memcpy(&out, p, 8);
      return true;
    }
    default:
      return false;
  }
}

std::uint32_t read_list_count_bin(const unsigned char*& p, const unsigned char* end, ScalarType ct)
{
  switch (ct)
  {
    case ScalarType::UInt8:
    {
      if (static_cast<size_t>(end - p) < 1)
        return 0;
      std::uint32_t v = *p;
      ++p;
      return v;
    }
    case ScalarType::UInt16:
    {
      if (static_cast<size_t>(end - p) < 2)
        return 0;
      std::uint16_t v;
      std::memcpy(&v, p, 2);
      p += 2;
      return v;
    }
    case ScalarType::UInt32:
    {
      if (static_cast<size_t>(end - p) < 4)
        return 0;
      std::uint32_t v;
      std::memcpy(&v, p, 4);
      p += 4;
      return v;
    }
    default:
      return 0;
  }
}

bool read_face_indices_bin(const unsigned char*& p,
                           const unsigned char* end,
                           ScalarType           vt,
                           std::uint32_t        n,
                           std::vector<int>&    idx_out)
{
  idx_out.clear();
  idx_out.reserve(n);
  for (std::uint32_t i = 0; i < n; ++i)
  {
    double d = 0;
    if (!read_scalar_bin_at(p, 0, end, vt, d))
      return false;
    int sz = size_of_scalar(vt);
    if (sz <= 0 || static_cast<size_t>(end - p) < static_cast<size_t>(sz))
      return false;
    p += static_cast<size_t>(sz);
    idx_out.push_back(static_cast<int>(d));
  }
  return true;
}

bool append_triangle(TopoDS_Compound& comp,
                     BRep_Builder&    bb,
                     const gp_Pnt&    p0,
                     const gp_Pnt&    p1,
                     const gp_Pnt&    p2,
                     int&             ntri)
{
  if (p0.IsEqual(p1, Precision::Confusion()) || p1.IsEqual(p2, Precision::Confusion()) ||
      p2.IsEqual(p0, Precision::Confusion()))
    return true;

  BRepBuilderAPI_MakePolygon poly;
  poly.Add(p0);
  poly.Add(p1);
  poly.Add(p2);
  poly.Close();
  if (!poly.IsDone())
    return true;
  BRepBuilderAPI_MakeFace face(poly.Wire(), true);
  if (!face.IsDone())
    return true;
  bb.Add(comp, face.Shape());
  ++ntri;
  return true;
}

}  // namespace

Status import_ply_shape(const std::string& file_bytes, TopoDS_Shape& out_shape)
{
  out_shape.Nullify();

  if (file_bytes.size() < 16)
    return Status::user_error("PLY: file too small.");

  size_t      pos = 0;
  std::string line;
  std::string format_mode;

  auto read_line = [&]() -> void
  {
    line.clear();
    while (pos < file_bytes.size())
    {
      char c = file_bytes[pos++];
      if (c == '\n')
        break;
      if (c != '\r')
        line.push_back(c);
    }
    trim_inplace(line);
  };

  read_line();
  if (line != "ply")
    return Status::user_error("PLY: missing 'ply' magic.");

  std::vector<ElementDesc> elements;
  ElementDesc*             cur_el = nullptr;

  while (pos < file_bytes.size())
  {
    read_line();
    if (line.empty())
      continue;
    if (line == "end_header")
      break;
    if (line.rfind("comment ", 0) == 0)
      continue;

    std::istringstream iss(line);
    std::string        kw;
    iss >> kw;
    if (kw == "format")
    {
      iss >> format_mode;
      std::string ver;
      iss >> ver;
      if (format_mode != "ascii" && format_mode != "binary_little_endian" && format_mode != "binary_big_endian")
        return Status::user_error("PLY: unknown format line.");
    }
    else if (kw == "element")
    {
      std::string name;
      int         cnt = 0;
      iss >> name >> cnt;
      elements.push_back(ElementDesc {});
      cur_el        = &elements.back();
      cur_el->name  = std::move(name);
      cur_el->count = cnt;
    }
    else if (kw == "property" && cur_el != nullptr)
    {
      std::string t1;
      iss >> t1;
      if (t1 == "list")
      {
        std::string ctok, vtok, pname;
        iss >> ctok >> vtok >> pname;
        ListProp lp;
        lp.count_type = scalar_from_token(ctok);
        lp.value_type = scalar_from_token(vtok);
        lp.name       = std::move(pname);
        const bool count_ok =
            lp.count_type == ScalarType::UInt8 || lp.count_type == ScalarType::UInt16 || lp.count_type == ScalarType::UInt32;
        const bool val_ok = lp.value_type == ScalarType::Int32 || lp.value_type == ScalarType::UInt32;
        if (!count_ok || !val_ok)
          return Status::user_error("PLY: unsupported face list property types.");
        cur_el->lists.push_back(std::move(lp));
      }
      else
      {
        std::string pname;
        iss >> pname;
        ScalarProp sp;
        sp.type = scalar_from_token(t1);
        sp.name = std::move(pname);
        if (sp.type == ScalarType::Unknown || size_of_scalar(sp.type) <= 0)
          return Status::user_error("PLY: unsupported vertex property type.");
        cur_el->scalars.push_back(std::move(sp));
      }
    }
  }

  if (format_mode.empty())
    return Status::user_error("PLY: missing format.");
  if (format_mode == "binary_big_endian")
    return Status::user_error("PLY: binary_big_endian is not supported.");

  const ElementDesc* vert_el = nullptr;
  const ElementDesc* face_el = nullptr;
  for (const ElementDesc& e : elements)
  {
    if (e.name == "vertex")
      vert_el = &e;
    else if (e.name == "face")
      face_el = &e;
  }
  if (vert_el == nullptr || vert_el->count <= 0)
    return Status::user_error("PLY: no vertex element.");
  if (face_el == nullptr || face_el->lists.empty())
    return Status::user_error("PLY: no face list (need triangulated mesh).");
  if (face_el->lists.size() != 1)
    return Status::user_error("PLY: only one list property per face is supported.");

  size_t     off_x = 0, off_y = 0, off_z = 0;
  size_t     vstride = 0;
  ScalarType tx = ScalarType::Unknown, ty = ScalarType::Unknown, tz = ScalarType::Unknown;

  {
    size_t o = 0;
    for (const ScalarProp& sp : vert_el->scalars)
    {
      const int sz = size_of_scalar(sp.type);
      if (iequals(sp.name, "x"))
      {
        off_x = o;
        tx    = sp.type;
      }
      else if (iequals(sp.name, "y"))
      {
        off_y = o;
        ty    = sp.type;
      }
      else if (iequals(sp.name, "z"))
      {
        off_z = o;
        tz    = sp.type;
      }
      o += static_cast<size_t>(sz);
    }
    vstride = o;
    if (vstride == 0 || off_x >= vstride || off_y >= vstride || off_z >= vstride)
      return Status::user_error("PLY: vertex x/y/z properties not found.");
    if (tx == ScalarType::Unknown || ty == ScalarType::Unknown || tz == ScalarType::Unknown)
      return Status::user_error("PLY: vertex x/y/z types invalid.");
  }

  const ListProp& face_list = face_el->lists.front();

  std::vector<gp_Pnt> verts(static_cast<size_t>(vert_el->count));
  TopoDS_Compound     comp;
  BRep_Builder        bb;
  bb.MakeCompound(comp);
  int ntri = 0;

  if (format_mode == "ascii")
  {
    std::istringstream body(file_bytes.substr(pos));
    for (size_t vi = 0; vi < verts.size(); ++vi)
    {
      std::string vl;
      if (!std::getline(body, vl))
        return Status::user_error("PLY: unexpected EOF in vertices.");
      trim_inplace(vl);
      std::istringstream ls(vl);
      double             x = 0, y = 0, z = 0;
      for (const ScalarProp& sp : vert_el->scalars)
      {
        if (sp.type == ScalarType::Float32 || sp.type == ScalarType::Float64)
        {
          double d = 0;
          if (!(ls >> d))
            return Status::user_error("PLY: bad vertex data.");
          if (iequals(sp.name, "x"))
            x = d;
          else if (iequals(sp.name, "y"))
            y = d;
          else if (iequals(sp.name, "z"))
            z = d;
        }
        else if (sp.type == ScalarType::Int8 || sp.type == ScalarType::UInt8 || sp.type == ScalarType::Int16 ||
                 sp.type == ScalarType::UInt16 || sp.type == ScalarType::Int32 || sp.type == ScalarType::UInt32)
        {
          long long v = 0;
          if (!(ls >> v))
            return Status::user_error("PLY: bad vertex data.");
          const double d = static_cast<double>(v);
          if (iequals(sp.name, "x"))
            x = d;
          else if (iequals(sp.name, "y"))
            y = d;
          else if (iequals(sp.name, "z"))
            z = d;
        }
        else
          return Status::user_error("PLY: unsupported vertex property for ASCII.");
      }
      verts[vi] = gp_Pnt(x, y, z);
    }

    for (int fi = 0; fi < face_el->count; ++fi)
    {
      std::string fl;
      if (!std::getline(body, fl))
        return Status::user_error("PLY: unexpected EOF in faces.");
      trim_inplace(fl);
      std::istringstream fs(fl);
      int                nidx = 0;
      fs >> nidx;
      if (nidx != 3)
        return Status::user_error("PLY: only triangular faces are supported.");
      int i0 = 0, i1 = 0, i2 = 0;
      fs >> i0 >> i1 >> i2;
      if (i0 < 0 || i1 < 0 || i2 < 0)
        return Status::user_error("PLY: negative vertex index.");
      const size_t su0 = static_cast<size_t>(i0);
      const size_t su1 = static_cast<size_t>(i1);
      const size_t su2 = static_cast<size_t>(i2);
      if (su0 >= verts.size() || su1 >= verts.size() || su2 >= verts.size())
        return Status::user_error("PLY: face vertex index out of range.");
      append_triangle(comp, bb, verts[su0], verts[su1], verts[su2], ntri);
    }
    if (ntri == 0)
      return Status::user_error("PLY: no valid triangles.");
    out_shape = comp;
    return Status::ok();
  }

  // binary_little_endian
  const unsigned char* p   = reinterpret_cast<const unsigned char*>(file_bytes.data()) + pos;
  const unsigned char* end = reinterpret_cast<const unsigned char*>(file_bytes.data()) + file_bytes.size();

  for (size_t vi = 0; vi < verts.size(); ++vi)
  {
    if (static_cast<size_t>(end - p) < vstride)
      return Status::user_error("PLY: truncated vertex data.");
    double x = 0, y = 0, z = 0;
    if (!read_scalar_bin_at(p, off_x, end, tx, x) || !read_scalar_bin_at(p, off_y, end, ty, y) ||
        !read_scalar_bin_at(p, off_z, end, tz, z))
      return Status::user_error("PLY: bad binary vertex.");
    verts[vi] = gp_Pnt(x, y, z);
    p += vstride;
  }

  for (int fi = 0; fi < face_el->count; ++fi)
  {
    if (static_cast<size_t>(end - p) < 1u)
      return Status::user_error("PLY: truncated face data.");
    const std::uint32_t n = read_list_count_bin(p, end, face_list.count_type);
    if (n != 3u)
      return Status::user_error("PLY: only triangular faces are supported.");
    std::vector<int> idx;
    if (!read_face_indices_bin(p, end, face_list.value_type, n, idx) || idx.size() != 3u)
      return Status::user_error("PLY: bad face indices.");

    const size_t su0 = static_cast<size_t>(idx[0]);
    const size_t su1 = static_cast<size_t>(idx[1]);
    const size_t su2 = static_cast<size_t>(idx[2]);
    if (su0 >= verts.size() || su1 >= verts.size() || su2 >= verts.size())
      return Status::user_error("PLY: face vertex index out of range.");
    append_triangle(comp, bb, verts[su0], verts[su1], verts[su2], ntri);
  }

  if (ntri == 0)
    return Status::user_error("PLY: no valid triangles.");
  out_shape = comp;
  return Status::ok();
}

Status export_ply_binary_file(const TopoDS_Shape& shape, const std::string& file_path)
{
  struct Tri
  {
    double x0, y0, z0, x1, y1, z1, x2, y2, z2;
  };
  std::vector<Tri> tris;

  for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next())
  {
    const TopoDS_Face&         face = TopoDS::Face(exp.Current());
    TopLoc_Location            loc;
    const Handle(Poly_Triangulation)& tri = BRep_Tool::Triangulation(face, loc);
    if (tri.IsNull())
      continue;

    // OCCT 7.9+: Poly_Triangulation has Node(i) and Triangle(i), not Nodes()/Triangles().
    const gp_Trsf& tr = loc.Transformation();
    for (Standard_Integer ti = 1, nbt = tri->NbTriangles(); ti <= nbt; ++ti)
    {
      const Poly_Triangle& tab = tri->Triangle(ti);
      Standard_Integer     i1 = 0, i2 = 0, i3 = 0;
      tab.Get(i1, i2, i3);
      const gp_Pnt p1 = tri->Node(i1).Transformed(tr);
      const gp_Pnt p2 = tri->Node(i2).Transformed(tr);
      const gp_Pnt p3 = tri->Node(i3).Transformed(tr);
      tris.push_back(Tri {p1.X(), p1.Y(), p1.Z(), p2.X(), p2.Y(), p2.Z(), p3.X(), p3.Y(), p3.Z()});
    }
  }

  if (tris.empty())
    return Status::user_error("PLY: no mesh data (tessellate the shape first).");

  const size_t nv = tris.size() * 3;
  const size_t nf = tris.size();

  std::ostringstream hdr;
  hdr << "ply\nformat binary_little_endian 1.0\n"
      << "comment EzyCad export\n"
      << "element vertex " << nv << "\n"
      << "property double x\n"
      << "property double y\n"
      << "property double z\n"
      << "element face " << nf << "\n"
      << "property list uchar int vertex_indices\n"
      << "end_header\n";
  const std::string hdr_str = hdr.str();

  std::ofstream out(file_path, std::ios::binary);
  if (!out)
    return Status::user_error("PLY: could not open file for writing.");

  out.write(hdr_str.data(), static_cast<std::streamsize>(hdr_str.size()));
  for (const Tri& t : tris)
  {
    const double v[9] = {t.x0, t.y0, t.z0, t.x1, t.y1, t.z1, t.x2, t.y2, t.z2};
    out.write(reinterpret_cast<const char*>(v), sizeof(v));
  }
  for (size_t fi = 0; fi < nf; ++fi)
  {
    const unsigned char three = 3;
    const std::int32_t i0 = static_cast<std::int32_t>(fi * 3);
    const std::int32_t i1 = static_cast<std::int32_t>(fi * 3 + 1);
    const std::int32_t i2 = static_cast<std::int32_t>(fi * 3 + 2);
    out.write(reinterpret_cast<const char*>(&three), 1);
    out.write(reinterpret_cast<const char*>(&i0), 4);
    out.write(reinterpret_cast<const char*>(&i1), 4);
    out.write(reinterpret_cast<const char*>(&i2), 4);
  }

  if (!out.good())
    return Status::user_error("PLY: error writing file.");
  return Status::ok();
}
