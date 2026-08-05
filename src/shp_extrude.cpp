#include "shp_extrude.h"

#include <AIS_Shape.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepOffsetAPI_ThruSections.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepTools.hxx>
#include <Precision.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <cmath>
#include <gp_Trsf.hxx>
#include <vector>

#include "utl_dbg.h"
#include "utl_geom.h"
#include "gui.h"
#include "gui_occt_view.h"
#include "skt.h"
#include "shp_delta.h"
#include "utl.h"
#include "utl_occt.h"

namespace
{
size_t       count_shape_edges_(const TopoDS_Shape& shape);
gp_Pnt       centroid_of_verts_(const std::vector<gp_Pnt>& verts);
TopoDS_Wire  transform_wire_(const TopoDS_Wire& wire, const gp_Trsf& trsf);
gp_Trsf      section_trsf_(const gp_Ax1& axis, double height_along_axis, double twist_rad);
TopoDS_Shape loft_twisted_wire_(const TopoDS_Wire& wire, const gp_Ax1& axis, double h0, double h1, double ang0, double ang1,
                                int n_seg);
std::vector<TopoDS_Wire> face_hole_wires_(const TopoDS_Face& face, const TopoDS_Wire& outer_wire);
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

  m_to_extrude_pln  = face->owner_sketch.get_plane();
  m_extrude_side    = Plane_side::Front;
  m_to_extrude_pt   = closest_to_camera(view().view_handle(), face->verts_3d);
  m_curr_view_pln   = view().get_view_plane(*m_to_extrude_pt);
  m_to_extrude      = shp;
  m_face_edge_count = count_shape_edges_(shp->Shape());
  m_twist_centroid  = centroid_of_verts_(face->verts_3d);
  clear_all(m_lite_preview_active, m_phase, m_twist_angle, m_show_angle_input, m_entered_twist_deg);

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
  EZY_ASSERT(m_extruded);
  EZY_ASSERT(m_last_preview_dist);

  // Lite preview only moved face copies. Always bake a shaded solid.
  clear_lite_other_face_();
  const TopoDS_Shape body = make_body_(*m_last_preview_dist, m_extrude_side, m_twist_angle);
  m_extruded->ResetTransformation();
  m_extruded->Set(body);
  m_extruded->set_disp_mode(AIS_Shaded);
  m_extruded->set_name(view().get_unique_shape_name("Shape"));
  add_shp_(m_extruded, true);
  view().push_undo_delta(std::make_unique<Shape_add_delta>(std::vector<Shape_rec>{capture_shape_rec(*m_extruded)}));
  clear_length_dim_();
  clear_angle_dim_();
  clear_all(m_to_extrude_pt, m_to_extrude, m_extruded);
  clear_preview_();
  clear_session_inputs_();
  ctx().ClearSelected(true);
}

void Shp_extrude::on_left_click()
{
  EZY_ASSERT(has_active_extrusion());
  if (m_phase == Phase::Height && m_twist_enabled)
  {
    lock_height_begin_twist_();
    return;
  }

  finalize();
}

bool Shp_extrude::cancel()
{
  clear_lite_other_face_();
  ctx().Remove(m_extruded, true);
  clear_length_dim_();
  clear_angle_dim_();
  bool did_cancel = m_to_extrude_pt.has_value();
  clear_all(m_to_extrude_pt, m_to_extrude, m_extruded);
  clear_preview_();
  clear_session_inputs_();

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

bool Shp_extrude::get_twist() const { return m_twist_enabled; }

void Shp_extrude::set_twist(const bool twist)
{
  if (m_twist_enabled == twist)
    return;

  m_twist_enabled = twist;

  if (!twist && m_phase == Phase::Twist)
  {
    // Return to editable height preview; drop twist angle.
    clear_all(m_phase, m_twist_angle, m_show_angle_input, m_entered_twist_deg);
    clear_angle_dim_();
    view().set_entered_dim(std::nullopt);
    view().set_show_dim_input(false);
    gui().hide_angle_edit(false);
  }

  if (!m_extruded.IsNull() && m_last_preview_dist)
    update_extrude_preview_(*m_last_preview_dist, m_extrude_side);
}

bool Shp_extrude::is_twist_phase() const { return m_phase == Phase::Twist; }

void Shp_extrude::begin_angle_input(const ScreenCoords& screen_coords)
{
  if (m_phase != Phase::Twist || !m_last_preview_dist)
    return;

  m_show_angle_input = true;
  update_twist_(screen_coords);
}

void Shp_extrude::lock_height_begin_twist_()
{
  EZY_ASSERT(m_last_preview_dist);
  EZY_ASSERT(m_twist_enabled);

  m_phase = Phase::Twist;
  view().set_show_dim_input(false);
  // Lock height to the last preview distance (typed or mouse).
  view().set_entered_dim(*m_last_preview_dist);
  clear_all(m_twist_angle, m_entered_twist_deg, m_show_angle_input);
  clear_length_dim_();
  update_extrude_preview_(*m_last_preview_dist, m_extrude_side);
}

void Shp_extrude::_update_extrude(const ScreenCoords& screen_coords)
{
  if (m_phase == Phase::Twist)
    update_twist_(screen_coords);
  else
    update_height_(screen_coords);
}

void Shp_extrude::update_height_(const ScreenCoords& screen_coords)
{
  std::optional<gp_Pnt> p = view().pt3d_on_plane(screen_coords, m_curr_view_pln);
  if (!p)
    return;

  double extrude_dist = m_to_extrude_pln.Distance(*p);
  if (auto entered_dim = view().get_entered_dim(); entered_dim)
    extrude_dist = *entered_dim;

  if (extrude_dist <= Precision::Confusion())
    return;

  const Plane_side cursor_side = side_of_plane(m_to_extrude_pln, *p);
  if (cursor_side != Plane_side::On)
    m_extrude_side = cursor_side;

  const double scaled_dist = extrude_dist / view().get_display_to_model_scale();

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
        if (m_twist_enabled)
          lock_height_begin_twist_();
        else
          finalize();
      }
    };

    gui().set_dist_edit(float(scaled_dist), std::move(std::function<void(float, bool)>(l)), screen_coords);
  }

  update_extrude_preview_(extrude_dist, m_extrude_side);
}

void Shp_extrude::update_twist_(const ScreenCoords& screen_coords)
{
  EZY_ASSERT(m_last_preview_dist);

  std::optional<gp_Pnt> p = view().pt3d_on_plane(screen_coords, m_to_extrude_pln);
  if (!p)
    return;

  double twist_rad = m_twist_angle;
  if (m_show_angle_input && m_entered_twist_deg)
    twist_rad = to_radians(*m_entered_twist_deg);
  else
  {
    const gp_Vec v(m_twist_centroid, *p);
    const gp_Vec x_dir(m_to_extrude_pln.XAxis().Direction());
    const gp_Vec y_dir(m_to_extrude_pln.YAxis().Direction());
    const double vx = v.Dot(x_dir);
    const double vy = v.Dot(y_dir);
    if (std::hypot(vx, vy) <= Precision::Confusion())
      return;

    twist_rad = std::atan2(vy, vx);
    if (!m_show_angle_input)
      m_entered_twist_deg.reset();
  }

  m_twist_angle = twist_rad;

  if (m_show_angle_input)
  {
    auto cb = [this](float new_angle_deg, bool do_finalize)
    {
      m_entered_twist_deg = static_cast<double>(new_angle_deg);
      m_twist_angle       = to_radians(*m_entered_twist_deg);
      if (m_last_preview_dist)
        update_extrude_preview_(*m_last_preview_dist, m_extrude_side);
      if (do_finalize)
      {
        m_show_angle_input = false;
        finalize();
      }
    };

    gui().set_angle_edit(float(to_degrees(m_twist_angle)), std::move(std::function<void(float, bool)>(cb)), screen_coords);
  }

  update_extrude_preview_(*m_last_preview_dist, m_extrude_side);
}

bool Shp_extrude::use_lite_preview_()
{
  if (!gui().extrude_fast_preview_enabled())
    return false;

  return m_face_edge_count > static_cast<size_t>(gui().extrude_fast_preview_edge_threshold());
}

void Shp_extrude::update_dim_(const double extrude_dist, const Plane_side side)
{
  clear_angle_dim_();

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
      if (plane_norm.Magnitude() <= Precision::Confusion())
      {
        // Measurement nearly along the view: fall back to sketch-plane X for a readable plane.
        const gp_Vec alt = measure.Crossed(gp_Vec(m_to_extrude_pln.XAxis().Direction()));
        if (alt.Magnitude() > Precision::Confusion())
          dim_pln = gp_Pln(dim_near, gp_Dir(alt));
      }
      else
        dim_pln = gp_Pln(dim_near, gp_Dir(plane_norm));
    }
  }

  // Recreate each update: SetMeasuredGeometry was unreliable when toggling both-sides
  // (endpoints jump from one-sided to a centered span).
  clear_length_dim_();
  m_tmp_dim = create_distance_annotation(dim_far, dim_near, dim_pln, gui().length_dimension_style());
  m_tmp_dim->SetCustomValue(extrude_dist / view().get_display_to_model_scale());
  ctx().Display(m_tmp_dim, false);
}

void Shp_extrude::update_angle_dim_()
{
  clear_length_dim_();
  EZY_ASSERT(m_last_preview_dist);

  const double radius = twist_dim_radius_();
  if (radius <= Precision::Confusion())
    return;

  // Geometry needs a non-zero opening; label still shows the real twist degrees.
  double geom_ang = m_twist_angle;
  if (std::fabs(geom_ang) < to_radians(1.0))
    geom_ang = (geom_ang < 0.0) ? -to_radians(1.0) : to_radians(1.0);

  const double side_sign = (m_extrude_side == Plane_side::Front) ? 1.0 : -1.0;
  const double h_far     = m_extrude_both_sides ? (0.5 * side_sign * *m_last_preview_dist) : (side_sign * *m_last_preview_dist);
  const gp_Vec normal_dir(m_to_extrude_pln.Axis().Direction());
  const gp_Pnt center(m_twist_centroid.XYZ() + normal_dir.XYZ() * h_far);

  const gp_Vec x_dir(m_to_extrude_pln.XAxis().Direction());
  const gp_Vec y_dir(m_to_extrude_pln.YAxis().Direction());
  const gp_Pnt p_ref(center.XYZ() + x_dir.XYZ() * radius);
  const gp_Pnt p_cur(center.XYZ() + (x_dir * std::cos(geom_ang) + y_dir * std::sin(geom_ang)).XYZ() * radius);

  clear_angle_dim_();
  m_tmp_angle_dim = create_angle_annotation(p_ref, center, p_cur, to_degrees(m_twist_angle), gui().length_dimension_style());
  ctx().Display(m_tmp_angle_dim, false);
}

void Shp_extrude::clear_length_dim_()
{
  if (!m_tmp_dim.IsNull())
  {
    ctx().Remove(m_tmp_dim, false);
    m_tmp_dim.Nullify();
  }
}

void Shp_extrude::clear_angle_dim_()
{
  if (!m_tmp_angle_dim.IsNull())
  {
    ctx().Remove(m_tmp_angle_dim, false);
    m_tmp_angle_dim.Nullify();
  }
}

void Shp_extrude::update_extrude_preview_(const double extrude_dist, const Plane_side side)
{
  if (extrude_dist <= Precision::Confusion())
    return;

  EZY_ASSERT(side != Plane_side::On);

  const bool lite            = use_lite_preview_();
  const bool twist_phase_now = (m_phase == Phase::Twist);

  // Skip redundant work when height / side / twist / phase / preview mode are unchanged.
  if (!m_extruded.IsNull() && m_last_preview_dist && m_last_preview_side == side &&
      m_last_preview_both_sides == m_extrude_both_sides && m_lite_preview_active == lite &&
      m_last_preview_was_twist_phase == twist_phase_now &&
      std::fabs(extrude_dist - *m_last_preview_dist) <= Precision::Confusion() &&
      std::fabs(m_twist_angle - m_last_preview_twist) <= Precision::Angular())
    return;

  // Switching between lite and full needs a fresh AIS object.
  if (!m_extruded.IsNull() && m_lite_preview_active != lite)
  {
    clear_lite_other_face_();
    ctx().Remove(m_extruded, false);
    m_extruded.Nullify();
  }

  if (twist_phase_now)
    update_angle_dim_();
  else
    update_dim_(extrude_dist, side);

  const double side_sign = (side == Plane_side::Front) ? 1.0 : -1.0;

  if (lite)
  {
    // Dense faces: move copies of the sketch face (translate + optional twist rotation).
    // Side walls are not previewed; finalize builds the real prism / thru-sections solid.
    if (m_extruded.IsNull())
    {
      m_extruded = new Shp(ctx(), m_to_extrude->Shape());
      m_extruded->set_disp_mode(AIS_WireFrame);
      ctx().Display(m_extruded, AIS_WireFrame, AIS_Shape::SelectionMode(TopAbs_SHAPE), false);
    }

    const gp_Ax1 axis    = twist_axis_();
    const double h_far   = m_extrude_both_sides ? (0.5 * side_sign * extrude_dist) : (side_sign * extrude_dist);
    const double ang_far = m_extrude_both_sides ? (0.5 * m_twist_angle) : m_twist_angle;
    m_extruded->SetLocalTransformation(section_trsf_(axis, h_far, ang_far));

    if (m_extrude_both_sides)
    {
      if (m_lite_face_other.IsNull())
      {
        m_lite_face_other = new AIS_Shape(m_to_extrude->Shape());
        ctx().Display(m_lite_face_other, AIS_WireFrame, -1, false);
        ctx().Deactivate(m_lite_face_other);
      }

      const double h_near   = -0.5 * side_sign * extrude_dist;
      const double ang_near = -0.5 * m_twist_angle;
      m_lite_face_other->SetLocalTransformation(section_trsf_(axis, h_near, ang_near));
      ctx().Redisplay(m_lite_face_other, false);
    }
    else
      clear_lite_other_face_();

    m_lite_preview_active = true;
  }
  else
  {
    clear_lite_other_face_();

    // Shaded solid: prism when twist ~ 0, thru-sections when twisted.
    const TopoDS_Shape body = make_body_(extrude_dist, side, m_twist_angle);

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
  m_last_preview_dist            = extrude_dist;
  m_last_preview_side            = side;
  m_last_preview_both_sides      = m_extrude_both_sides;
  m_last_preview_twist           = m_twist_angle;
  m_last_preview_was_twist_phase = twist_phase_now;
}

gp_Ax1 Shp_extrude::twist_axis_() const { return gp_Ax1(m_twist_centroid, m_to_extrude_pln.Axis().Direction()); }

double Shp_extrude::twist_dim_radius_() const
{
  double radius = 0.0;
  if (auto* face = dynamic_cast<Sketch_face_shp*>(m_to_extrude.get()))
  {
    for (const gp_Pnt& p : face->verts_3d)
      radius = std::max(radius, m_twist_centroid.Distance(p));
  }

  if (radius <= Precision::Confusion() && m_to_extrude_pt)
    radius = m_twist_centroid.Distance(*m_to_extrude_pt);

  return radius;
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

TopoDS_Shape Shp_extrude::make_body_(const double extrude_dist, const Plane_side side, const double twist_rad) const
{
  EZY_ASSERT(side != Plane_side::On);
  EZY_ASSERT(extrude_dist > Precision::Confusion());

  if (std::fabs(twist_rad) <= Precision::Angular())
    return make_prism_body_(extrude_dist, side);

  const TopoDS_Face face       = TopoDS::Face(m_to_extrude->Shape());
  const TopoDS_Wire outer_wire = BRepTools::OuterWire(face);
  EZY_ASSERT(!outer_wire.IsNull());

  const double side_sign = (side == Plane_side::Front) ? 1.0 : -1.0;
  const gp_Ax1 axis      = twist_axis_();

  // Param t in [0, 1]: height along signed extrude axis, twist from near to far.
  const double h0   = m_extrude_both_sides ? (-0.5 * side_sign * extrude_dist) : 0.0;
  const double h1   = m_extrude_both_sides ? (0.5 * side_sign * extrude_dist) : (side_sign * extrude_dist);
  const double ang0 = m_extrude_both_sides ? (-0.5 * twist_rad) : 0.0;
  const double ang1 = m_extrude_both_sides ? (0.5 * twist_rad) : twist_rad;

  const double abs_ang = std::fabs(twist_rad);
  const int    n_seg   = std::max(1, static_cast<int>(std::ceil(abs_ang / to_radians(45.0))));

  TopoDS_Shape body = loft_twisted_wire_(outer_wire, axis, h0, h1, ang0, ang1, n_seg);

  // MakePrism keeps face holes; loft only the outer wire then cut matching twisted hole solids.
  for (const TopoDS_Wire& hole : face_hole_wires_(face, outer_wire))
  {
    const TopoDS_Shape hole_body = loft_twisted_wire_(hole, axis, h0, h1, ang0, ang1, n_seg);
    BRepAlgoAPI_Cut    cut(body, hole_body);
    EZY_ASSERT(cut.IsDone());
    body = try_make_solid(cut.Shape());
  }

  return body;
}

void Shp_extrude::clear_lite_other_face_()
{
  if (!m_lite_face_other.IsNull())
  {
    ctx().Remove(m_lite_face_other, false);
    m_lite_face_other.Nullify();
  }
}

void Shp_extrude::clear_session_inputs_()
{
  view().set_show_dim_input(false);
  view().set_entered_dim(std::nullopt);
  clear_all(m_show_angle_input, m_entered_twist_deg);
  gui().hide_angle_edit(false);
  gui().hide_dist_edit(false);
}

void Shp_extrude::clear_preview_()
{
  clear_lite_other_face_();
  clear_length_dim_();
  clear_angle_dim_();
  clear_all(m_face_edge_count, m_lite_preview_active, m_last_preview_dist, m_last_preview_side, m_last_preview_both_sides,
            m_last_preview_twist, m_last_preview_was_twist_phase, m_phase, m_twist_angle);
}

void Shp_extrude::refresh_tmp_dimension_style(const Length_dimension_style& style)
{
  if (!m_tmp_dim.IsNull())
  {
    apply_length_dimension_style(m_tmp_dim, style);
    ctx().Redisplay(m_tmp_dim, true);
  }

  if (!m_tmp_angle_dim.IsNull())
  {
    apply_angle_dimension_style(m_tmp_angle_dim, style);
    ctx().Redisplay(m_tmp_angle_dim, true);
  }
}

namespace
{
size_t count_shape_edges_(const TopoDS_Shape& shape)
{
  size_t n = 0;
  for (TopExp_Explorer ex(shape, TopAbs_EDGE); ex.More(); ex.Next())
    ++n;

  return n;
}

gp_Pnt centroid_of_verts_(const std::vector<gp_Pnt>& verts)
{
  EZY_ASSERT(!verts.empty());
  gp_XYZ sum(0.0, 0.0, 0.0);
  for (const gp_Pnt& p : verts)
    sum += p.XYZ();

  sum /= static_cast<double>(verts.size());

  return gp_Pnt(sum);
}

TopoDS_Wire transform_wire_(const TopoDS_Wire& wire, const gp_Trsf& trsf)
{
  return TopoDS::Wire(BRepBuilderAPI_Transform(wire, trsf, true).Shape());
}

gp_Trsf section_trsf_(const gp_Ax1& axis, double height_along_axis, double twist_rad)
{
  gp_Trsf rot;
  rot.SetRotation(axis, twist_rad);
  gp_Trsf trans;
  trans.SetTranslation(gp_Vec(axis.Direction()) * height_along_axis);

  return trans * rot;
}

/// Ruled thru-sections solid from a closed wire with height + twist along `axis`.
/// Compatibility is off so intentional twist keeps edge/vertex pairing.
TopoDS_Shape loft_twisted_wire_(const TopoDS_Wire& wire, const gp_Ax1& axis, const double h0, const double h1,
                                const double ang0, const double ang1, const int n_seg)
{
  EZY_ASSERT(!wire.IsNull());
  EZY_ASSERT(n_seg >= 1);

  BRepOffsetAPI_ThruSections maker(true /*isSolid*/, true /*ruled*/);
  maker.CheckCompatibility(false);
  for (int i = 0; i <= n_seg; ++i)
  {
    const double t      = static_cast<double>(i) / static_cast<double>(n_seg);
    const double height = h0 + t * (h1 - h0);
    const double ang    = ang0 + t * (ang1 - ang0);
    maker.AddWire(transform_wire_(wire, section_trsf_(axis, height, ang)));
  }

  maker.Build();
  EZY_ASSERT(maker.IsDone());

  return try_make_solid(maker.Shape());
}

std::vector<TopoDS_Wire> face_hole_wires_(const TopoDS_Face& face, const TopoDS_Wire& outer_wire)
{
  std::vector<TopoDS_Wire> holes;
  for (TopExp_Explorer ex(face, TopAbs_WIRE); ex.More(); ex.Next())
  {
    const TopoDS_Wire w = TopoDS::Wire(ex.Current());
    if (w.IsNull() || w.IsSame(outer_wire))
      continue;

    holes.push_back(w);
  }

  return holes;
}
} // namespace
