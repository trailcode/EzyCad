#include "shp_fillet.h"
#include "occt_view.h"
#include "modes.h"

#include <BRepFilletAPI_MakeFillet.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>

Shp_fillet::Shp_fillet(Occt_view& view)
    : Shp_operation_base(view) {}

Status Shp_fillet::add_fillet(const ScreenCoords& screen_coords, const Fillet_mode fillet_mode)
{
  ShapeBase_ptr fillet_src_shp = ShapeBase_ptr::DownCast(get_shape_(screen_coords));
  if (fillet_src_shp.IsNull())
    return Status::user_error("Click on a shape.");

  BRepFilletAPI_MakeFillet fillet_maker(fillet_src_shp->Shape());

  switch (fillet_mode)
  {
    case Fillet_mode::Shape:
    {
      TopExp_Explorer edge_explorer(fillet_src_shp->Shape(), TopAbs_EDGE);
      while (edge_explorer.More())
      {
        const TopoDS_Edge& edge = TopoDS::Edge(edge_explorer.Current());
        fillet_maker.Add(m_fillet_radius, edge);
        edge_explorer.Next();
      }
      break;
    }

    case Fillet_mode::Face:
    {
      const TopoDS_Face* face = get_face_(screen_coords);
      if (!face)
        return Status::user_error("No fillet face detected.");

      TopExp_Explorer edge_explorer(*face, TopAbs_EDGE);
      while (edge_explorer.More())
      {
        const TopoDS_Edge& edge = TopoDS::Edge(edge_explorer.Current());
        fillet_maker.Add(m_fillet_radius, edge);
        edge_explorer.Next();
      }
      break;
    }

    case Fillet_mode::Wire:
    {
      const TopoDS_Wire* wire = get_wire_(screen_coords);
      if (!wire)
        return Status::user_error("No fillet wire detected.");
      
      // Fillet all edges in the wire
      TopExp_Explorer edge_explorer(*wire, TopAbs_EDGE);
      while (edge_explorer.More())
      {
        const TopoDS_Edge& wire_edge = TopoDS::Edge(edge_explorer.Current());
        fillet_maker.Add(m_fillet_radius, wire_edge);
        edge_explorer.Next();
      }

      break;
    }

    case Fillet_mode::Edge:
    {
      const TopoDS_Edge* edge = get_edge_(screen_coords);
      if (!edge)
        return Status::user_error("No fillet edge detected.");
      
      fillet_maker.Add(m_fillet_radius, *edge);
      break;
    }

    default:
      EZY_ASSERT(false);
  }

  try
  {
    fillet_maker.Build();
    FilletShp_ptr fillet_shp = new FilletShp(ctx(), fillet_maker.Shape());
    ctx().Remove(fillet_src_shp, false);
    view().m_shps.remove(fillet_src_shp);
    fillet_shp->set_name("Filleted shape");
    add_shp_(fillet_shp);
  }
  catch (const Standard_Failure& e)
  {

    auto err_str = e.GetMessageString();
    DBG_MSG(err_str);
    return Status::user_error(err_str);
  }
  catch (...)
  {
    // Investigate if this happens
    EZY_ASSERT(false);
  }

  return Status::ok();
}

void Shp_fillet::set_fillet_radius(const double radius)
{
  m_fillet_radius = radius;
}

double Shp_fillet::get_fillet_radius() const
{
  return m_fillet_radius;
}

