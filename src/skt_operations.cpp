// Sketch mirror/revolve and operation-axis helpers (state remains on Sketch).
#include "sketch.h"

#include <BRepPrimAPI_MakeRevol.hxx>
#include <TopoDS.hxx>
#include <set>

#include "gui.h"
#include "mode.h"
#include "skt_op_recorder.h"
#include "skt_edge.h"
#include "utl_geom.h"
#include "utl_occt.h"
#include "utl.h"

void Sketch::sketch_json_set_operation_axis_(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b)
{
  if (!unique(pt_a, pt_b))
    return;

  clear_operation_axis();

  Edge axis{m_nodes.get_node_exact(pt_a)};
  axis.node_idx_b = m_nodes.get_node_exact(pt_b);
  update_edge_shp_(axis, pt_a, pt_b);
  m_operation_axis = std::move(axis);

  sync_operation_axis_display_();
}

void Sketch::clear_operation_axis()
{
  if (m_operation_axis.has_value())
    m_ctx.Remove(m_operation_axis->shp, false);

  m_operation_axis = std::nullopt;
  m_node_marks.sync();
  m_nodes.hide_snap_annos();
}

bool Sketch::has_operation_axis() const { return m_operation_axis.has_value(); }

void Sketch::mirror_selected_edges()
{
  EZY_ASSERT(m_operation_axis.has_value());
  Sketch_op_recorder rec(m_view, *this);

  const std::vector<Edge> mirror_edges = get_selected_edges_();
  if (mirror_edges.empty())
  {
    rec.cancel();
    m_view.gui().show_message(ERROR_NO_EDGES_SELECTED);
    return;
  }

  EZY_ASSERT(!m_operation_axis->shp.IsNull());
  const auto [mirror_pt_a, mirror_pt_b] = get_edge_endpoints(m_pln, TopoDS::Edge(m_operation_axis->shp->Shape()));

  std::vector<std::pair<gp_Pnt2d, gp_Pnt2d>> mirrored_edges;
  std::set<AIS_Shape_ptr>                    mirrored_arc_shps;

  for (const Edge& e : m_edges.edges())
    for (const Edge& selected : mirror_edges)
      if (e.shp == selected.shp)
      {
        if (sketch_edge_is_arc(e))
        {
          if (!mirrored_arc_shps.count(e.shp))
          {
            mirrored_arc_shps.insert(e.shp);
            EZY_ASSERT(e.node_idx_b.has_value());
            const gp_Pnt2d arc_pt = e.node_idx_arc_pt.has_value() ? m_nodes[*e.node_idx_arc_pt]
                                                                  : arc_curve_midpoint_2d(TopoDS::Edge(e.shp->Shape()), m_pln);
            const gp_Pnt2d pt_a   = mirror_point(mirror_pt_a, mirror_pt_b, m_nodes[e.node_idx_a]);
            const gp_Pnt2d pt_b   = mirror_point(mirror_pt_a, mirror_pt_b, arc_pt);
            const gp_Pnt2d pt_c   = mirror_point(mirror_pt_a, mirror_pt_b, m_nodes[*e.node_idx_b]);
            add_arc_circle_(pt_a, pt_b, pt_c, rec);
          }
        }
        else
        {
          const gp_Pnt2d a = mirror_point(mirror_pt_a, mirror_pt_b, m_nodes[e.node_idx_a]);
          const gp_Pnt2d b = mirror_point(mirror_pt_a, mirror_pt_b, m_nodes[e.node_idx_b]);
          mirrored_edges.push_back({a, b});
        }
        break;
      }

  for (const auto& [a, b] : mirrored_edges)
    add_edge_(a, b, rec);

  m_nodes.hide_snap_annos();
  update_faces_();
  rec.commit();
}

Shp_rslt Sketch::revolve_selected(const double angle)
{
  EZY_ASSERT_MSG(m_operation_axis.has_value(), "No defined operation axis.");

  TopoDS_Compound compound;
  BRep_Builder    builder;
  builder.MakeCompound(compound);

  const std::vector<Sketch_face_shp_ptr> faces          = get_selected_faces_();
  const std::vector<Edge>                selected_edges = get_selected_edges_();
  if (selected_edges.size())
  {
    std::set<AIS_Shape*> seen;
    for (const Edge& e : selected_edges)
      if (!seen.count(e.shp.get()))
      {
        seen.insert(e.shp.get());
        builder.Add(compound, e.shp->Shape());
      }
  }
  else if (faces.size())
    for (const Sketch_face_shp_ptr& face : faces)
      builder.Add(compound, face->Shape());

  else
    return Shp_rslt(Result_status::User_error, "No selected faces or edges.");

  try
  {
    const auto [pt_a, pt_b] = get_edge_endpoints(TopoDS::Edge(m_operation_axis->shp->Shape()));

    const gp_Dir direction((pt_b.XYZ() - pt_a.XYZ()).Normalized());
    const gp_Ax1 axis(pt_a, direction);

    BRepPrimAPI_MakeRevol revolMaker(compound, axis, angle);
    return Shp_rslt(new Shp(m_ctx, try_make_solid(revolMaker.Shape())));
  }
  catch (const Standard_Failure& e)
  {
    return Shp_rslt(Result_status::Topo_error, std::string("Revolution failed: ") + standard_failure_message(e));
  }
  catch (const std::exception& e)
  {
    return Shp_rslt(Result_status::User_error, "Unexpected error: " + std::string(e.what()));
  }
}

bool Sketch::show_operation_axis_() const
{
  return m_operation_axis.has_value() && m_visible && is_current() && get_mode() == Mode::Sketch_operation_axis;
}

bool Sketch::operation_axis_suppresses_sketch_snap_() const
{
  return get_mode() == Mode::Sketch_operation_axis && m_operation_axis.has_value();
}

void Sketch::sync_operation_axis_display_()
{
  if (!m_operation_axis.has_value())
    return;

  if (show_operation_axis_())
    m_ctx.Display(m_operation_axis->shp, AIS_WireFrame, 0, false);
  else
    m_ctx.Erase(m_operation_axis->shp, false);
}
