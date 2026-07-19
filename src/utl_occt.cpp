#include "utl_occt.h"

#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <Precision.hxx>
#include <Standard_Failure.hxx>
#include <Standard_Version.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopoDS_Shell.hxx>

const char* standard_failure_message(const Standard_Failure& e)
{
#if OCC_VERSION_HEX >= 0x080000
  const char* msg = e.what();
#else
  const char* msg = e.GetMessageString();
#endif
  return (msg && msg[0]) ? msg : "Unknown OCCT error";
}

namespace
{
TopoDS_Shape solid_from_shell(const TopoDS_Shell& shell)
{
  if (shell.IsNull())
    return TopoDS_Shape();

  BRepBuilderAPI_MakeSolid maker(shell);
  if (!maker.IsDone())
    return TopoDS_Shape();

  TopoDS_Shape solid = maker.Solid();
  return solid;
}

TopoDS_Shape solids_from_shells(const TopoDS_Shape& shape)
{
  TopoDS_Compound out;
  BRep_Builder    builder;
  builder.MakeCompound(out);

  int solid_count = 0;
  for (TopExp_Explorer exp(shape, TopAbs_SHELL); exp.More(); exp.Next())
  {
    const TopoDS_Shape solid = solid_from_shell(TopoDS::Shell(exp.Current()));
    if (!solid.IsNull())
    {
      builder.Add(out, solid);
      ++solid_count;
    }
  }

  if (solid_count == 0)
    return TopoDS_Shape();

  if (solid_count == 1)
  {
    for (TopExp_Explorer exp(out, TopAbs_SOLID); exp.More(); exp.Next())
      return exp.Current();
  }

  return out;
}

TopoDS_Shape solid_from_sewn_faces(const TopoDS_Shape& shape)
{
  BRepBuilderAPI_Sewing sewer(Precision::Confusion());
  int                   face_count = 0;
  for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next())
  {
    sewer.Add(exp.Current());
    ++face_count;
  }

  if (face_count == 0)
    return TopoDS_Shape();

  sewer.Perform();
  const TopoDS_Shape sewed = sewer.SewedShape();
  if (sewed.IsNull())
    return TopoDS_Shape();

  if (sewed.ShapeType() == TopAbs_SHELL)
    return solid_from_shell(TopoDS::Shell(sewed));

  return solids_from_shells(sewed);
}
} // namespace

TopoDS_Shape try_make_solid(const TopoDS_Shape& shape)
{
  if (shape.IsNull())
    return shape;

  switch (shape.ShapeType())
  {
  case TopAbs_SOLID:
  case TopAbs_COMPSOLID:
    return shape;

  case TopAbs_SHELL:
  {
    const TopoDS_Shape solid = solid_from_shell(TopoDS::Shell(shape));
    return solid.IsNull() ? shape : solid;
  }

  case TopAbs_COMPOUND:
  {
    int          solid_count = 0;
    TopoDS_Shape lone_solid;
    for (TopExp_Explorer exp(shape, TopAbs_SOLID); exp.More(); exp.Next())
    {
      lone_solid = exp.Current();
      ++solid_count;
    }
    if (solid_count == 1)
      return lone_solid;

    const TopoDS_Shape from_shells = solids_from_shells(shape);
    if (!from_shells.IsNull())
      return from_shells;

    const TopoDS_Shape from_faces = solid_from_sewn_faces(shape);
    if (!from_faces.IsNull())
      return from_faces;

    return shape;
  }

  default:
    return shape;
  }
}

void append_cad_import_bodies(const TopoDS_Shape& shape, std::vector<TopoDS_Shape>& out)
{
  if (shape.IsNull())
    return;

  switch (shape.ShapeType())
  {
  case TopAbs_COMPOUND:
  {
    // STEP assemblies usually transfer as one compound with many nested solids.
    bool has_solid = false;
    for (TopExp_Explorer exp(shape, TopAbs_SOLID); exp.More(); exp.Next())
    {
      has_solid = true;
      out.push_back(exp.Current());
    }
    if (has_solid)
      return;

    bool has_shell = false;
    for (TopExp_Explorer exp(shape, TopAbs_SHELL); exp.More(); exp.Next())
    {
      has_shell = true;
      out.push_back(exp.Current());
    }
    if (has_shell)
      return;

    for (TopoDS_Iterator it(shape); it.More(); it.Next())
      append_cad_import_bodies(it.Value(), out);
    return;
  }

  case TopAbs_COMPSOLID:
    for (TopExp_Explorer exp(shape, TopAbs_SOLID); exp.More(); exp.Next())
      out.push_back(exp.Current());
    return;

  default:
    out.push_back(shape);
    return;
  }
}
