#include "shp_extrude.h"

#include <AIS_Shape.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <Precision.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <cmath>
#include <gp_Trsf.hxx>

#include "utl_dbg.h"
#include "utl_geom.h"
#include "gui.h"
#include "gui_occt_view.h"
#include "skt.h"
#include "shp_delta.h"
#include "utl.h"

namespace
{
size_t count_shape_edges_(const TopoDS_Shape& shape)
{
  size_t n = 0;
  for (TopExp_Explorer ex(shape, TopAbs_EDGE); ex.More(); ex.Next())
    ++n;
  return n;
}
} // namespace

Shp_extrude::Shp_extrude(Occt_view& view)
    : Shp_operation_base(view)
    , m_extrude_side(Plane_side::Front)
    , m_last_preview_side(Plane_side::Front)
{
}

bool Shp_extrude::begin_face_extrude(const AIS_Shape_ptr& shp)
{
  auto* face = dynamic_cast<Sketch_face_shp*>(shp.get());
  if (!face || face->verts_3d.empty())
    return false;

  cancel();

  m_to_extrude_pln      = face->owner_sketch.get_plane();
  m_extrude_side        = Plane_side::Front;
  m_to_extrude_pt       = closest_to_camera(view().view_handle(), face->verts_3d);
  m_curr_view_pln       = view().get_view_plane(*m_to_extrude_pt);
  m_to_extrude          = shp;
  m_face_edge_count     = count_shape_edges_(shp->Shape());
  m_lite_preview_active = false;

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
  EZY_ASSERT(m_last_preview_dist);

  // Lite preview only translated face copies. Always bake a shaded MakePrism solid.
  clear_lite_other_face_();
  const TopoDS_Shape body = make_prism_body_(*m_last_preview_dist, m_extrude_side);
  m_extruded->ResetTransformation();
  m_extruded->Set(body);
  m_extruded->set_disp_mode(AIS_Shaded);
  m_extruded->set_name(view().get_unique_shape_name("Shape"));
  add_shp_(m_extruded, true);
  view().push_undo_delta(std::make_unique<Shape_add_delta>(std::vector<Shape_rec>{capture_shape_rec(*m_extruded)}));
  ctx().Remove(m_tmp_dim, false);
  clear_all(m_to_extrude_pt, m_to_extrude, m_extruded, m_tmp_dim);
  clear_preview_();
  view().set_show_dim_input(false);
  view().set_entered_dim(std::nullopt);
  ctx().ClearSelected(true);
}

bool Shp_extrude::cancel()
{
  clear_lite_other_face_();
  ctx().Remove(m_extruded, true);
  ctx().Remove(m_tmp_dim, false);
  bool did_cancel = m_to_extrude_pt.has_value();
  clear_all(m_to_extrude_pt, m_to_extrude, m_extruded, m_tmp_dim);
  clear_preview_();
  view().set_show_dim_input(false);
  view().set_entered_dim(std::nullopt);

  ctx().ClearSelected(true);

  return did_cancel;
}

bool Shp_extrude::has_active_extrusion() const { return !m_extruded.IsNull(); }

bool Shp_extrude::get_both_sides() const { return m_extrude_both_sides; }

void Shp_extrude::set_both_sides(const bool both_sides)
{
  if (m_extrude_both_sides == both_sides)
    return;

  m_extrude_both_sides = both_sides;

  // Options checkbox does not move the mouse; refresh preview + length dim immediately.
  if (!m_extruded.IsNull() && m_last_preview_dist)
    update_extrude_preview_(*m_last_preview_dist, m_extrude_side);
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

bool Shp_extrude::use_lite_preview_()
{
  if (!gui().extrude_fast_preview_enabled())
    return false;

  return m_face_edge_count > static_cast<size_t>(gui().extrude_fast_preview_edge_threshold());
}

void Shp_extrude::update_dim_(const double extrude_dist, const Plane_side side)
{
  const gp_Vec normal_dir(m_to_extrude_pln.Axis().Direction());
  const double side_sign   = (side == Plane_side::Front) ? 1.0 : -1.0;
  const gp_Vec extrude_vec = normal_dir * (side_sign * extrude_dist);
  gp_Vec       face_offset(0.0, 0.0, 0.0);

  if (m_extrude_both_sides)
    face_offset = normal_dir * (-side_sign * (extrude_dist * 0.5));

  // Span the full extrusion height (near face to far face), including both-sides.
  const gp_Pnt dim_near(m_to_extrude_pt->XYZ() + face_offset.XYZ());
  const gp_Pnt dim_far(m_to_extrude_pt->XYZ() + face_offset.XYZ() + extrude_vec.XYZ());

  // Plane must contain the measurement segment so PrsDim_LengthDimension stays valid
  // (view plane alone can break when the segment is centered across the sketch plane).
  gp_Pln dim_pln = m_curr_view_pln;
  {
    const gp_Vec measure(dim_near, dim_far);
    if (measure.Magnitude() > Precision::Confusion())
    {
      const gp_Vec plane_norm = measure.Crossed(gp_Vec(m_curr_view_pln.Axis().Direction()));
      if (plane_norm.Magnitude() > Precision::Confusion())
        dim_pln = gp_Pln(dim_near, gp_Dir(plane_norm));
      else
      {
        // Measurement nearly along the view: fall back to sketch-plane X for a readable plane.
        const gp_Vec alt = measure.Crossed(gp_Vec(m_to_extrude_pln.XAxis().Direction()));
        if (alt.Magnitude() > Precision::Confusion())
          dim_pln = gp_Pln(dim_near, gp_Dir(alt));
      }
    }
  }

  // Recreate each update: SetMeasuredGeometry was unreliable when toggling both-sides
  // (endpoints jump from one-sided to a centered span).
  ctx().Remove(m_tmp_dim, false);
  m_tmp_dim = create_distance_annotation(dim_far, dim_near, dim_pln, gui().length_dimension_style());
  m_tmp_dim->SetCustomValue(extrude_dist / view().get_display_to_model_scale());
  ctx().Display(m_tmp_dim, false);
}

void Shp_extrude::update_extrude_preview_(const double extrude_dist, const Plane_side side)
{
  if (extrude_dist <= Precision::Confusion())
    return;

  EZY_ASSERT(side != Plane_side::On);

  const bool lite = use_lite_preview_();

  // Skip redundant work when height / side / preview mode are unchanged.
  if (!m_extruded.IsNull() && m_last_preview_dist && m_last_preview_side == side
      && m_last_preview_both_sides == m_extrude_both_sides && m_lite_preview_active == lite
      && std::fabs(extrude_dist - *m_last_preview_dist) <= Precision::Confusion())
    return;

  // Switching between lite and full needs a fresh AIS object.
  if (!m_extruded.IsNull() && m_lite_preview_active != lite)
  {
    clear_lite_other_face_();
    ctx().Remove(m_extruded, false);
    m_extruded.Nullify();
  }

  update_dim_(extrude_dist, side);

  const gp_Vec normal_dir(m_to_extrude_pln.Axis().Direction());
  const double side_sign   = (side == Plane_side::Front) ? 1.0 : -1.0;
  const gp_Vec extrude_vec = normal_dir * (side_sign * extrude_dist);
  gp_Vec       face_offset(0.0, 0.0, 0.0);
  if (m_extrude_both_sides)
    face_offset = normal_dir * (-side_sign * (extrude_dist * 0.5));

  if (lite)
  {
    // Dense faces: translate copies of the sketch face (Move-style cheap preview).
    if (m_extruded.IsNull())
    {
      m_extruded = new Shp(ctx(), m_to_extrude->Shape());
      m_extruded->set_disp_mode(AIS_WireFrame);
      ctx().Display(m_extruded, AIS_WireFrame, AIS_Shape::SelectionMode(TopAbs_SHAPE), false);
    }

    gp_Trsf far_trsf;
    far_trsf.SetTranslation(face_offset + extrude_vec);
    m_extruded->SetLocalTransformation(far_trsf);

    if (m_extrude_both_sides)
    {
      if (m_lite_face_other.IsNull())
      {
        m_lite_face_other = new AIS_Shape(m_to_extrude->Shape());
        ctx().Display(m_lite_face_other, AIS_WireFrame, -1, false);
        ctx().Deactivate(m_lite_face_other);
      }

      gp_Trsf near_trsf;
      near_trsf.SetTranslation(face_offset);
      m_lite_face_other->SetLocalTransformation(near_trsf);
      ctx().Redisplay(m_lite_face_other, false);
    }
    else
      clear_lite_other_face_();

    m_lite_preview_active = true;
  }
  else
  {
    clear_lite_other_face_();

    // Simple faces: full shaded solid prism each update.
    const TopoDS_Shape body = make_prism_body_(extrude_dist, side);

    if (m_extruded.IsNull())
    {
      m_extruded = new Shp(ctx(), body);
      m_extruded->set_disp_mode(AIS_Shaded);
      ctx().Display(m_extruded, AIS_Shaded, AIS_Shape::SelectionMode(TopAbs_SHAPE), false);
    }
    else
    {
      m_extruded->ResetTransformation();
      m_extruded->Set(body);
      m_extruded->set_disp_mode(AIS_Shaded);
    }

    m_extruded->SetMaterial(view().get_default_material());
    view().refresh_shape_shading_(m_extruded);
    m_lite_preview_active = false;
  }

  ctx().Redisplay(m_extruded, false);
  ctx().UpdateCurrentViewer();
  m_last_preview_dist       = extrude_dist;
  m_last_preview_side       = side;
  m_last_preview_both_sides = m_extrude_both_sides;
}

TopoDS_Shape Shp_extrude::make_prism_body_(const double extrude_dist, const Plane_side side) const
{
  EZY_ASSERT(side != Plane_side::On);
  EZY_ASSERT(extrude_dist > Precision::Confusion());

  const gp_Vec normal_dir(m_to_extrude_pln.Axis().Direction());
  const double side_sign   = (side == Plane_side::Front) ? 1.0 : -1.0;
  const gp_Vec extrude_vec = normal_dir * (side_sign * extrude_dist);

  TopoDS_Face face = TopoDS::Face(m_to_extrude->Shape());
  if (m_extrude_both_sides)
  {
    gp_Trsf trsf;
    trsf.SetTranslation(normal_dir * (-side_sign * (extrude_dist * 0.5)));
    face = TopoDS::Face(BRepBuilderAPI_Transform(face, trsf, true).Shape());
  }

  return BRepPrimAPI_MakePrism(face, extrude_vec);
}

void Shp_extrude::clear_lite_other_face_()
{
  if (!m_lite_face_other.IsNull())
  {
    ctx().Remove(m_lite_face_other, false);
    m_lite_face_other.Nullify();
  }
}

void Shp_extrude::clear_preview_()
{
  clear_lite_other_face_();
  m_face_edge_count         = 0;
  m_lite_preview_active     = false;
  m_last_preview_dist.reset();
  m_last_preview_side       = Plane_side::Front;
  m_last_preview_both_sides = false;
}

void Shp_extrude::refresh_tmp_dimension_style(const Length_dimension_style& style)
{
  if (m_tmp_dim.IsNull())
    return;
  apply_length_dimension_style(m_tmp_dim, style);
  ctx().Redisplay(m_tmp_dim, true);
}
