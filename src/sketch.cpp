#include "sketch.h"

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <Graphic3d_AspectFillArea3d.hxx>
#include <Precision.hxx>
#include <PrsDim_LengthDimension.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>
#include <V3d_View.hxx>
#include <cmath>
#include <functional>
#include <map>

#include "utl_geom.h"
#include "gui.h"
#include "mode.h"
#include "occt_view.h"
#include "sketch_delta.h"
#include "utl_occt.h"
#include "utl.h"

using namespace glm;

Sketch::Sketch(const std::string& name, Occt_view& view, const gp_Pln& pln)
    : m_view(view)
    , m_ctx(view.ctx())
    , m_pln(pln)
    , m_name(name)
    , m_nodes(view, pln)
    , m_edges(*this)
    , m_topo(*this)
    , m_dims(*this)
    , m_node_marks(*this)
    , m_tools(*this)
    , m_underlay(view.ctx())
{
}

Sketch::Sketch(const std::string& name, Occt_view& view, const gp_Pln& pln, const TopoDS_Wire& outer_wire)
    : m_view(view)
    , m_ctx(view.ctx())
    , m_pln(pln)
    , m_name(name)
    , m_nodes(view, pln)
    , m_edges(*this)
    , m_topo(*this)
    , m_dims(*this)
    , m_node_marks(*this)
    , m_tools(*this)
    , m_underlay(view.ctx())
{
  m_originating_face = new AIS_Shape(outer_wire);
  m_ctx.Display(m_originating_face, true);
  update_originating_face_style();
}

Sketch::~Sketch()
{
  m_underlay.ctx_erase();
  m_edges.remove_displayed();
  m_dims.remove_displayed();
  m_topo.remove_displayed_faces();

  if (m_operation_axis.has_value())
    m_ctx.Remove(m_operation_axis->shp, false);

  m_node_marks.remove_all();
  m_ctx.Remove(m_originating_face, true);
}

void Sketch::add_sketch_pt(const ScreenCoords& screen_coords) { m_tools.on_click(screen_coords); }

void Sketch::sketch_pt_move(const ScreenCoords& screen_coords)
{
  switch (get_mode())
  {
  case Mode::Sketch_dim_anno:
    (void)m_nodes.try_pick_existing_node(screen_coords);
    m_dims.update_len_dim_rubber_line_(screen_coords);
    return;
  default:
    break;
  }

  m_tools.on_move(screen_coords);
}

void Sketch::dimension_input(const ScreenCoords& screen_coords)
{
  m_dims.set_show_dim_input(true);
  sketch_pt_move(screen_coords);
}

void Sketch::angle_input(const ScreenCoords& screen_coords)
{
  m_dims.set_show_angle_input(true);
  sketch_pt_move(screen_coords);
}

void Sketch::remove_edge(const Sketch_AIS_edge& to_remove)
{
  m_edges.remove_by_ais(to_remove);
  update_faces_();
}

void Sketch::dbg_append_str(std::string& out) const
{
  out += "    Nodes: " + std::to_string(m_nodes.size()) + "\n";
  out += "    Edges: " + std::to_string(m_edges.size()) + "\n";
  out += "    Faces: " + std::to_string(m_topo.faces().size()) + "\n";
  out += "Tmp Nodes: " + std::to_string(m_tools.tmp_node_count()) + "\n";
  out += "Tmp Edges: " + std::to_string(m_tools.tmp_edge_count()) + "\n";
}

void Sketch::update_edge_style_(AIS_Shape_ptr& shp)
{
  switch (m_edge_style)
  {
  case Edge_style::Full:
    // shp->SetWidth(1.5);
    shp->SetWidth(1.0);
    shp->SetColor(Quantity_Color(0.0, 1.0, 0.0, Quantity_TOC_RGB));
    shp->SetTransparency(0.0);

    break;

  case Edge_style::Background:
    shp->SetWidth(1.0);
    shp->SetColor(Quantity_Color(0.3, 0.3, 0.3, Quantity_TOC_RGB));
    shp->SetTransparency(0.7);
    break;

  default:
    EZY_ASSERT(false);
  }
}

void Sketch::update_originating_face_style()
{
  if (!m_originating_face)
    return;

  switch (m_edge_style)
  {
  case Edge_style::Full:
    m_originating_face->SetWidth(1.0);
    m_originating_face->SetColor(Quantity_Color(0.3, 0.0, 0.0, Quantity_TOC_RGB));
    m_originating_face->SetTransparency(0.0);

    break;

  case Edge_style::Background:
    m_originating_face->SetWidth(1.0);
    m_originating_face->SetColor(Quantity_Color(0.3, 0.3, 0.3, Quantity_TOC_RGB));
    m_originating_face->SetTransparency(0.7);
    break;

  default:
    EZY_ASSERT(false);
  }

  m_ctx.Redisplay(m_originating_face, true);
}

void Sketch::update_edge_shp_(Edge& edge, const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b)
{
  EZY_ASSERT(unique(pt_a, pt_b));

  TopoDS_Shape edge_shape = BRepBuilderAPI_MakeEdge(to_3d(m_pln, pt_a), to_3d(m_pln, pt_b)).Edge();

  if (edge.shp)
  {
    edge.shp->Set(edge_shape);
    update_edge_style_(edge.shp);
    m_ctx.Redisplay(edge.shp, true);
  }
  else
  {
    edge.shp = new Sketch_AIS_edge(*this, edge_shape);
    update_edge_style_(edge.shp);
    m_ctx.Display(edge.shp, true);
  }
}

void Sketch::on_enter() { m_tools.on_enter(); }

void Sketch::update_faces_() { m_topo.update_faces(); }

void Sketch::add_edge_(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b) { m_edges.add_edge(pt_a, pt_b); }

void Sketch::add_edge_(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, Sketch_op_recorder& rec)
{
  m_edges.add_edge(pt_a, pt_b, rec);
}

void Sketch::add_edge_raw_(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b) { m_edges.add_edge_raw_(pt_a, pt_b); }

void Sketch::sketch_json_add_linear_edge_(size_t idx_a, size_t idx_b, std::optional<size_t> idx_mid)
{
  m_edges.sketch_json_add_linear_edge(idx_a, idx_b, idx_mid);
}

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

std::vector<Sketch::Edge> Sketch::get_selected_edges_() const { return m_edges.get_selected(); }

std::vector<Sketch_face_shp_ptr> Sketch::get_selected_faces_() const
{
  std::vector<Sketch_face_shp_ptr> ret;
  for (const AIS_Shape_ptr& selected : m_view.get_selected())
    for (const Sketch_face_shp_ptr& face : m_topo.faces())
      if (face == selected)
        ret.push_back(face);

  return ret;
}

std::list<Sketch::Edge>::iterator Sketch::get_edge_at_(const ScreenCoords& screen_coords)
{
  return m_edges.get_at(screen_coords);
}

bool Sketch::try_remove_length_dimension(PrsDim_LengthDimension* dim) { return m_dims.try_remove_length_dimension(dim); }

void Sketch::toggle_edge_dim_anno(const ScreenCoords& screen_coords) { m_dims.toggle_edge_dim_anno(screen_coords); }

void Sketch::json_add_length_dimension_(size_t node_a, size_t node_b, const bool visible,
                                        const std::optional<double> flyout_offset, const std::string& name)
{
  m_dims.json_add_length_dimension_(node_a, node_b, visible, flyout_offset, name);
}

void Sketch::remove_length_dimensions_referencing_node_(size_t node_idx)
{
  m_dims.remove_length_dimensions_referencing_node_(node_idx);
}

void Sketch::set_show_dims(bool show) { m_dims.set_show_dims(show); }

bool Sketch::shows_dimensions() const { return m_dims.shows_dimensions(); }

bool Sketch::dimension_visible(size_t dim_index) const { return m_dims.dimension_visible(dim_index); }

void Sketch::set_dimension_visible(size_t dim_index, bool visible) { m_dims.set_dimension_visible(dim_index, visible); }

size_t Sketch::dimension_node_lo(size_t dim_index) const { return m_dims.dimension_node_lo(dim_index); }

size_t Sketch::dimension_node_hi(size_t dim_index) const { return m_dims.dimension_node_hi(dim_index); }

double Sketch::dimension_offset(size_t dim_index) const { return m_dims.dimension_offset(dim_index); }

void Sketch::set_dimension_offset(size_t dim_index, const double offset) { m_dims.set_dimension_offset(dim_index, offset); }

std::string Sketch::dimension_name(size_t dim_index) const { return m_dims.dimension_name(dim_index); }

void Sketch::set_dimension_name(size_t dim_index, const std::string& name) { m_dims.set_dimension_name(dim_index, name); }

PrsDim_LengthDimension_ptr Sketch::length_dimension_handle(const size_t dim_index) const
{
  return m_dims.length_dimension_handle(dim_index);
}

void Sketch::refresh_annotations(const Sketch_annotation_refresh& refresh)
{
  if (refresh.length_dimensions)
    m_dims.refresh_all_length_dimensions();

  if (refresh.permanent_node_marks)
    m_node_marks.sync();
}

size_t Sketch::length_dimension_count() const { return m_dims.length_dimension_count(); }

std::vector<std::string> Sketch::inspector_dimension_labels() const { return m_dims.inspector_dimension_labels(); }

std::vector<std::string> Sketch::inspector_node_labels() const
{
  std::vector<std::string> labels;
  for (size_t i = 0; i < m_nodes.size(); ++i)
  {
    const Sketch_nodes::Node& n = m_nodes[i];
    if (n.permanent && !n.deleted)
    {
      std::string lbl = n.name.empty() ? ("N" + std::to_string(i)) : n.name;
      labels.push_back(std::move(lbl));
    }
  }

  return labels;
}

void Sketch::finalize_elm() { m_tools.finalize(); }

bool Sketch::cancel_elm()
{
  const bool operation_canceled = m_tools.cancel();
  m_dims.clear_pick_state();
  m_dims.clear_tmp_dim_anno();
  m_nodes.hide_snap_annos();
  m_nodes.cancel();

  m_node_marks.trim_trailing();

  return operation_canceled;
}
bool Sketch::s_add_mid_pt_edges = false;
bool Sketch::s_edge_from_center = false;

void Sketch::set_add_mid_pt_edges(bool on) { s_add_mid_pt_edges = on; }

bool Sketch::get_add_mid_pt_edges() { return s_add_mid_pt_edges; }

void Sketch::set_edge_from_center(bool on) { s_edge_from_center = on; }

bool Sketch::get_edge_from_center() { return s_edge_from_center; }

void Sketch::update_edge_end_pt_(Edge& edge, size_t end_pt_idx) { m_edges.update_end_pt(edge, end_pt_idx); }

void Sketch::add_arc_circle_(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, const gp_Pnt2d& pt_c)
{
  const size_t node_idx_a = m_nodes.get_node_exact(pt_a);
  const size_t node_idx_c = m_nodes.get_node_exact(pt_c);
  const size_t node_idx_b = m_nodes.get_node_exact(pt_b);
  add_arc_circle_(std::vector<size_t>{node_idx_a, node_idx_c, node_idx_b});
}

void Sketch::add_arc_circle_(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, const gp_Pnt2d& pt_c, Sketch_op_recorder& rec)
{
  const size_t node_idx_a = m_nodes.get_node_exact(pt_a);
  const size_t node_idx_c = m_nodes.get_node_exact(pt_c);
  const size_t node_idx_b = m_nodes.get_node_exact(pt_b);

  rec.note_curr_arc_edge(pt_a, pt_b, pt_c);
  rec.note_curr_node(node_idx_a);
  rec.note_curr_node(node_idx_c);
  rec.note_curr_node(node_idx_b);

  add_arc_circle_(std::vector<size_t>{node_idx_a, node_idx_c, node_idx_b});
}

void Sketch::add_arc_circle_(const std::vector<size_t>& node_idxs) { m_edges.add_arc_circle_edges(node_idxs); }

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

  std::vector<std::pair<gp_Pnt2d, gp_Pnt2d>>     mirrored_edges;
  std::map<AIS_Shape_ptr, std::set<const Edge*>> arc_circles;

  for (const Edge& e : m_edges.edges())
    for (const Edge& selected : mirror_edges)
      if (e.shp == selected.shp || e.circle_arc == selected.circle_arc)
      {
        if (e.circle_arc)
        {
          std::set<const Edge*>& arc_circle_edges = arc_circles[e.shp];
          arc_circle_edges.insert(&e);
          if (arc_circle_edges.size() == 2)
            break;
        }
        else
        {
          const gp_Pnt2d a = mirror_point(mirror_pt_a, mirror_pt_b, m_nodes[e.node_idx_a]);
          const gp_Pnt2d b = mirror_point(mirror_pt_a, mirror_pt_b, m_nodes[e.node_idx_b]);
          mirrored_edges.push_back({a, b});
          break;
        }
      }

  for (const auto& [a, b] : mirrored_edges)
    add_edge_(a, b, rec);

  for (auto& [_, arc_circle_edges] : arc_circles)
  {
    EZY_ASSERT(arc_circle_edges.size() == 2);
    const Edge* a = nullptr;
    const Edge* b = nullptr;
    for (const Edge* e : arc_circle_edges)
      if (e->node_idx_arc.has_value())
        a = e;
      else
        b = e;

    EZY_ASSERT(a && b);
    EZY_ASSERT(a->node_idx_arc.has_value());
    EZY_ASSERT(b->node_idx_b);
    const gp_Pnt2d pt_a = mirror_point(mirror_pt_a, mirror_pt_b, m_nodes[a->node_idx_a]);
    const gp_Pnt2d pt_b = mirror_point(mirror_pt_a, mirror_pt_b, m_nodes[a->node_idx_arc]);
    const gp_Pnt2d pt_c = mirror_point(mirror_pt_a, mirror_pt_b, m_nodes[b->node_idx_b]);
    add_arc_circle_(pt_a, pt_b, pt_c, rec);
  }

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
      // Arc circles share the same shape
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
    // Catch OCCT exception and return error with message
    std::string error_msg = "Revolution failed: ";
    const char* msg       = e.GetMessageString();
    error_msg += msg ? msg : "Unknown OCCT error";
    return Shp_rslt(Result_status::Topo_error, error_msg);
  }
  catch (const std::exception& e)
  {
    // Catch other unexpected errors
    return Shp_rslt(Result_status::User_error, "Unexpected error: " + std::string(e.what()));
  }
}

void Sketch::on_mode()
{
  // Reset state
  cancel_elm();
  sync_operation_axis_display_();
  m_node_marks.sync();
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

Mode Sketch::get_mode() const { return m_view.get_mode(); }

const gp_Pln& Sketch::get_plane() const { return m_pln; }

Sketch_nodes& Sketch::get_nodes() { return m_nodes; }

void Sketch::get_snap_pts_3d_(std::vector<gp_Pnt>& out)
{
  out.clear();
  get_originating_face_snp_pts_3d_(out);
  m_nodes.get_snap_pts_3d(out);
}

void Sketch::get_originating_face_snp_pts_3d_(std::vector<gp_Pnt>& out)
{
  if (!m_originating_face)
    return;

  std::vector<gp_Pnt> snp_pts;
  const TopoDS_Shape& shape = m_originating_face->Shape();
  EZY_ASSERT(shape.ShapeType() == TopAbs_WIRE);
  const TopoDS_Wire& wire = TopoDS::Wire(shape);
  TopExp_Explorer    edgeExplorer(wire, TopAbs_EDGE);
  while (edgeExplorer.More())
  {
    double             first, last;
    const TopoDS_Edge& edge  = TopoDS::Edge(edgeExplorer.Current());
    Geom_Curve_ptr     curve = BRep_Tool::Curve(edge, first, last);
    EZY_ASSERT(curve);

    TopoDS_Vertex v1, v2;
    TopExp::Vertices(edge, v1, v2);
    gp_Pnt p1 = BRep_Tool::Pnt(v1);
    gp_Pnt pc = curve->Value((first + last) * 0.5);
    gp_Pnt p2 = BRep_Tool::Pnt(v2);

    if (snp_pts.empty() || !snp_pts.back().IsEqual(p1, Precision::Confusion()))
      snp_pts.push_back(p1);

    snp_pts.push_back(pc);

    if (!snp_pts.front().IsEqual(p2, Precision::Confusion()))
      snp_pts.push_back(p2);

    edgeExplorer.Next();
  }

  append(out, snp_pts);
}
void Sketch::set_visible(bool state)
{
  m_visible = state;

  if (state)
  {
    if (m_show_faces)
      for (AIS_Shape_ptr& face : m_topo.faces())
        m_ctx.Display(face, AIS_Shaded, 0, false);

    for (Edge& e : m_edges.edges())
      m_ctx.Display(e.shp, AIS_WireFrame, 0, false);

    m_dims.on_sketch_shown();

    if (m_originating_face)
      m_ctx.Display(m_originating_face, AIS_WireFrame, 0, false);

    if (m_underlay.has_image())
      m_underlay.rebuild_and_display(m_pln);

    sync_operation_axis_display_();
    m_node_marks.sync();
  }
  else
  {
    for (AIS_Shape_ptr& face : m_topo.faces())
      m_ctx.Erase(face, false);

    for (Edge& e : m_edges.edges())
      m_ctx.Erase(e.shp, false);

    m_dims.on_sketch_hidden();

    if (m_originating_face)
      m_ctx.Erase(m_originating_face, false);

    m_underlay.ctx_erase();

    if (m_operation_axis.has_value())
      m_ctx.Erase(m_operation_axis->shp, false);

    m_nodes.hide_snap_annos();
    cancel_elm();

    m_node_marks.erase_all();
  }

  m_ctx.UpdateCurrentViewer();

  m_view.curr_sketch().set_current();
}

bool Sketch::is_visible() const { return m_visible; }

void Sketch::set_show_faces(bool show)
{
  if (show && m_visible)
    for (AIS_Shape_ptr& face : m_topo.faces())
      m_ctx.Display(face, AIS_Shaded, 0, false);
  else
    for (AIS_Shape_ptr& face : m_topo.faces())
      m_ctx.Erase(face, false);

  m_show_faces = show;
}

void Sketch::set_show_edges(bool show)
{
  if (show && m_visible)
    for (Edge& e : m_edges.edges())
      m_ctx.Display(e.shp, AIS_WireFrame, 0, false);
  else
    for (Edge& e : m_edges.edges())
      m_ctx.Erase(e.shp, false);
}
void Sketch::append_list_hover_ais(std::vector<AIS_InteractiveObject_ptr>& out) const
{
  if (!m_visible)
    return;

  for (const Edge& e : m_edges.edges())
    if (!e.shp.IsNull())
      out.push_back(e.shp);

  if (m_show_faces)
    for (const Sketch_face_shp_ptr& face : m_topo.faces())
      if (!face.IsNull())
        out.push_back(face);

  if (m_originating_face)
    out.push_back(m_originating_face);

  if (m_operation_axis.has_value() && !m_operation_axis->shp.IsNull())
    out.push_back(m_operation_axis->shp);

  if (m_underlay.has_image() && m_underlay.visible())
    m_underlay.append_list_hover_ais(out);
}
bool Sketch::is_current() const { return this == &m_view.curr_sketch(); }

void Sketch::set_current()
{
  m_ctx.CurrentViewer()->SetPrivilegedPlane(m_pln.Position());
  set_edge_style(Edge_style::Full);
  m_nodes.clear_outside_snap_pnts();

  m_tmp_pts_3d.clear();
  get_originating_face_snp_pts_3d_(m_tmp_pts_3d);

  for (const gp_Pnt& pt3d : m_tmp_pts_3d)
    // Insert 2D points on this `m_pln`, points will be considered the same using `Precision::Confusion()`
    m_nodes.add_outside_snap_pnt(pt3d);

  for (Sketch_ptr& sketch : m_view.get_sketches())
    if (sketch.get() != this)
    {
      sketch->set_edge_style(Edge_style::Background);

      // Hide operation axis from other sketches
      if (sketch->m_operation_axis.has_value())
        m_ctx.Erase(sketch->m_operation_axis->shp, false);

      if (sketch->is_visible())
      {
        // Add snap points from other sketches, but only if they are visible.
        sketch->get_snap_pts_3d_(m_tmp_pts_3d);
        for (const gp_Pnt& pt3d : m_tmp_pts_3d)
          // Insert 2D points on this `m_pln`, points will be considered the same using `Precision::Confusion()`
          m_nodes.add_outside_snap_pnt(pt3d);
      }
    }

  sync_operation_axis_display_();

  // DBG_MSG("m_outside_snap_pts " << m_nodes.m_outside_snap_pts.size());
}

void Sketch::set_edge_style(Edge_style style)
{
  m_edge_style = style;

  for (Edge& e : m_edges.edges())
    update_edge_style_(e.shp);

  // Permanent node marker style/visibility is managed elsewhere; avoid
  // rebuilding marker geometry during sketch switching.
  update_originating_face_style();
}

gp_Pnt Sketch::to_3d_(size_t node_idx) const { return to_3d(m_pln, m_nodes[node_idx]); }

gp_Pnt Sketch::to_3d_(const std::optional<size_t>& node_idx) const
{
  EZY_ASSERT(node_idx.has_value());
  return to_3d_(*node_idx);
}

const std::string& Sketch::get_name() const { return m_name; }

void Sketch::set_name(const std::string& name) { m_name = name; }

bool Sketch::has_edges() const { return !m_edges.empty(); }

size_t Sketch::edge_count() const { return m_edges.size(); }

size_t Sketch::face_count() const { return m_topo.faces().size(); }

std::vector<std::string> Sketch::inspector_edge_labels() const
{
  std::vector<std::string> labels;
  labels.reserve(m_edges.size());
  size_t idx = 0;
  for (const Edge& e : m_edges.edges())
  {
    std::string lbl = e.name.empty() ? ("E" + std::to_string(idx)) : e.name;
    labels.push_back(std::move(lbl));
    ++idx;
  }

  return labels;
}

std::vector<std::string> Sketch::inspector_face_labels() const
{
  std::vector<std::string> labels;
  labels.reserve(m_topo.faces().size());
  for (size_t i = 0; i < m_topo.faces().size(); ++i)
  {
    const Sketch_face_shp_ptr& f   = m_topo.faces()[i];
    std::string                lbl = (f && !f->name.empty()) ? f->name : ("F" + std::to_string(i));
    labels.push_back(std::move(lbl));
  }

  return labels;
}

void Sketch::remove_permanent_node_mark(Sketch_AIS_node_mark& mark)
{
  EZY_ASSERT(&mark.owner_sketch == this);
  const size_t i = mark.node_idx;
  EZY_ASSERT(i < m_nodes.size());
  m_nodes[i].deleted = true;
  remove_length_dimensions_referencing_node_(i);
  m_node_marks.remove_at(i);

  m_ctx.UpdateCurrentViewer();
}