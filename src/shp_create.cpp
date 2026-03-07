#include "shp_create.h"

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeTorus.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <gp_Pln.hxx>
#include <gp_Vec.hxx>

namespace shp_create
{
TopoDS_Shape create_cube(double side)
{
  TopoDS_Shape box = BRepPrimAPI_MakeBox(side, side, side).Shape();
  gp_Trsf      trsf;
  trsf.SetTranslation(gp_Vec(-side / 2.0, -side / 2.0, -side / 2.0));
  return BRepBuilderAPI_Transform(box, trsf).Shape();
}

TopoDS_Shape create_pyramid(double side)
{
  const double h    = side;
  const double half = side / 2.0;

  gp_Pnt apex(0, 0, h);
  gp_Pnt p0(-half, -half, 0);
  gp_Pnt p1(half, -half, 0);
  gp_Pnt p2(half, half, 0);
  gp_Pnt p3(-half, half, 0);

  BRepBuilderAPI_MakeWire base_wire;
  base_wire.Add(BRepBuilderAPI_MakeEdge(p0, p1).Edge());
  base_wire.Add(BRepBuilderAPI_MakeEdge(p1, p2).Edge());
  base_wire.Add(BRepBuilderAPI_MakeEdge(p2, p3).Edge());
  base_wire.Add(BRepBuilderAPI_MakeEdge(p3, p0).Edge());
  TopoDS_Face base_face =
      BRepBuilderAPI_MakeFace(gp_Pln(gp_Pnt(0, 0, 0), gp_Dir(0, 0, -1)), base_wire.Wire(), true).Face();

  auto make_triangle_face = [](const gp_Pnt& a, const gp_Pnt& b, const gp_Pnt& c) -> TopoDS_Face
  {
    BRepBuilderAPI_MakeWire w;
    w.Add(BRepBuilderAPI_MakeEdge(a, b).Edge());
    w.Add(BRepBuilderAPI_MakeEdge(b, c).Edge());
    w.Add(BRepBuilderAPI_MakeEdge(c, a).Edge());
    return BRepBuilderAPI_MakeFace(w.Wire(), true).Face();
  };

  TopoDS_Face f0 = make_triangle_face(apex, p0, p1);
  TopoDS_Face f1 = make_triangle_face(apex, p1, p2);
  TopoDS_Face f2 = make_triangle_face(apex, p2, p3);
  TopoDS_Face f3 = make_triangle_face(apex, p3, p0);

  BRepBuilderAPI_Sewing sewer(1e-6);
  sewer.Add(base_face);
  sewer.Add(f0);
  sewer.Add(f1);
  sewer.Add(f2);
  sewer.Add(f3);
  sewer.Perform();

  TopoDS_Shape sewed = sewer.SewedShape();
  TopoDS_Shell shell;
  for (TopExp_Explorer exp(sewed, TopAbs_SHELL); exp.More(); exp.Next())
  {
    shell = TopoDS::Shell(exp.Current());
    break;
  }

  if (shell.IsNull())
    return TopoDS_Shape();

  BRepBuilderAPI_MakeSolid solid_maker(shell);
  if (!solid_maker.IsDone())
    return TopoDS_Shape();

  TopoDS_Shape pyramid = solid_maker.Solid();
  gp_Trsf      trsf;
  trsf.SetTranslation(gp_Vec(0, 0, -h / 2.0));
  return BRepBuilderAPI_Transform(pyramid, trsf).Shape();
}

TopoDS_Shape create_sphere(double radius)
{
  return BRepPrimAPI_MakeSphere(radius).Shape();
}

TopoDS_Shape create_cylinder(double radius, double height)
{
  TopoDS_Shape cyl = BRepPrimAPI_MakeCylinder(radius, height).Shape();
  gp_Trsf      trsf;
  trsf.SetTranslation(gp_Vec(0, 0, -height / 2.0));
  return BRepBuilderAPI_Transform(cyl, trsf).Shape();
}

TopoDS_Shape create_cone(double R1, double R2, double height)
{
  TopoDS_Shape cone = BRepPrimAPI_MakeCone(R1, R2, height).Shape();
  gp_Trsf      trsf;
  trsf.SetTranslation(gp_Vec(0, 0, -height / 2.0));
  return BRepBuilderAPI_Transform(cone, trsf).Shape();
}

TopoDS_Shape create_torus(double R1, double R2)
{
  return BRepPrimAPI_MakeTorus(R1, R2).Shape();
}
}  // namespace shp_create
