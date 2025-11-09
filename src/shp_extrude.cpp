#include "shp_extrude.h"

#include <BRepPrimAPI_MakePrism.hxx>
#include <Precision.hxx>
#include <TopoDS.hxx>
#include <V3d_View.hxx>

#include "dbg.h"
#include "geom.h"
#include "gui.h"
#include "occt_view.h"
#include "sketch.h"
#include "utl.h"

Shp_extrude::Shp_extrude(Occt_view& view)
    : Shp_operation_base(view) {}

void Shp_extrude::sketch_face_extrude(const ScreenCoords& screen_coords, bool is_mouse_move)
{
  if (!m_to_extrude_pt)
  {
    if (is_mouse_move)
      return;

    //  Find face to extrude
    auto shp = view().get_shape(screen_coords);
    if (auto face = dynamic_cast<Sketch_face_shp*>(shp.get()); face)
    {
      m_to_extrude_pln = face->owner_sketch.get_plane();
      m_to_extrude_pt  = closest_to_camera(view().m_view, face->verts_3d);
      m_curr_view_pln  = view().get_view_plane(*m_to_extrude_pt);
      m_to_extrude     = shp;

      const gp_Ax1& a = m_to_extrude_pln.Axis();
      const gp_Ax1& b = m_curr_view_pln.Axis();

      if (a.IsParallel(b, to_radians(5.0)))
      {
        // Rotate view by 45 degrees (radians)
        auto rotation_axis = gp_Vec(m_to_extrude_pln.XAxis().Direction()) + gp_Vec(m_to_extrude_pln.YAxis().Direction());
        rotation_axis.Normalize();
        rotation_axis *= to_radians(45.0);
        view().m_view->Rotate(rotation_axis.X(), rotation_axis.Y(), rotation_axis.Z(), m_to_extrude_pt->X(), m_to_extrude_pt->Y(), m_to_extrude_pt->Z());  // rotate around arbitrary axis
        view().m_view->Redraw();                                                                                                                           // request redraw
        m_curr_view_pln = view().get_view_plane(*m_to_extrude_pt);
      }
      _update_extrude(screen_coords);
    }
  }
  else
    _update_extrude(screen_coords);
}

void Shp_extrude::finalize()
{
  DBG_MSG("");
  EZY_ASSERT(m_extruded);
  view().add_shp_(m_extruded);
  ctx().Remove(m_tmp_dim, false);
  clear_all(m_to_extrude_pt, m_to_extrude, m_extruded, m_tmp_dim);
  view().set_show_dim_input(false);
  view().set_entered_dim(std::nullopt);
  ctx().ClearSelected(true);

  // gui().set_mode(Mode::Normal);
}

bool Shp_extrude::cancel()
{
  ctx().Remove(m_extruded, true);
  ctx().Remove(m_tmp_dim, false);
  bool did_cancel = m_to_extrude_pt.has_value();
  clear_all(m_to_extrude_pt, m_to_extrude, m_extruded, m_tmp_dim);
  view().set_show_dim_input(false);
  view().set_entered_dim(std::nullopt);

  ctx().ClearSelected(true);

  return did_cancel;
}

bool Shp_extrude::has_active_extrusion() const
{
  return !m_extruded.IsNull();
}

void Shp_extrude::set_curr_view_pln(const gp_Pln& pln)
{
  m_curr_view_pln = pln;
}

void Shp_extrude::_update_extrude(const ScreenCoords& screen_coords)
{
  //  Extrude the face
  std::optional<gp_Pnt> p = view().pt3d_on_plane(screen_coords, m_curr_view_pln);
  if (p)  // TODO report error!
  {
    auto   extrude_vec  = gp_Vec(m_to_extrude_pln.Axis().Direction());
    double extrude_dist = m_to_extrude_pln.Distance(*p);
    if (auto entered_dim = view().get_entered_dim(); entered_dim)
      extrude_dist = *entered_dim;

    if (extrude_dist <= Precision::Confusion())
      return;

    double scaled_dist = extrude_dist / view().get_dimension_scale();

    if (view().get_show_dim_input())
    {
      auto l = [&](float new_dist, bool finalize)
      {
        if (finalize)
          view().set_show_dim_input(false);
        else
          view().set_entered_dim(new_dist * view().get_dimension_scale());
      };

      gui().set_dist_edit(float(scaled_dist), std::move(std::function<void(float, bool)>(l)), screen_coords);
    }
    if (side_of_plane(m_to_extrude_pln, *p) == Plane_side::Front)
      extrude_vec *= extrude_dist;
    else
      extrude_vec *= -extrude_dist;

    ctx().Remove(m_tmp_dim, false);
    m_tmp_dim = create_distance_annotation(gp_Pnt(m_to_extrude_pt->XYZ() + extrude_vec.XYZ()),
                                           *m_to_extrude_pt,
                                           m_curr_view_pln);
    m_tmp_dim->SetCustomValue(scaled_dist);

    ctx().Display(m_tmp_dim, false);

    const TopoDS_Face& face = TopoDS::Face(m_to_extrude->Shape());
    TopoDS_Shape       body = BRepPrimAPI_MakePrism(face, extrude_vec);
    if (!m_extruded)
    {
      m_extruded = new ExtrudedShp(ctx(), body);
      m_extruded->SetMaterial(view().m_default_material);
      ctx().Display(m_extruded, AIS_Shaded, -1, Standard_True);
    }
    else
    {
      m_extruded->Set(body);
      ctx().Redisplay(m_extruded, true);
    }
  }
}
