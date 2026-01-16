#include "shp_chamfer.h"
#include "occt_view.h"
#include "modes.h"

#include <BRepFilletAPI_MakeChamfer.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>

Shp_chamfer::Shp_chamfer(Occt_view& view)
    : Shp_operation_base(view) {}

Status Shp_chamfer::add_chamfer(const ScreenCoords& screen_coords, const Chamfer_mode chamfer_mode)
{
  ShapeBase_ptr chamfer_src_shp = ShapeBase_ptr::DownCast(get_shape_(screen_coords));
  if (chamfer_src_shp.IsNull())
    return Status::user_error("Click on a shape.");

  BRepFilletAPI_MakeChamfer chamfer_maker(chamfer_src_shp->Shape());

  switch (chamfer_mode)
  {
    case Chamfer_mode::Shape:
    {
      TopExp_Explorer edge_explorer(chamfer_src_shp->Shape(), TopAbs_EDGE);
      while (edge_explorer.More())
      {
        const TopoDS_Edge& edge = TopoDS::Edge(edge_explorer.Current());
        chamfer_maker.Add(m_chamfer_dist, edge);
        edge_explorer.Next();
      }
      break;
    }

    case Chamfer_mode::Face:
    {
      const TopoDS_Face* face = get_face_(screen_coords);
      if (!face)
        return Status::user_error("No chamfer face detected.");

      TopExp_Explorer edge_explorer(*face, TopAbs_EDGE);
      while (edge_explorer.More())
      {
        const TopoDS_Edge& edge = TopoDS::Edge(edge_explorer.Current());
        chamfer_maker.Add(m_chamfer_dist, edge);
        edge_explorer.Next();
      }
      break;
    }

    case Chamfer_mode::Wire:
    {
      const TopoDS_Wire* wire = get_wire_(screen_coords);
      if (!wire)
        return Status::user_error("No chamfer wire detected.");
      
      // Chamfer all edges in the wire
      TopExp_Explorer edge_explorer(*wire, TopAbs_EDGE);
      while (edge_explorer.More())
      {
        const TopoDS_Edge& wire_edge = TopoDS::Edge(edge_explorer.Current());
        chamfer_maker.Add(m_chamfer_dist, wire_edge);
        edge_explorer.Next();
      }

      break;
    }

    case Chamfer_mode::Edge:
    {
      const TopoDS_Edge* edge = get_edge_(screen_coords);
      if (!edge)
        return Status::user_error("No chamfer edge detected.");
      
      chamfer_maker.Add(m_chamfer_dist, *edge);
      break;
    }

    default:
      EZY_ASSERT(false);
  }

  try
  {
    chamfer_maker.Build();
    chamfer_shp_ptr chamfer_shp = new Chamfer_shp(ctx(), chamfer_maker.Shape());
    ctx().Remove(chamfer_src_shp, false);
    view().m_shps.remove(chamfer_src_shp);
    chamfer_shp->set_name("Chamfered shape");
    add_shp_(chamfer_shp);
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

void Shp_chamfer::set_chamfer_dist(const double dist)
{
  m_chamfer_dist = dist;
}

double Shp_chamfer::get_chamfer_dist() const
{
  return m_chamfer_dist;
}