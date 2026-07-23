#include "shp_cross_section.h"

#include "gui_occt_view.h"
#include "shp_delta.h"
#include "utl_occt.h"

#include <BRepAdaptor_Curve.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Section.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepPrimAPI_MakeHalfSpace.hxx>
#include <BRep_Builder.hxx>
#include <Bnd_Box.hxx>
#include <GeomAbs_CurveType.hxx>
#include <Graphic3d_ClipPlane.hxx>
#include <Graphic3d_ZLayerId.hxx>
#include <Quantity_Color.hxx>
#include <Standard_Failure.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Solid.hxx>
#include <gp_Pln.hxx>
#include <gp_Vec.hxx>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <functional>
#include <limits>
#include <optional>
#include <sstream>
#include <utility>
#include <vector>

#ifndef __EMSCRIPTEN__
#  include <chrono>
#  include <future>
#  include <thread>
#endif

namespace
{
struct Assembled_section
{
  Status       status{Result_status::Success};
  TopoDS_Shape compound;
};

gp_Pln                cross_section_plane_(const gp_Ax3& frame, Cross_section_plane plane, double offset, bool invert_normal);
void                  count_curve_type_(const TopoDS_Edge& edge, Cross_section_geometry& result);
bool                  contains_solid_(const TopoDS_Shape& shape);
TopoDS_Shape          shape_world_(const Shp& shp);
gp_Ax3                frame_world_(const Shp& shp);
bool                  append_bounds_(const TopoDS_Shape& shape, Bnd_Box& bounds);
std::array<gp_Pnt, 8> bbox_corners_(const Bnd_Box& bounds);
void project_bbox_offsets_(const Bnd_Box& bounds, const gp_Pnt& origin, const gp_Dir& normal, double& out_min, double& out_max);
void project_bbox_uv_(const Bnd_Box& bounds, const gp_Pln& plane, double& u_min, double& u_max, double& v_min, double& v_max);
bool bbox_misses_plane_(const Bnd_Box& bounds, const gp_Pln& plane);
void add_plane_annotation_(const Bnd_Box& bounds, const gp_Pln& plane, BRep_Builder& builder, TopoDS_Compound& fill,
                           TopoDS_Compound& lines);
Result<Cross_section_geometry> section_one_shape_on_plane_(const TopoDS_Shape& shape, const gp_Pln& plane);
Result<Cross_section_geometry> cross_section_shape_on_plane_(const TopoDS_Shape& shape, const gp_Pln& plane);
Result<TopoDS_Solid>           keep_half_space_(const gp_Pln& plane, const Bnd_Box& bounds);
Result<TopoDS_Shape>           clip_solid_to_half_space_(const TopoDS_Shape& world_shape, const TopoDS_Solid& half_space);
void                           for_each_index_(size_t count, const std::function<void(size_t)>& fn, std::atomic<bool>* cancel);
std::vector<Result<Cross_section_geometry>> section_shapes_on_plane_(const std::vector<TopoDS_Shape>& world_shapes,
                                                                     const gp_Pln& plane, std::atomic<bool>* cancel);
Assembled_section assemble_section_geometries_(const std::vector<Result<Cross_section_geometry>>& section_results,
                                               const std::vector<std::string>&                    shape_names);
} // namespace

Result<Cross_section_geometry> cross_section_shape(const TopoDS_Shape& shape, const gp_Ax3& frame, Cross_section_plane plane,
                                                   double offset)
{
  return cross_section_shape_on_plane_(shape, cross_section_plane_(frame, plane, offset, false));
}

Shp_cross_section::Shp_cross_section(Occt_view& view)
    : Shp_operation_base(view)
{
}

Shp_cross_section::~Shp_cross_section()
{
  cancel_section_jobs_();
  clear_preview_ais_();
  clear_ais_clips_();
}

Status Shp_cross_section::preview_selected() { return preview(get_selected_shps_()); }

Status Shp_cross_section::request_preview_selected() { return request_preview(get_selected_shps_()); }

Result<Shp_cross_section::Shared_plane> Shp_cross_section::build_shared_plane_(const std::vector<Shp_ptr>& shapes)
{
  if (!std::isfinite(m_offset_display))
    return {Result_status::User_error, "Section offset must be a finite number."};

  if (shapes.empty())
    return {Result_status::User_error, "Select one or more solid shapes."};

  Shared_plane shared;
  shared.shapes.reserve(shapes.size());
  shared.world_shapes.reserve(shapes.size());
  shared.shape_names.reserve(shapes.size());
  gp_Ax3 shared_axes;
  bool   have_axes = false;

  for (const Shp_ptr& shp : shapes)
  {
    if (shp.IsNull() || shp->is_group())
      continue;

    TopoDS_Shape world_shape = shape_world_(*shp);
    if (!contains_solid_(world_shape))
      return {Result_status::User_error, shp->get_name() + ": Cross-section supports solid shapes only."};

    if (!append_bounds_(world_shape, shared.bounds))
      return {Result_status::User_error, shp->get_name() + ": Shape has empty bounds."};

    if (!have_axes)
    {
      shared_axes = frame_world_(*shp);
      have_axes   = true;
    }

    shared.shapes.push_back(shp);
    shared.world_shapes.push_back(std::move(world_shape));
    shared.shape_names.push_back(shp->get_name());
  }

  if (shared.shapes.empty() || shared.bounds.IsVoid() || !have_axes)
    return {Result_status::User_error, "Select one or more solid shapes."};

  double x_min, y_min, z_min, x_max, y_max, z_max;
  shared.bounds.Get(x_min, y_min, z_min, x_max, y_max, z_max);
  shared_axes.SetLocation(gp_Pnt((x_min + x_max) * 0.5, (y_min + y_max) * 0.5, (z_min + z_max) * 0.5));

  const double offset_model = view().to_model(m_offset_display);
  shared.plane              = cross_section_plane_(shared_axes, m_plane, offset_model, m_invert_normal);
  return shared;
}

Status Shp_cross_section::request_preview(const std::vector<Shp_ptr>& shapes)
{
  acknowledge_inputs_(shapes);

  Result<Shared_plane> shared = build_shared_plane_(shapes);
  if (!shared.has_value())
  {
    cancel_section_jobs_();
    clear_preview_ais_();
    clear_ais_clips_();
    m_last_section_status = Status(shared.status(), shared.message());
    ctx().UpdateCurrentViewer();
    return m_last_section_status;
  }

  const Shared_plane& plane_ctx = *shared;
  if (m_hide_back_side)
    apply_ais_clips_(plane_ctx.shapes, plane_ctx.plane);
  else
    clear_ais_clips_();

  display_plane_annotation_(plane_ctx.bounds, plane_ctx.plane);
  // Keep last cyan wires until the new section job finishes (feels less flickery while dragging).
  enqueue_section_(plane_ctx);
  ctx().UpdateCurrentViewer();
  return Status::ok();
}

Status Shp_cross_section::preview(const std::vector<Shp_ptr>& shapes)
{
  const Status requested = request_preview(shapes);
  if (!requested.is_ok())
    return requested;
  return wait_section_();
}

void Shp_cross_section::enqueue_section_(const Shared_plane& plane_ctx)
{
  m_cancel.store(true);
  const std::uint64_t gen = m_generation.fetch_add(1) + 1;

  Section_request req;
  req.generation   = gen;
  req.world_shapes = plane_ctx.world_shapes;
  req.shape_names  = plane_ctx.shape_names;
  req.plane        = plane_ctx.plane;

#ifndef __EMSCRIPTEN__
  const bool running = m_running_active;
#else
  const bool running = m_chunked.has_value();
#endif

  if (running)
    m_pending = std::move(req);
  else
  {
    m_pending.reset();
    start_section_job_(std::move(req));
  }
}

void Shp_cross_section::start_section_job_(Section_request req)
{
  m_cancel.store(false);

#ifndef __EMSCRIPTEN__
  m_running = std::async(std::launch::async,
                         [req = std::move(req), cancel = &m_cancel]() mutable
                         { return compute_section_result_(std::move(req), cancel); });
  m_running_active = true;
#else
  Chunked_job job;
  job.request = std::move(req);
  job.next    = 0;
  job.results.assign(job.request.world_shapes.size(), Result<Cross_section_geometry>{});
  m_chunked = std::move(job);
#endif
}

Shp_cross_section::Section_result Shp_cross_section::compute_section_result_(Section_request req, std::atomic<bool>* cancel)
{
  const std::vector<Result<Cross_section_geometry>> section_results =
      section_shapes_on_plane_(req.world_shapes, req.plane, cancel);

  Section_result out;
  out.generation = req.generation;
  if (cancel && cancel->load())
  {
    out.status = Status::user_error("Section cancelled.");
    return out;
  }

  const Assembled_section assembled = assemble_section_geometries_(section_results, req.shape_names);
  out.status                        = assembled.status;
  out.compound                      = assembled.compound;
  return out;
}

std::optional<Status> Shp_cross_section::finish_section_result_(Section_result result)
{
  if (result.generation != m_generation.load())
    return std::nullopt;

  m_last_section_status = result.status;
  if (result.status.is_ok() && !result.compound.IsNull())
    display_section_wires_(result.compound);
  else
    clear_section_wires_();

  ctx().UpdateCurrentViewer();
  return result.status;
}

std::optional<Status> Shp_cross_section::poll()
{
  std::optional<Section_result> finished;

#ifndef __EMSCRIPTEN__
  if (m_running_active && m_running.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
  {
    finished         = m_running.get();
    m_running_active = false;
  }
#else
  if (m_chunked.has_value())
  {
    Chunked_job& job = *m_chunked;
    if (m_cancel.load() || job.request.generation != m_generation.load())
    {
      m_chunked.reset();
    }
    else if (job.next < job.request.world_shapes.size())
    {
      job.results[job.next] = section_one_shape_on_plane_(job.request.world_shapes[job.next], job.request.plane);
      ++job.next;
    }
    else
    {
      const Assembled_section assembled = assemble_section_geometries_(job.results, job.request.shape_names);
      Section_result          result;
      result.generation = job.request.generation;
      result.status     = assembled.status;
      result.compound   = assembled.compound;
      finished          = std::move(result);
      m_chunked.reset();
    }
  }
#endif

  std::optional<Status> out;
  if (finished.has_value())
    out = finish_section_result_(std::move(*finished));

#ifndef __EMSCRIPTEN__
  const bool running = m_running_active;
#else
  const bool running = m_chunked.has_value();
#endif

  if (!running && m_pending.has_value())
  {
    Section_request req = std::move(*m_pending);
    m_pending.reset();
    start_section_job_(std::move(req));
  }

  return out;
}

bool Shp_cross_section::section_busy() const
{
#ifndef __EMSCRIPTEN__
  return m_running_active || m_pending.has_value();
#else
  return m_chunked.has_value() || m_pending.has_value();
#endif
}

Status Shp_cross_section::wait_section_()
{
  while (section_busy())
  {
    if (std::optional<Status> status = poll())
      m_last_section_status = *status;

#ifndef __EMSCRIPTEN__
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
#endif
  }

  if (std::optional<Status> status = poll())
    m_last_section_status = *status;

  return m_last_section_status;
}

void Shp_cross_section::cancel_section_jobs_()
{
  m_cancel.store(true);
  m_generation.fetch_add(1);
  m_pending.reset();

#ifndef __EMSCRIPTEN__
  if (m_running_active)
  {
    m_running.wait();
    (void)m_running.get();
    m_running_active = false;
  }
#else
  m_chunked.reset();
#endif
}

void Shp_cross_section::display_plane_annotation_(const Bnd_Box& bounds, const gp_Pln& plane)
{
  clear_plane_annotation_();

  TopoDS_Compound plane_fill;
  TopoDS_Compound plane_lines;
  BRep_Builder    builder;
  builder.MakeCompound(plane_fill);
  builder.MakeCompound(plane_lines);
  add_plane_annotation_(bounds, plane, builder, plane_fill, plane_lines);

  m_plane_fill = new AIS_Shape(plane_fill);
  m_plane_fill->SetColor(Quantity_NOC_YELLOW);
  m_plane_fill->SetTransparency(0.85);
  ctx().Display(m_plane_fill, AIS_Shaded, -1, false);
  ctx().Deactivate(m_plane_fill);

  m_plane_lines = new AIS_Shape(plane_lines);
  m_plane_lines->SetColor(Quantity_NOC_YELLOW);
  m_plane_lines->SetWidth(1.5);
  m_plane_lines->SetZLayer(Graphic3d_ZLayerId_Topmost);
  ctx().Display(m_plane_lines, AIS_WireFrame, -1, false);
  ctx().Deactivate(m_plane_lines);
}

void Shp_cross_section::display_section_wires_(const TopoDS_Shape& compound)
{
  clear_section_wires_();

  m_preview = new AIS_Shape(compound);
  m_preview->SetColor(Quantity_NOC_CYAN);
  m_preview->SetWidth(3.0);
  m_preview->SetZLayer(Graphic3d_ZLayerId_Topmost);
  ctx().Display(m_preview, AIS_WireFrame, -1, false);
  ctx().Deactivate(m_preview);
}

void Shp_cross_section::clear_section_wires_()
{
  if (!m_preview.IsNull())
    ctx().Remove(m_preview, false);

  m_preview.Nullify();
}

void Shp_cross_section::clear_plane_annotation_()
{
  if (!m_plane_fill.IsNull())
    ctx().Remove(m_plane_fill, false);

  if (!m_plane_lines.IsNull())
    ctx().Remove(m_plane_lines, false);

  m_plane_fill.Nullify();
  m_plane_lines.Nullify();
}

void Shp_cross_section::clear_preview_ais_()
{
  clear_section_wires_();
  clear_plane_annotation_();
}

Status Shp_cross_section::clip_selected() { return clip(get_selected_shps_()); }

Status Shp_cross_section::clip(const std::vector<Shp_ptr>& shapes)
{
  cancel_section_jobs_();

  Result<Shared_plane> shared = build_shared_plane_(shapes);
  if (!shared.has_value())
    return Status(shared.status(), shared.message());

  const Shared_plane& plane_ctx = *shared;
  Result<TopoDS_Solid> half     = keep_half_space_(plane_ctx.plane, plane_ctx.bounds);
  if (!half.has_value())
    return Status(half.status(), half.message());

  std::vector<TopoDS_Shape> clipped_geoms;
  clipped_geoms.reserve(plane_ctx.shapes.size());
  for (size_t i = 0; i < plane_ctx.shapes.size(); ++i)
  {
    Result<TopoDS_Shape> clipped = clip_solid_to_half_space_(plane_ctx.world_shapes[i], *half);
    if (!clipped.has_value())
      return Status(clipped.status(), plane_ctx.shapes[i]->get_name() + ": " + clipped.message());

    clipped_geoms.push_back(std::move(*clipped));
  }

  std::vector<Shape_rec> removed;
  removed.reserve(plane_ctx.shapes.size());
  for (const Shp_ptr& shp : plane_ctx.shapes)
    removed.push_back(capture_shape_rec(*shp));

  std::vector<Shape_rec> added;
  added.reserve(plane_ctx.shapes.size());
  m_shps = plane_ctx.shapes;

  for (size_t i = 0; i < plane_ctx.shapes.size(); ++i)
  {
    const Shp_ptr& old_shp = plane_ctx.shapes[i];
    Shp_ptr        new_shp = new Shp(ctx(), clipped_geoms[i]);
    new_shp->set_name(old_shp->get_name());
    new_shp->set_frame(frame_world_(*old_shp));
    assign_result_parent_(new_shp, std::vector<Shp_ptr>{old_shp});
    add_shp_(new_shp);
    copy_shape_material_from_(new_shp, old_shp);
    added.push_back(capture_shape_rec(*new_shp));
  }

  delete_operation_shps_();
  view().push_undo_delta(std::make_unique<Shape_replace_delta>(std::move(removed), std::move(added)));
  clear();
  return Status::ok("Clipped " + std::to_string(added.size()) + (added.size() == 1 ? " shape." : " shapes."));
}

void Shp_cross_section::clear()
{
  cancel_section_jobs_();
  clear_preview_ais_();
  clear_ais_clips_();
  ctx().UpdateCurrentViewer();
}

void Shp_cross_section::clear_ais_clips_()
{
  if (!m_ais_clip_plane.IsNull())
  {
    for (const Shp_ptr& shp : m_ais_clipped_shapes)
    {
      if (shp.IsNull())
        continue;

      shp->RemoveClipPlane(m_ais_clip_plane);
      ctx().Redisplay(shp, false);
    }
  }

  m_ais_clipped_shapes.clear();
  m_ais_clip_plane.Nullify();
}

void Shp_cross_section::apply_ais_clips_(const std::vector<Shp_ptr>& shapes, const gp_Pln& plane)
{
  clear_ais_clips_();
  m_ais_clip_plane = new Graphic3d_ClipPlane(plane);
  m_ais_clip_plane->SetOn(true);
  m_ais_clipped_shapes.reserve(shapes.size());
  for (const Shp_ptr& shp : shapes)
  {
    if (shp.IsNull() || shp->is_group())
      continue;

    shp->AddClipPlane(m_ais_clip_plane);
    ctx().Redisplay(shp, false);
    m_ais_clipped_shapes.push_back(shp);
  }
}

std::vector<Shape_id> Shp_cross_section::selection_ids_(const std::vector<Shp_ptr>& shapes)
{
  std::vector<Shape_id> ids;
  ids.reserve(shapes.size());
  for (const Shp_ptr& shp : shapes)
    if (!shp.IsNull())
      ids.push_back(shp->get_id());
  return ids;
}

bool Shp_cross_section::selection_stale() const { return selection_ids_(get_selected_shps_()) != m_acked_selection_ids; }

bool Shp_cross_section::preview_inputs_stale() const
{
  return selection_stale() || m_acked_plane != m_plane || m_acked_offset_display != m_offset_display ||
         m_acked_invert_normal != m_invert_normal || m_acked_hide_back_side != m_hide_back_side;
}

void Shp_cross_section::set_invert_normal(bool invert)
{
  if (m_invert_normal == invert)
    return;

  m_invert_normal  = invert;
  m_offset_display = -m_offset_display;
}

void Shp_cross_section::acknowledge_inputs_(const std::vector<Shp_ptr>& shapes)
{
  m_acked_selection_ids  = selection_ids_(shapes);
  m_acked_plane          = m_plane;
  m_acked_offset_display = m_offset_display;
  m_acked_invert_normal  = m_invert_normal;
  m_acked_hide_back_side = m_hide_back_side;
}

void Shp_cross_section::acknowledge_current_selection() { acknowledge_inputs_(get_selected_shps_()); }

bool Shp_cross_section::try_get_offset_range_display(double& out_min, double& out_max)
{
  const std::vector<Shp_ptr> selected = get_selected_shps_();
  if (selected.empty())
    return false;

  Bnd_Box combined_bounds;
  gp_Ax3  shared_axes;
  bool    have_axes = false;

  for (const Shp_ptr& shp : selected)
  {
    TopoDS_Shape world_shape = shape_world_(*shp);
    if (!contains_solid_(world_shape) || !append_bounds_(world_shape, combined_bounds))
      continue;

    if (!have_axes)
    {
      shared_axes = frame_world_(*shp);
      have_axes   = true;
    }
  }

  if (!have_axes || combined_bounds.IsVoid())
    return false;

  double x_min, y_min, z_min, x_max, y_max, z_max;
  combined_bounds.Get(x_min, y_min, z_min, x_max, y_max, z_max);
  shared_axes.SetLocation(gp_Pnt((x_min + x_max) * 0.5, (y_min + y_max) * 0.5, (z_min + z_max) * 0.5));

  const gp_Pln plane = cross_section_plane_(shared_axes, m_plane, 0.0, m_invert_normal);
  double       model_min;
  double       model_max;
  project_bbox_offsets_(combined_bounds, plane.Location(), plane.Axis().Direction(), model_min, model_max);
  if (!(model_max > model_min))
    return false;

  out_min = view().to_display(model_min);
  out_max = view().to_display(model_max);
  if (out_min > out_max)
    std::swap(out_min, out_max);

  return true;
}

namespace
{
gp_Pln cross_section_plane_(const gp_Ax3& frame, Cross_section_plane plane, double offset, bool invert_normal)
{
  gp_Dir normal;
  gp_Dir x_dir;
  switch (plane)
  {
  case Cross_section_plane::XY:
    normal = frame.Direction();
    x_dir  = frame.XDirection();
    break;
  case Cross_section_plane::XZ:
    normal = frame.YDirection();
    x_dir  = frame.XDirection();
    break;
  case Cross_section_plane::YZ:
    normal = frame.XDirection();
    x_dir  = frame.YDirection();
    break;
  }

  if (invert_normal)
    normal.Reverse();

  const gp_Pnt location = frame.Location().Translated(gp_Vec(normal) * offset);
  return gp_Pln(gp_Ax3(location, normal, x_dir));
}

Result<Cross_section_geometry> cross_section_shape_on_plane_(const TopoDS_Shape& shape, const gp_Pln& plane)
{
  if (shape.IsNull())
    return {Result_status::User_error, "Cannot section a null shape."};

  if (!contains_solid_(shape))
    return {Result_status::User_error, "Cross-section supports solid shapes only."};

  try
  {
    BRepAlgoAPI_Section section(shape, plane, false);
    section.Approximation(false);
    section.Build();
    if (!section.IsDone())
      return {Result_status::Topo_error, "Open CASCADE could not compute the section."};

    Cross_section_geometry result;
    result.shape = section.Shape();
    for (TopExp_Explorer it(result.shape, TopAbs_EDGE); it.More(); it.Next())
    {
      ++result.edge_count;
      count_curve_type_(TopoDS::Edge(it.Current()), result);
    }

    if (result.shape.IsNull() || result.edge_count == 0)
      return {Result_status::User_error, "The section plane does not intersect the shape."};

    return result;
  }
  catch (const Standard_Failure& e)
  {
    return {Result_status::Topo_error, std::string("Section failed: ") + standard_failure_message(e)};
  }
}

Result<Cross_section_geometry> section_one_shape_on_plane_(const TopoDS_Shape& shape, const gp_Pln& plane)
{
  Bnd_Box bounds;
  BRepBndLib::Add(shape, bounds);
  if (bbox_misses_plane_(bounds, plane))
    return {Result_status::User_error, "The section plane does not intersect the shape."};

  return cross_section_shape_on_plane_(shape, plane);
}

void count_curve_type_(const TopoDS_Edge& edge, Cross_section_geometry& result)
{
  switch (BRepAdaptor_Curve(edge).GetType())
  {
  case GeomAbs_Line:
    ++result.line_count;
    break;
  case GeomAbs_Circle:
    ++result.circle_count;
    break;
  case GeomAbs_Ellipse:
    ++result.ellipse_count;
    break;
  case GeomAbs_BSplineCurve:
    ++result.bspline_count;
    break;
  default:
    ++result.other_curve_count;
    break;
  }
}

bool contains_solid_(const TopoDS_Shape& shape)
{
  return !shape.IsNull() && (shape.ShapeType() == TopAbs_SOLID || TopExp_Explorer(shape, TopAbs_SOLID).More());
}

TopoDS_Shape shape_world_(const Shp& shp)
{
  TopoDS_Shape   shape           = shp.Shape();
  const gp_Trsf& local_transform = shp.LocalTransformation();
  if (local_transform.Form() != gp_Identity)
    shape = BRepBuilderAPI_Transform(shape, local_transform, true).Shape();

  return shape;
}

gp_Ax3 frame_world_(const Shp& shp)
{
  gp_Ax3         frame           = shp.get_frame();
  const gp_Trsf& local_transform = shp.LocalTransformation();
  if (local_transform.Form() != gp_Identity)
    frame.Transform(local_transform);

  return frame;
}

bool append_bounds_(const TopoDS_Shape& shape, Bnd_Box& bounds)
{
  Bnd_Box local;
  BRepBndLib::Add(shape, local);
  if (local.IsVoid())
    return false;

  bounds.Add(local);
  return true;
}

std::array<gp_Pnt, 8> bbox_corners_(const Bnd_Box& bounds)
{
  double x_min, y_min, z_min, x_max, y_max, z_max;
  bounds.Get(x_min, y_min, z_min, x_max, y_max, z_max);
  return {
      gp_Pnt(x_min, y_min, z_min), gp_Pnt(x_min, y_min, z_max), gp_Pnt(x_min, y_max, z_min), gp_Pnt(x_min, y_max, z_max),
      gp_Pnt(x_max, y_min, z_min), gp_Pnt(x_max, y_min, z_max), gp_Pnt(x_max, y_max, z_min), gp_Pnt(x_max, y_max, z_max),
  };
}

void project_bbox_offsets_(const Bnd_Box& bounds, const gp_Pnt& origin, const gp_Dir& normal, double& out_min, double& out_max)
{
  out_min = std::numeric_limits<double>::max();
  out_max = std::numeric_limits<double>::lowest();
  const gp_Vec normal_vec(normal);
  for (const gp_Pnt& corner : bbox_corners_(bounds))
  {
    const double t = gp_Vec(origin, corner).Dot(normal_vec);
    out_min        = std::min(out_min, t);
    out_max        = std::max(out_max, t);
  }
}

bool bbox_misses_plane_(const Bnd_Box& bounds, const gp_Pln& plane)
{
  if (bounds.IsVoid())
    return true;

  double d_min = 0.0;
  double d_max = 0.0;
  project_bbox_offsets_(bounds, plane.Location(), plane.Axis().Direction(), d_min, d_max);
  // Strict same-side means Section cannot hit; keep a tiny tolerance for near-tangency.
  constexpr double tol = 1.0e-7;
  return d_max < -tol || d_min > tol;
}

void for_each_index_(size_t count, const std::function<void(size_t)>& fn, std::atomic<bool>* cancel)
{
  if (count == 0)
    return;

#ifdef __EMSCRIPTEN__
  for (size_t i = 0; i < count; ++i)
  {
    if (cancel && cancel->load())
      break;

    fn(i);
  }
#else
  if (count == 1)
  {
    if (!(cancel && cancel->load()))
      fn(0);
    return;
  }

  const unsigned hw = std::thread::hardware_concurrency();
  const size_t   workers =
      std::min(count, static_cast<size_t>(hw == 0 ? 2u : hw));
  std::atomic<size_t> next{0};
  std::vector<std::thread> threads;
  threads.reserve(workers);
  for (size_t w = 0; w < workers; ++w)
  {
    threads.emplace_back(
        [&]()
        {
          for (;;)
          {
            if (cancel && cancel->load())
              break;

            const size_t i = next.fetch_add(1, std::memory_order_relaxed);
            if (i >= count)
              break;
            fn(i);
          }
        });
  }
  for (std::thread& t : threads)
    t.join();
#endif
}

std::vector<Result<Cross_section_geometry>> section_shapes_on_plane_(const std::vector<TopoDS_Shape>& world_shapes,
                                                                     const gp_Pln& plane, std::atomic<bool>* cancel)
{
  std::vector<Result<Cross_section_geometry>> results(world_shapes.size());
  for_each_index_(world_shapes.size(),
                  [&](size_t i)
                  {
                    if (cancel && cancel->load())
                      return;

                    results[i] = section_one_shape_on_plane_(world_shapes[i], plane);
                  },
                  cancel);
  return results;
}

Assembled_section assemble_section_geometries_(const std::vector<Result<Cross_section_geometry>>& section_results,
                                               const std::vector<std::string>&                    shape_names)
{
  Assembled_section out;
  TopoDS_Compound   compound;
  BRep_Builder      builder;
  builder.MakeCompound(compound);

  Cross_section_geometry totals;
  size_t                 missed = 0;
  for (size_t i = 0; i < section_results.size(); ++i)
  {
    const Result<Cross_section_geometry>& result = section_results[i];
    if (!result.has_value())
    {
      // A miss on one solid should not kill the shared preview for the others.
      if (result.status() == Result_status::User_error)
      {
        ++missed;
        continue;
      }

      const std::string name = i < shape_names.size() ? shape_names[i] : "Shape";
      out.status             = Status(result.status(), name + ": " + result.message());
      return out;
    }

    const Cross_section_geometry& geometry = *result;
    builder.Add(compound, geometry.shape);
    totals.edge_count += geometry.edge_count;
    totals.line_count += geometry.line_count;
    totals.circle_count += geometry.circle_count;
    totals.ellipse_count += geometry.ellipse_count;
    totals.bspline_count += geometry.bspline_count;
    totals.other_curve_count += geometry.other_curve_count;
  }

  if (totals.edge_count == 0)
  {
    out.status = Status::user_error("The section plane does not intersect the selection.");
    return out;
  }

  out.compound = compound;

  std::ostringstream msg;
  msg << "Section preview: " << totals.edge_count << " edge";
  if (totals.edge_count != 1)
    msg << "s";
  msg << " (" << totals.line_count << " line, " << totals.circle_count << " circle, " << totals.ellipse_count << " ellipse, "
      << totals.bspline_count << " B-spline, " << totals.other_curve_count << " other)";
  if (missed > 0)
  {
    msg << "; missed " << missed << " solid";
    if (missed != 1)
      msg << "s";
  }
  msg << ".";
  out.status = Status::ok(msg.str());
  return out;
}

void project_bbox_uv_(const Bnd_Box& bounds, const gp_Pln& plane, double& u_min, double& u_max, double& v_min, double& v_max)
{
  const gp_Pnt origin = plane.Location();
  const gp_Vec x_axis(plane.XAxis().Direction());
  const gp_Vec y_axis(plane.YAxis().Direction());
  u_min = std::numeric_limits<double>::max();
  u_max = std::numeric_limits<double>::lowest();
  v_min = std::numeric_limits<double>::max();
  v_max = std::numeric_limits<double>::lowest();
  for (const gp_Pnt& corner : bbox_corners_(bounds))
  {
    const gp_Vec relative(origin, corner);
    const double u = relative.Dot(x_axis);
    const double v = relative.Dot(y_axis);
    u_min          = std::min(u_min, u);
    u_max          = std::max(u_max, u);
    v_min          = std::min(v_min, v);
    v_max          = std::max(v_max, v);
  }
}

Result<TopoDS_Solid> keep_half_space_(const gp_Pln& plane, const Bnd_Box& bounds)
{
  if (bounds.IsVoid())
    return {Result_status::User_error, "Cannot build a clipping half-space from empty bounds."};

  double u_min, u_max, v_min, v_max;
  project_bbox_uv_(bounds, plane, u_min, u_max, v_min, v_max);
  const double width  = u_max - u_min;
  const double height = v_max - v_min;
  const double margin = std::max(1.0, std::max(width, height) * 0.5);
  u_min -= margin;
  u_max += margin;
  v_min -= margin;
  v_max += margin;

  try
  {
    const TopoDS_Face face = BRepBuilderAPI_MakeFace(plane, u_min, u_max, v_min, v_max).Face();
    if (face.IsNull())
      return {Result_status::Topo_error, "Could not build the clipping plane face."};

    // Reference point on the positive-normal side (kept half, same as Hide back side / AIS clip).
    const gp_Pnt ref = plane.Location().Translated(gp_Vec(plane.Axis().Direction()));
    BRepPrimAPI_MakeHalfSpace half(face, ref);
    const TopoDS_Solid        solid = half.Solid();
    if (solid.IsNull())
      return {Result_status::Topo_error, "Could not build the clipping half-space."};

    return solid;
  }
  catch (const Standard_Failure& e)
  {
    return {Result_status::Topo_error, std::string("Half-space failed: ") + standard_failure_message(e)};
  }
}

Result<TopoDS_Shape> clip_solid_to_half_space_(const TopoDS_Shape& world_shape, const TopoDS_Solid& half_space)
{
  try
  {
    BRepAlgoAPI_Common common(world_shape, half_space);
    common.Build();
    if (!common.IsDone())
      return {Result_status::Topo_error, "Open CASCADE could not clip the solid."};

    const TopoDS_Shape& result = common.Shape();
    if (result.IsNull() || !contains_solid_(result))
      return {Result_status::User_error, "Clip removed the entire solid (nothing left on the kept side)."};

    return result;
  }
  catch (const Standard_Failure& e)
  {
    return {Result_status::Topo_error, std::string("Clip failed: ") + standard_failure_message(e)};
  }
}

void add_plane_annotation_(const Bnd_Box& bounds, const gp_Pln& plane, BRep_Builder& builder, TopoDS_Compound& fill,
                           TopoDS_Compound& lines)
{
  if (bounds.IsVoid())
    return;

  const gp_Pnt origin = plane.Location();
  const gp_Vec x_axis(plane.XAxis().Direction());
  const gp_Vec y_axis(plane.YAxis().Direction());
  double       u_min, u_max, v_min, v_max;
  project_bbox_uv_(bounds, plane, u_min, u_max, v_min, v_max);

  const double width  = u_max - u_min;
  const double height = v_max - v_min;
  const double margin = std::max(width, height) * 0.1;
  u_min -= margin;
  u_max += margin;
  v_min -= margin;
  v_max += margin;

  const TopoDS_Face plane_face = BRepBuilderAPI_MakeFace(plane, u_min, u_max, v_min, v_max).Face();
  builder.Add(fill, plane_face);

  auto point_on_plane = [&](double u, double v)
  {
    return origin.Translated(x_axis * u + y_axis * v);
  };

  BRepBuilderAPI_MakePolygon outline;
  outline.Add(point_on_plane(u_min, v_min));
  outline.Add(point_on_plane(u_max, v_min));
  outline.Add(point_on_plane(u_max, v_max));
  outline.Add(point_on_plane(u_min, v_max));
  outline.Close();
  builder.Add(lines, outline.Wire());

  const double center_u      = (u_min + u_max) * 0.5;
  const double center_v      = (v_min + v_max) * 0.5;
  const gp_Pnt center        = point_on_plane(center_u, center_v);
  const double normal_length = std::max(u_max - u_min, v_max - v_min) * 0.25;
  const gp_Vec normal(plane.Axis().Direction());
  const gp_Pnt tip = center.Translated(normal * normal_length);
  builder.Add(lines, BRepBuilderAPI_MakeEdge(center, tip).Edge());

  const gp_Pnt arrow_left  = tip.Translated(normal * (-normal_length * 0.2) + x_axis * (normal_length * 0.1));
  const gp_Pnt arrow_right = tip.Translated(normal * (-normal_length * 0.2) - x_axis * (normal_length * 0.1));
  builder.Add(lines, BRepBuilderAPI_MakeEdge(arrow_left, tip).Edge());
  builder.Add(lines, BRepBuilderAPI_MakeEdge(arrow_right, tip).Edge());
}
} // namespace
