#include "sketch.h"

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRep_Tool.hxx>
#include <Precision.hxx>
#include <PrsDim_LengthDimension.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>
#include <functional>

#include "utl_geom.h"
#include "mode.h"
#include "gui_occt_view.h"
#include "sketch_op_recorder.h"
#include "utl.h"

using namespace glm;

Sketch::Sketch(const std::string& name, Occt_view& view, const gp_Pln& pln)
    : m_view(view)
    , m_ctx(view.ctx())
    , m_pln(pln)
    , m_name(name)
    , m_id(view.allocate_sketch_id())
    , m_nodes(view, pln)
    , m_edges(*this)
    , m_topo(*this)
    , m_dims(*this)
    , m_node_marks(*this)
    , m_tools(*this)
    , m_underlay(view.ctx())
{
  ensure_origin_node_();
}

Sketch::Sketch(const std::string& name, Occt_view& view, const gp_Pln& pln, const TopoDS_Wire& outer_wire)
    : m_view(view)
    , m_ctx(view.ctx())
    , m_pln(pln)
    , m_name(name)
    , m_id(view.allocate_sketch_id())
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
  ensure_origin_node_();
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

void Sketch::add_linear_edge(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b) { add_edge_(pt_a, pt_b); }

void Sketch::rebuild_faces() { update_faces_(); }

void Sketch::add_edge_(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, Sketch_op_recorder& rec)
{
  m_edges.add_edge(pt_a, pt_b, rec);
}

void Sketch::add_edge_raw_(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b) { m_edges.add_edge_raw_(pt_a, pt_b); }

void Sketch::sketch_json_add_linear_edge_(size_t idx_a, size_t idx_b, std::optional<size_t> idx_mid)
{
  m_edges.sketch_json_add_linear_edge(idx_a, idx_b, idx_mid);
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

  if (refresh.edge_face_style)
  {
    set_edge_style(m_edge_style);
    m_ctx.UpdateCurrentViewer();
  }
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
      std::string lbl;
      if (n.origin)
        lbl = n.name.empty() ? "Origin" : n.name;
      else
        lbl = n.name.empty() ? ("N" + std::to_string(i)) : n.name;

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

  add_arc_circle_(std::vector<size_t>{node_idx_a, node_idx_c, node_idx_b}, rec);
}

void Sketch::add_arc_circle_(const std::vector<size_t>& node_idxs) { m_edges.add_arc_circle_edges(node_idxs, nullptr); }

void Sketch::add_arc_circle_(const std::vector<size_t>& node_idxs, Sketch_op_recorder& rec)
{
  m_edges.add_arc_circle_edges(node_idxs, &rec);
}

void Sketch::on_mode()
{
  // Reset state
  cancel_elm();
  sync_operation_axis_display_();
  m_node_marks.sync();
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

void Sketch::ensure_origin_node_()
{
  if (origin_node_idx_().has_value())
  {
    // Commit baseline so cancel_elm() (e.g. from on_mode) does not remove the origin.
    m_nodes.finalize();
    return;
  }

  const gp_Pnt2d      pt  = default_origin_pt_();
  const size_t        idx = m_nodes.add_new_node(pt, false, true);
  Sketch_nodes::Node& n   = m_nodes[idx];
  n.origin                = true;
  n.name                  = "Origin";
  m_nodes.finalize();
}

std::optional<size_t> Sketch::origin_node_idx_() const
{
  for (size_t i = 0; i < m_nodes.size(); ++i)
    if (!m_nodes[i].deleted && m_nodes[i].origin)
      return i;

  return std::nullopt;
}

gp_Pnt2d Sketch::default_origin_pt_() const
{
  gp_Pnt2d pt(0., 0.);
  if (m_originating_face)
  {
    const TopoDS_Shape& shape = m_originating_face->Shape();
    if (shape.ShapeType() == TopAbs_WIRE)
      pt = to_2d(m_pln, get_shape_bbox_center(shape));
  }

  return pt;
}

gp_Pnt2d Sketch::default_origin_pt() const { return default_origin_pt_(); }

gp_Pnt2d Sketch::origin_pt() const
{
  if (const std::optional<size_t> idx = origin_node_idx_())
    return gp_Pnt2d(m_nodes[*idx].X(), m_nodes[*idx].Y());

  return default_origin_pt_();
}

void Sketch::set_origin_pt(const gp_Pnt2d& pt)
{
  ensure_origin_node_();
  const std::optional<size_t> idx = origin_node_idx_();
  EZY_ASSERT(idx.has_value());

  Sketch_nodes::Node& n = m_nodes[*idx];
  n.SetX(pt.X());
  n.SetY(pt.Y());
  m_node_marks.sync();
}

void Sketch::reset_origin_pt() { set_origin_pt(default_origin_pt_()); }

bool Sketch::show_origin_marker() const { return m_show_origin_marker; }

void Sketch::set_show_origin_marker(bool show)
{
  if (m_show_origin_marker == show)
    return;

  m_show_origin_marker = show;
  m_nodes.set_origin_snap_enabled(show);
  m_node_marks.sync();
  if (Sketch* cur = m_view.current_sketch_if_any())
    cur->set_current();
}

gp_Pnt Sketch::to_3d_(size_t node_idx) const { return to_3d(m_pln, m_nodes[node_idx]); }

gp_Pnt Sketch::to_3d_(const std::optional<size_t>& node_idx) const
{
  EZY_ASSERT(node_idx.has_value());
  return to_3d_(*node_idx);
}

const std::string& Sketch::get_name() const { return m_name; }

void Sketch::set_name(const std::string& name) { m_name = name; }

size_t Sketch::get_id() const { return m_id; }

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

Sketch_face_shp_ptr Sketch::inspector_face(size_t index) const
{
  if (index >= m_topo.faces().size())
    return {};
  return m_topo.faces()[index];
}

void Sketch::remove_permanent_node_mark(Sketch_AIS_node_mark& mark)
{
  EZY_ASSERT(&mark.owner_sketch == this);
  const size_t i = mark.node_idx;
  EZY_ASSERT(i < m_nodes.size());
  if (m_nodes[i].origin)
    return;

  m_nodes[i].deleted = true;
  remove_length_dimensions_referencing_node_(i);
  m_node_marks.remove_at(i);

  m_ctx.UpdateCurrentViewer();
}