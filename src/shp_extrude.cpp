#include "shp_extrude.h"

#include <BRepBuilderAPI_Transform.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <Precision.hxx>
#include <TopoDS.hxx>

#include "utl_dbg.h"
#include "utl_geom.h"
#include "gui.h"
#include "gui_occt_view.h"
#include "skt.h"
#include "shp_delta.h"
#include "utl.h"

Shp_extrude::Shp_extrude(Occt_view& view)
    : Shp_operation_base(view)
    , m_extrude_side(Plane_side::Front)
{
}

bool Shp_extrude::begin_face_extrude(const AIS_Shape_ptr& shp)
{
  auto* face = dynamic_cast<Sketch_face_shp*>(shp.get());
  if (!face || face->verts_3d.empty())
    return false;

  cancel();

  m_to_extrude_pln = face->owner_sketch.get_plane();
  m_extrude_side   = Plane_side::Front;
  m_to_extrude_pt  = closest_to_camera(view().view_handle(), face->verts_3d);
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
    // rotate around arbitrary axis (use clean view API)
    view().rotate_view(rotation_axis, *m_to_extrude_pt);
    view().redraw_view();
    m_curr_view_pln = view().get_view_plane(*m_to_extrude_pt);
  }

  return true;
}

void Shp_extrude::sketch_face_extrude(const ScreenCoords& screen_coords, bool is_mouse_move)
{
  if (!m_to_extrude_pt)
  {
    if (is_mouse_move)
      return;

    //  Find face to extrude
    auto shp = view().get_shape(screen_coords);
    if (begin_face_extrude(shp))
      _update_extrude(screen_coords);
  }
  else
    _update_extrude(screen_coords);
}

void Shp_extrude::finalize()
{
  DBG_MSG("");
  EZY_ASSERT(m_extruded);
  m_extruded->set_name(view().get_unique_shape_name("Shape"));
  add_shp_(m_extruded, true);
  view().push_undo_delta(std::make_unique<Shape_add_delta>(std::vector<Shape_rec>{capture_shape_rec(*m_extruded)}));
  ctx().Remove(m_tmp_dim, false);
  clear_all(m_to_extrude_pt, m_to_extrude, m_extruded, m_tmp_dim);
  view().set_show_dim_input(false);
  view().set_entered_dim(std::nullopt);
  ctx().ClearSelected(true);
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

bool Shp_extrude::has_active_extrusion() const { return !m_extruded.IsNull(); }

bool Shp_extrude::get_both_sides() const { return m_extrude_both_sides; }

void Shp_extrude::set_both_sides(const bool both_sides) { m_extrude_both_sides = both_sides; }

void Shp_extrude::_update_extrude(const ScreenCoords& screen_coords)
{
  //  Extrude the face
  std::optional<gp_Pnt> p = view().pt3d_on_plane(screen_coords, m_curr_view_pln);
  if (p) // TODO report error!
  {
    double extrude_dist = m_to_extrude_pln.Distance(*p);
    if (auto entered_dim = view().get_entered_dim(); entered_dim)
      extrude_dist = *entered_dim;

    if (extrude_dist <= Precision::Confusion())
      return;

    const Plane_side cursor_side = side_of_plane(m_to_extrude_pln, *p);
    if (cursor_side != Plane_side::On)
      m_extrude_side = cursor_side;

    double scaled_dist = extrude_dist / view().get_display_to_model_scale();

    if (view().get_show_dim_input())
    {
      auto l = [this](float new_dist, bool do_finalize)
      {
        const double entered_dist = static_cast<double>(new_dist) * view().get_display_to_model_scale();
        view().set_entered_dim(entered_dist);
        if (do_finalize)
        {
          view().set_show_dim_input(false);
          update_extrude_preview_(entered_dist, m_extrude_side);
          finalize();
        }
      };

      gui().set_dist_edit(float(scaled_dist), std::move(std::function<void(float, bool)>(l)), screen_coords);
    }

    update_extrude_preview_(extrude_dist, m_extrude_side);
  }
}

void Shp_extrude::update_extrude_preview_(const double extrude_dist, const Plane_side side)
{
  if (extrude_dist <= Precision::Confusion())
    return;

  EZY_ASSERT(side != Plane_side::On);

  const gp_Vec normal_dir(m_to_extrude_pln.Axis().Direction());
  const double side_sign   = (side == Plane_side::Front) ? 1.0 : -1.0;
  const gp_Vec extrude_vec = normal_dir * (side_sign * extrude_dist);
  gp_Vec       face_offset(0.0, 0.0, 0.0);

  if (m_extrude_both_sides)
    face_offset = normal_dir * (-side_sign * (extrude_dist * 0.5));

  ctx().Remove(m_tmp_dim, false);
  m_tmp_dim = create_distance_annotation(gp_Pnt(m_to_extrude_pt->XYZ() + face_offset.XYZ() + extrude_vec.XYZ()),
                                         gp_Pnt(m_to_extrude_pt->XYZ() + face_offset.XYZ()), m_curr_view_pln,
                                         gui().length_dimension_style());

  m_tmp_dim->SetCustomValue(extrude_dist / view().get_display_to_model_scale());

  ctx().Display(m_tmp_dim, false);

  TopoDS_Face face = TopoDS::Face(m_to_extrude->Shape());
  if (m_extrude_both_sides)
  {
    gp_Trsf trsf;
    trsf.SetTranslation(face_offset);
    TopoDS_Shape shifted_face = BRepBuilderAPI_Transform(face, trsf, true).Shape();
    face                      = TopoDS::Face(shifted_face);
  }

  TopoDS_Shape body = BRepPrimAPI_MakePrism(face, extrude_vec);
  if (m_extruded.IsNull())
  {
    m_extruded = new Shp(ctx(), body);
    ctx().Display(m_extruded, AIS_Shaded, AIS_Shape::SelectionMode(TopAbs_SHAPE), true);
  }
  else
    m_extruded->Set(body);

  m_extruded->SetMaterial(view().get_default_material());
  view().refresh_shape_shading_(m_extruded);
  ctx().Redisplay(m_extruded, true);
  ctx().UpdateCurrentViewer();
}

void Shp_extrude::refresh_tmp_dimension_style(const Length_dimension_style& style)
{
  if (m_tmp_dim.IsNull())
    return;
  apply_length_dimension_style(m_tmp_dim, style);
  ctx().Redisplay(m_tmp_dim, true);
}
