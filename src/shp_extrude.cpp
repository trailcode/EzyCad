#include "shp_extrude.h"

#include <BRepBuilderAPI_GTransform.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <Precision.hxx>
#include <TopoDS.hxx>
#include <cmath>
#include <gp_GTrsf.hxx>

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
    , m_unit_prism_side(Plane_side::Front)
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
  // Drag preview is wireframe; bake the shaded solid for the document (add_shp_
  // applies the default material and redisplays).
  m_extruded->set_disp_mode(AIS_Shaded);
  m_extruded->set_name(view().get_unique_shape_name("Shape"));
  add_shp_(m_extruded, true);
  view().push_undo_delta(std::make_unique<Shape_add_delta>(std::vector<Shape_rec>{capture_shape_rec(*m_extruded)}));
  ctx().Remove(m_tmp_dim, false);
  clear_all(m_to_extrude_pt, m_to_extrude, m_extruded, m_tmp_dim);
  invalidate_unit_prism_();
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
  invalidate_unit_prism_();
  view().set_show_dim_input(false);
  view().set_entered_dim(std::nullopt);

  ctx().ClearSelected(true);

  return did_cancel;
}

bool Shp_extrude::has_active_extrusion() const { return !m_extruded.IsNull(); }

bool Shp_extrude::get_both_sides() const { return m_extrude_both_sides; }

void Shp_extrude::set_both_sides(const bool both_sides) { m_extrude_both_sides = both_sides; }

bool Shp_extrude::get_shaded_preview() const { return m_shaded_preview; }

void Shp_extrude::set_shaded_preview(const bool shaded)
{
  if (m_shaded_preview == shaded)
    return;

  m_shaded_preview = shaded;
  if (m_extruded.IsNull())
    return;

  // Switch the live preview in place.
  if (shaded)
  {
    m_extruded->SetMaterial(view().get_default_material());
    view().refresh_shape_shading_(m_extruded);
  }

  m_extruded->set_disp_mode(shaded ? AIS_Shaded : AIS_WireFrame);
}

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

  // Skip redundant rebuilds when the height (and side) are effectively unchanged
  // between frames (e.g. cursor moved parallel to the plane).
  if (!m_extruded.IsNull() && m_last_preview_dist && m_unit_prism_side == side
      && m_unit_prism_both_sides == m_extrude_both_sides
      && std::fabs(extrude_dist - *m_last_preview_dist) <= Precision::Confusion())
    return;

  const gp_Vec normal_dir(m_to_extrude_pln.Axis().Direction());
  const double side_sign   = (side == Plane_side::Front) ? 1.0 : -1.0;
  const gp_Vec extrude_vec = normal_dir * (side_sign * extrude_dist);
  gp_Vec       face_offset(0.0, 0.0, 0.0);

  if (m_extrude_both_sides)
    face_offset = normal_dir * (-side_sign * (extrude_dist * 0.5));

  // Length dimension: create once, then update its endpoints in place instead of
  // removing and rebuilding the annotation every mouse move.
  const gp_Pnt dim_top(m_to_extrude_pt->XYZ() + face_offset.XYZ() + extrude_vec.XYZ());
  const gp_Pnt dim_base(m_to_extrude_pt->XYZ() + face_offset.XYZ());
  if (m_tmp_dim.IsNull())
  {
    m_tmp_dim = create_distance_annotation(dim_top, dim_base, m_curr_view_pln, gui().length_dimension_style());
    ctx().Display(m_tmp_dim, false);
  }
  else
  {
    m_tmp_dim->SetMeasuredGeometry(dim_top, dim_base, m_curr_view_pln);
    ctx().Redisplay(m_tmp_dim, false);
  }
  m_tmp_dim->SetCustomValue(extrude_dist / view().get_display_to_model_scale());

  // Body: stretch a cached height-1 prism along the plane normal (affinity) rather
  // than sweeping a fresh solid with BRepPrimAPI_MakePrism every frame. Points on
  // the sketch plane stay fixed; distance from the plane scales by extrude_dist.
  if (m_unit_prism.IsNull() || m_unit_prism_side != side || m_unit_prism_both_sides != m_extrude_both_sides)
    build_unit_prism_(side);

  gp_GTrsf affinity;
  affinity.SetAffinity(m_to_extrude_pln.Position().Ax2(), extrude_dist);
  const TopoDS_Shape body = BRepBuilderAPI_GTransform(m_unit_prism, affinity, true).Shape();

  if (m_extruded.IsNull())
  {
    m_extruded = new Shp(ctx(), body);
    // Wireframe while dragging keeps the preview cheap (no shaded remesh per move);
    // finalize() bakes the shaded solid. Options "Shaded preview" opts back in.
    const AIS_DisplayMode preview_mode = m_shaded_preview ? AIS_Shaded : AIS_WireFrame;
    m_extruded->set_disp_mode(preview_mode);
    ctx().Display(m_extruded, preview_mode, AIS_Shape::SelectionMode(TopAbs_SHAPE), false);
  }
  else
    m_extruded->Set(body);

  if (m_shaded_preview)
  {
    // Shaded preview keeps live-material behavior (Options Material row changes
    // show while dragging).
    m_extruded->SetMaterial(view().get_default_material());
    view().refresh_shape_shading_(m_extruded);
  }

  ctx().Redisplay(m_extruded, false);
  ctx().UpdateCurrentViewer();
  m_last_preview_dist = extrude_dist;
}

void Shp_extrude::build_unit_prism_(const Plane_side side)
{
  const gp_Vec normal_dir(m_to_extrude_pln.Axis().Direction());
  const double side_sign = (side == Plane_side::Front) ? 1.0 : -1.0;
  const gp_Vec unit_vec  = normal_dir * side_sign; // height 1 along the extrude side

  TopoDS_Face face = TopoDS::Face(m_to_extrude->Shape());
  if (m_extrude_both_sides)
  {
    // Center the unit prism on the sketch plane (span -0.5..+0.5) so the affinity
    // stretch yields -d/2..+d/2.
    gp_Trsf trsf;
    trsf.SetTranslation(normal_dir * (-side_sign * 0.5));
    face = TopoDS::Face(BRepBuilderAPI_Transform(face, trsf, true).Shape());
  }

  m_unit_prism            = BRepPrimAPI_MakePrism(face, unit_vec);
  m_unit_prism_side       = side;
  m_unit_prism_both_sides = m_extrude_both_sides;
}

void Shp_extrude::invalidate_unit_prism_()
{
  m_unit_prism.Nullify();
  m_last_preview_dist.reset();
}

void Shp_extrude::refresh_tmp_dimension_style(const Length_dimension_style& style)
{
  if (m_tmp_dim.IsNull())
    return;
  apply_length_dimension_style(m_tmp_dim, style);
  ctx().Redisplay(m_tmp_dim, true);
}
