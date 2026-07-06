#include "sketch_delta.h"

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <Precision.hxx>
#include <algorithm>
#include <utility>

#include "utl_dbg.h"
#include "utl_geom.h"
#include "gui_occt_view.h"
#include "sketch.h"
#include "sketch_nodes.h"
#include "sketch_edge.h"

namespace
{

bool on_closed_segment_2d_(const gp_Pnt2d& p, const gp_Pnt2d& a, const gp_Pnt2d& b);
bool is_linear_sketch_edge_(const Sketch::Edge& e);
bool pts_equal_(const gp_Pnt2d& a, const gp_Pnt2d& b);

} // namespace

class Sketch_delta::Impl
{
public:
  friend class Sketch_op_recorder::Impl;

  struct Prev_edge_rec
  {
    gp_Pnt2d                pt_a;
    gp_Pnt2d                pt_b;
    std::optional<gp_Pnt2d> pt_mid;
    std::string             name;
  };

  struct Curr_linear_edge_record
  {
    gp_Pnt2d pt_a;
    gp_Pnt2d pt_b;
  };

  struct Arc_edge_record
  {
    gp_Pnt2d pt_a;
    gp_Pnt2d pt_b;
    gp_Pnt2d pt_c;
  };

  struct Length_dim_record
  {
    gp_Pnt2d              pt_lo;
    gp_Pnt2d              pt_hi;
    bool                  visible{true};
    std::optional<double> flyout_offset;
    std::string           name;
  };

  Sketch*                                m_sketch{nullptr};
  size_t                                 m_sketch_id{0};
  std::vector<Prev_edge_rec>             prev_linear_edges;
  std::vector<Curr_linear_edge_record>   curr_linear_edges;
  std::vector<Arc_edge_record>           prev_arc_edges;
  std::vector<Arc_edge_record>           curr_arc_edges;
  std::vector<gp_Pnt2d>                  curr_node_pts;
  std::vector<Length_dim_record>         prev_length_dims;
  std::vector<Length_dim_record>         curr_length_dims;
  std::optional<Prev_edge_rec>           prev_operation_axis;
  std::optional<Curr_linear_edge_record> curr_operation_axis;

  explicit Impl(size_t sketch_id);
  Impl(Sketch& sketch, size_t sketch_id);

  Sketch*                       resolve_sketch_(Occt_view& view) const;
  void                          apply_forward_(Sketch& sketch) const;
  void                          apply_reverse_(Sketch& sketch) const;
  std::unique_ptr<Sketch_delta> clone() const;

  static bool prev_linear_equal_(const Prev_edge_rec& x, const Prev_edge_rec& y);
  static bool curr_linear_equal_(const Curr_linear_edge_record& x, const Curr_linear_edge_record& y);
  static bool arc_equal_(const Arc_edge_record& x, const Arc_edge_record& y);
  static bool arc_edge_matches_record_(const Sketch& sketch, const Sketch::Edge& e, const Arc_edge_record& rec);
  static bool length_dim_equal_(const Length_dim_record& x, const Length_dim_record& y);
  static void remove_linear_edges_on_segment_(Sketch& sketch, const gp_Pnt2d& seg_a, const gp_Pnt2d& seg_b);
  static void remove_linear_edges_on_node_segment_(Sketch& sketch, const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b);
  static void remove_arc_edge_(Sketch& sketch, const Arc_edge_record& rec);
  static void remove_length_dim_(Sketch& sketch, const Length_dim_record& rec);
  static void tombstone_node_at_pt_(Sketch& sketch, const gp_Pnt2d& pt);
  static void restore_prev_linear_edge_(Sketch& sketch, const Prev_edge_rec& rec);
  static void restore_length_dim_(Sketch& sketch, const Length_dim_record& rec);
  static void restore_prev_operation_axis_(Sketch& sketch, const Prev_edge_rec& rec);
  static void capture_linear_edges_at_start_(Sketch& sketch, std::vector<Prev_edge_rec>& out);
  static bool linear_edge_at_op_start_(const std::vector<Prev_edge_rec>& at_start, const Prev_edge_rec& rec);
};

class Sketch_op_recorder::Impl
{
public:
  Occt_view&                                     m_view;
  Sketch&                                        m_sketch;
  Sketch_op_recorder*                            m_owner{nullptr};
  bool                                           m_active{true};
  bool                                           m_committed{false};
  std::vector<gp_Pnt2d>                          m_live_node_pts_at_start;
  std::vector<Sketch_delta::Impl::Prev_edge_rec> m_linear_edges_at_start;
  std::unique_ptr<Sketch_delta>                  m_delta;

  Impl(Occt_view& view, Sketch& sketch);

  void register_owner_(Sketch_op_recorder& owner);
  void on_destroy_();

  void note_prev_linear_edge(size_t node_idx_a, size_t node_idx_b, std::optional<size_t> node_idx_mid, const std::string& name);
  void note_curr_linear_edge(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b);
  void note_prev_arc_edge(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, const gp_Pnt2d& pt_c);
  void note_curr_arc_edge(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, const gp_Pnt2d& pt_c);
  void note_curr_node(size_t node_idx);
  void note_prev_length_dim(size_t lo, size_t hi, bool visible, std::optional<double> flyout, const std::string& name);
  void note_curr_length_dim(size_t lo, size_t hi, bool visible, std::optional<double> flyout, const std::string& name);
  void note_prev_operation_axis(size_t node_idx_a, size_t node_idx_b);
  void note_curr_operation_axis(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b);

  void commit();
  void cancel();

private:
  bool empty_() const;
  void unregister_owner_();
};

Sketch_delta::Sketch_delta(Sketch& sketch, const size_t sketch_id)
    : m_impl(std::make_unique<Impl>(sketch, sketch_id))
{
}

Sketch_delta::Sketch_delta(const size_t sketch_id)
    : m_impl(std::make_unique<Impl>(sketch_id))
{
}

Sketch_delta::~Sketch_delta() = default;

void Sketch_delta::apply_forward(Occt_view& view)
{
  Sketch* sketch = m_impl->resolve_sketch_(view);
  if (!sketch)
    return;

  m_impl->apply_forward_(*sketch);
}

void Sketch_delta::apply_reverse(Occt_view& view)
{
  Sketch* sketch = m_impl->resolve_sketch_(view);
  if (!sketch)
    return;

  m_impl->apply_reverse_(*sketch);
}

std::unique_ptr<Delta> Sketch_delta::clone() const { return m_impl->clone(); }

Sketch_op_recorder::Sketch_op_recorder(Occt_view& view, Sketch& sketch)
    : m_impl(std::make_unique<Impl>(view, sketch))
{
  m_impl->register_owner_(*this);
}

Sketch_op_recorder::~Sketch_op_recorder()
{
  if (m_impl)
    m_impl->on_destroy_();
}

void Sketch_op_recorder::note_prev_linear_edge(size_t node_idx_a, size_t node_idx_b, std::optional<size_t> node_idx_mid,
                                               const std::string& name)
{
  m_impl->note_prev_linear_edge(node_idx_a, node_idx_b, node_idx_mid, name);
}

void Sketch_op_recorder::note_curr_linear_edge(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b)
{
  m_impl->note_curr_linear_edge(pt_a, pt_b);
}

void Sketch_op_recorder::note_prev_arc_edge(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, const gp_Pnt2d& pt_c)
{
  m_impl->note_prev_arc_edge(pt_a, pt_b, pt_c);
}

void Sketch_op_recorder::note_curr_arc_edge(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, const gp_Pnt2d& pt_c)
{
  m_impl->note_curr_arc_edge(pt_a, pt_b, pt_c);
}

void Sketch_op_recorder::note_curr_node(size_t node_idx) { m_impl->note_curr_node(node_idx); }

void Sketch_op_recorder::note_prev_length_dim(size_t lo, size_t hi, bool visible, std::optional<double> flyout,
                                              const std::string& name)
{
  m_impl->note_prev_length_dim(lo, hi, visible, flyout, name);
}

void Sketch_op_recorder::note_curr_length_dim(size_t lo, size_t hi, bool visible, std::optional<double> flyout,
                                              const std::string& name)
{
  m_impl->note_curr_length_dim(lo, hi, visible, flyout, name);
}

void Sketch_op_recorder::note_prev_operation_axis(size_t node_idx_a, size_t node_idx_b)
{
  m_impl->note_prev_operation_axis(node_idx_a, node_idx_b);
}

void Sketch_op_recorder::note_curr_operation_axis(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b)
{
  m_impl->note_curr_operation_axis(pt_a, pt_b);
}

void Sketch_op_recorder::commit() { m_impl->commit(); }

void Sketch_op_recorder::cancel() { m_impl->cancel(); }

Sketch_op_recorder::Impl::Impl(Occt_view& view, Sketch& sketch)
    : m_view(view)
    , m_sketch(sketch)
{
  for (size_t i = 0, n = sketch.m_nodes.size(); i < n; ++i)
    if (!sketch.m_nodes[i].deleted)
      m_live_node_pts_at_start.push_back(sketch.m_nodes[i]);

  Sketch_delta::Impl::capture_linear_edges_at_start_(sketch, m_linear_edges_at_start);
  m_delta = std::make_unique<Sketch_delta>(sketch, sketch.get_id());
}

void Sketch_op_recorder::Impl::register_owner_(Sketch_op_recorder& owner) { m_owner = &owner; }

void Sketch_op_recorder::Impl::unregister_owner_() { m_owner = nullptr; }

void Sketch_op_recorder::Impl::on_destroy_()
{
  unregister_owner_();
  if (m_active && !m_committed)
    cancel();
}

void Sketch_op_recorder::Impl::note_prev_linear_edge(size_t node_idx_a, size_t node_idx_b, std::optional<size_t> node_idx_mid,
                                                     const std::string& name)
{
  if (!m_active || !m_delta)
    return;

  const gp_Pnt2d          pt_a = m_sketch.m_nodes[node_idx_a];
  const gp_Pnt2d          pt_b = m_sketch.m_nodes[node_idx_b];
  std::optional<gp_Pnt2d> pt_mid;
  if (node_idx_mid.has_value())
    pt_mid = m_sketch.m_nodes[*node_idx_mid];

  Sketch_delta::Impl::Prev_edge_rec rec{pt_a, pt_b, pt_mid, name};
  if (!Sketch_delta::Impl::linear_edge_at_op_start_(m_linear_edges_at_start, rec))
    return;

  Sketch_delta::Impl& d = *m_delta->m_impl;
  for (const Sketch_delta::Impl::Prev_edge_rec& x : d.prev_linear_edges)
    if (Sketch_delta::Impl::prev_linear_equal_(x, rec))
      return;

  d.prev_linear_edges.push_back(std::move(rec));
}

void Sketch_op_recorder::Impl::note_curr_linear_edge(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b)
{
  if (!m_active || !m_delta)
    return;

  Sketch_delta::Impl::Curr_linear_edge_record rec{pt_a, pt_b};
  for (const Sketch_delta::Impl::Curr_linear_edge_record& x : m_delta->m_impl->curr_linear_edges)
    if (Sketch_delta::Impl::curr_linear_equal_(x, rec))
      return;

  m_delta->m_impl->curr_linear_edges.push_back(rec);
}

void Sketch_op_recorder::Impl::note_prev_arc_edge(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, const gp_Pnt2d& pt_c)
{
  if (!m_active || !m_delta)
    return;

  Sketch_delta::Impl::Arc_edge_record rec{pt_a, pt_b, pt_c};
  for (const Sketch_delta::Impl::Arc_edge_record& x : m_delta->m_impl->prev_arc_edges)
    if (Sketch_delta::Impl::arc_equal_(x, rec))
      return;

  m_delta->m_impl->prev_arc_edges.push_back(rec);
}

void Sketch_op_recorder::Impl::note_curr_arc_edge(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, const gp_Pnt2d& pt_c)
{
  if (!m_active || !m_delta)
    return;

  Sketch_delta::Impl::Arc_edge_record rec{pt_a, pt_b, pt_c};
  for (const Sketch_delta::Impl::Arc_edge_record& x : m_delta->m_impl->curr_arc_edges)
    if (Sketch_delta::Impl::arc_equal_(x, rec))
      return;

  m_delta->m_impl->curr_arc_edges.push_back(rec);
}

void Sketch_op_recorder::Impl::note_curr_node(size_t node_idx)
{
  if (!m_active || !m_delta)
    return;

  const gp_Pnt2d pt = m_sketch.m_nodes[node_idx];

  for (const gp_Pnt2d& live : m_live_node_pts_at_start)
    if (pts_equal_(live, pt))
      return;

  std::vector<gp_Pnt2d>& nodes = m_delta->m_impl->curr_node_pts;
  for (const gp_Pnt2d& x : nodes)
    if (pts_equal_(x, pt))
      return;

  nodes.push_back(pt);
}

void Sketch_op_recorder::Impl::note_prev_length_dim(size_t lo, size_t hi, bool visible, std::optional<double> flyout,
                                                    const std::string& name)
{
  if (!m_active || !m_delta)
    return;

  Sketch_delta::Impl::Length_dim_record rec{m_sketch.m_nodes[lo], m_sketch.m_nodes[hi], visible, flyout, name};
  for (const Sketch_delta::Impl::Length_dim_record& x : m_delta->m_impl->prev_length_dims)
    if (Sketch_delta::Impl::length_dim_equal_(x, rec))
      return;

  m_delta->m_impl->prev_length_dims.push_back(std::move(rec));
}

void Sketch_op_recorder::Impl::note_curr_length_dim(size_t lo, size_t hi, bool visible, std::optional<double> flyout,
                                                    const std::string& name)
{
  if (!m_active || !m_delta)
    return;

  Sketch_delta::Impl::Length_dim_record rec{m_sketch.m_nodes[lo], m_sketch.m_nodes[hi], visible, flyout, name};
  for (const Sketch_delta::Impl::Length_dim_record& x : m_delta->m_impl->curr_length_dims)
    if (Sketch_delta::Impl::length_dim_equal_(x, rec))
      return;

  m_delta->m_impl->curr_length_dims.push_back(std::move(rec));
}

void Sketch_op_recorder::Impl::note_prev_operation_axis(size_t node_idx_a, size_t node_idx_b)
{
  if (!m_active || !m_delta)
    return;

  m_delta->m_impl->prev_operation_axis =
      Sketch_delta::Impl::Prev_edge_rec{m_sketch.m_nodes[node_idx_a], m_sketch.m_nodes[node_idx_b], std::nullopt, {}};
}

void Sketch_op_recorder::Impl::note_curr_operation_axis(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b)
{
  if (!m_active || !m_delta)
    return;

  m_delta->m_impl->curr_operation_axis = Sketch_delta::Impl::Curr_linear_edge_record{pt_a, pt_b};
}

bool Sketch_op_recorder::Impl::empty_() const
{
  if (!m_delta)
    return true;

  const Sketch_delta::Impl& d = *m_delta->m_impl;
  return d.prev_linear_edges.empty() && d.curr_linear_edges.empty() && d.prev_arc_edges.empty() && d.curr_arc_edges.empty() &&
         d.curr_node_pts.empty() && d.prev_length_dims.empty() && d.curr_length_dims.empty() &&
         !d.prev_operation_axis.has_value() && !d.curr_operation_axis.has_value();
}

void Sketch_op_recorder::Impl::commit()
{
  if (!m_active || m_committed)
    return;

  m_committed = true;
  m_active    = false;

  unregister_owner_();

  if (!empty_())
    m_view.push_undo_delta(std::move(m_delta));
}

void Sketch_op_recorder::Impl::cancel()
{
  m_active    = false;
  m_committed = true;

  unregister_owner_();
}

Sketch_delta::Impl::Impl(const size_t sketch_id)
    : m_sketch_id(sketch_id)
{
}

Sketch_delta::Impl::Impl(Sketch& sketch, const size_t sketch_id)
    : m_sketch(&sketch)
    , m_sketch_id(sketch_id)
{
}

Sketch* Sketch_delta::Impl::resolve_sketch_(Occt_view& view) const
{
  for (const Sketch::sptr& s : view.get_sketches())
    if (s->get_id() == m_sketch_id)
      return s.get();

  if (m_sketch)
  {
    for (const Sketch::sptr& s : view.get_sketches())
      if (s.get() == m_sketch)
        return m_sketch;
  }

  // Headless tests use a stack `Sketch` not registered in the view.
  return m_sketch;
}

void Sketch_delta::Impl::apply_forward_(Sketch& sketch) const
{
  for (const Curr_linear_edge_record& e : curr_linear_edges)
    sketch.add_edge_(e.pt_a, e.pt_b);

  for (const Arc_edge_record& e : curr_arc_edges)
    sketch.add_arc_circle_(e.pt_a, e.pt_b, e.pt_c);

  for (const Length_dim_record& d : curr_length_dims)
    restore_length_dim_(sketch, d);

  if (curr_operation_axis.has_value())
    sketch.sketch_json_set_operation_axis_(curr_operation_axis->pt_a, curr_operation_axis->pt_b);

  sketch.m_nodes.hide_snap_annos();
  sketch.update_faces_();
}

void Sketch_delta::Impl::apply_reverse_(Sketch& sketch) const
{
  if (curr_operation_axis.has_value())
    sketch.clear_operation_axis();

  for (const Length_dim_record& d : curr_length_dims)
    remove_length_dim_(sketch, d);

  for (const Arc_edge_record& e : curr_arc_edges)
    remove_arc_edge_(sketch, e);

  for (const Curr_linear_edge_record& e : curr_linear_edges)
    remove_linear_edges_on_segment_(sketch, e.pt_a, e.pt_b);

  for (const Prev_edge_rec& e : prev_linear_edges)
    restore_prev_linear_edge_(sketch, e);

  for (const Arc_edge_record& e : prev_arc_edges)
    sketch.add_arc_circle_(e.pt_a, e.pt_b, e.pt_c);

  if (prev_operation_axis.has_value())
    restore_prev_operation_axis_(sketch, *prev_operation_axis);

  for (const Length_dim_record& d : prev_length_dims)
    restore_length_dim_(sketch, d);

  for (const gp_Pnt2d& pt : curr_node_pts)
    tombstone_node_at_pt_(sketch, pt);

  sketch.m_nodes.hide_snap_annos();
  sketch.update_faces_();
}

std::unique_ptr<Sketch_delta> Sketch_delta::Impl::clone() const
{
  auto  copy                    = std::make_unique<Sketch_delta>(m_sketch_id);
  Impl& copy_impl               = *copy->m_impl;
  copy_impl.m_sketch            = m_sketch;
  copy_impl.prev_linear_edges   = prev_linear_edges;
  copy_impl.curr_linear_edges   = curr_linear_edges;
  copy_impl.prev_arc_edges      = prev_arc_edges;
  copy_impl.curr_arc_edges      = curr_arc_edges;
  copy_impl.curr_node_pts       = curr_node_pts;
  copy_impl.prev_length_dims    = prev_length_dims;
  copy_impl.curr_length_dims    = curr_length_dims;
  copy_impl.prev_operation_axis = prev_operation_axis;
  copy_impl.curr_operation_axis = curr_operation_axis;
  return copy;
}

bool Sketch_delta::Impl::prev_linear_equal_(const Prev_edge_rec& x, const Prev_edge_rec& y)
{
  if (!pts_equal_(x.pt_a, y.pt_a) || !pts_equal_(x.pt_b, y.pt_b))
    return false;

  if (x.pt_mid.has_value() != y.pt_mid.has_value())
    return false;

  if (x.pt_mid.has_value() && !pts_equal_(*x.pt_mid, *y.pt_mid))
    return false;

  return true;
}

bool Sketch_delta::Impl::curr_linear_equal_(const Curr_linear_edge_record& x, const Curr_linear_edge_record& y)
{
  return x.pt_a.SquareDistance(y.pt_a) <= Precision::SquareConfusion() &&
         x.pt_b.SquareDistance(y.pt_b) <= Precision::SquareConfusion();
}

bool Sketch_delta::Impl::arc_equal_(const Arc_edge_record& x, const Arc_edge_record& y)
{
  return x.pt_a.SquareDistance(y.pt_a) <= Precision::SquareConfusion() &&
         x.pt_b.SquareDistance(y.pt_b) <= Precision::SquareConfusion() &&
         x.pt_c.SquareDistance(y.pt_c) <= Precision::SquareConfusion();
}

bool Sketch_delta::Impl::arc_edge_matches_record_(const Sketch& sketch, const Sketch::Edge& e, const Arc_edge_record& rec)
{
  if (!sketch_edge_is_arc(e) || !e.node_idx_b.has_value() || e.shp.IsNull())
    return false;

  const gp_Pnt2d start = sketch.m_nodes[e.node_idx_a];
  const gp_Pnt2d end   = sketch.m_nodes[*e.node_idx_b];
  if (!pts_equal_(start, rec.pt_a) || !pts_equal_(end, rec.pt_c))
    return false;

  Geom_TrimmedCurve_ptr expected =
      GC_MakeArcOfCircle(to_3d(sketch.m_pln, rec.pt_a), to_3d(sketch.m_pln, rec.pt_b), to_3d(sketch.m_pln, rec.pt_c));
  if (!expected)
    return false;

  const gp_Pnt2d expected_mid = arc_curve_midpoint_2d(BRepBuilderAPI_MakeEdge(expected).Edge(), sketch.m_pln);
  const gp_Pnt2d actual_mid   = arc_curve_midpoint_2d(TopoDS::Edge(e.shp->Shape()), sketch.m_pln);
  return pts_equal_(expected_mid, actual_mid);
}

bool Sketch_delta::Impl::length_dim_equal_(const Length_dim_record& x, const Length_dim_record& y)
{
  return pts_equal_(x.pt_lo, y.pt_lo) && pts_equal_(x.pt_hi, y.pt_hi);
}

void Sketch_delta::Impl::remove_linear_edges_on_segment_(Sketch& sketch, const gp_Pnt2d& seg_a, const gp_Pnt2d& seg_b)
{
  for (auto itr = sketch.m_edges.edges().begin(); itr != sketch.m_edges.edges().end();)
  {
    if (!sketch_edge_is_linear(*itr))
    {
      ++itr;
      continue;
    }

    const gp_Pnt2d& pa = sketch.m_nodes[itr->node_idx_a];
    const gp_Pnt2d& pb = sketch.m_nodes[*itr->node_idx_b];
    if (on_closed_segment_2d_(pa, seg_a, seg_b) && on_closed_segment_2d_(pb, seg_a, seg_b))
    {
      sketch.m_ctx.Remove(itr->shp, false);
      itr = sketch.m_edges.edges().erase(itr);
    }
    else
      ++itr;
  }
}

void Sketch_delta::Impl::remove_linear_edges_on_node_segment_(Sketch& sketch, const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b)
{
  remove_linear_edges_on_segment_(sketch, pt_a, pt_b);
}

void Sketch_delta::Impl::remove_arc_edge_(Sketch& sketch, const Arc_edge_record& rec)
{
  for (auto itr = sketch.m_edges.edges().begin(); itr != sketch.m_edges.edges().end();)
  {
    if (!sketch_edge_is_arc(*itr))
    {
      ++itr;
      continue;
    }

    if (!arc_edge_matches_record_(sketch, *itr, rec))
    {
      ++itr;
      continue;
    }

    sketch.m_ctx.Remove(itr->shp, false);
    itr = sketch.m_edges.edges().erase(itr);
  }
}

void Sketch_delta::Impl::remove_length_dim_(Sketch& sketch, const Length_dim_record& rec)
{
  for (auto it = sketch.m_dims.dimensions().begin(); it != sketch.m_dims.dimensions().end(); ++it)
  {
    const gp_Pnt2d& lo = sketch.m_nodes[it->node_idx_lo];
    const gp_Pnt2d& hi = sketch.m_nodes[it->node_idx_hi];
    if (!pts_equal_(lo, rec.pt_lo) || !pts_equal_(hi, rec.pt_hi))
      continue;

    if (!it->dim.IsNull())
      sketch.m_ctx.Remove(it->dim, true);

    sketch.m_dims.dimensions().erase(it);
    return;
  }
}

void Sketch_delta::Impl::tombstone_node_at_pt_(Sketch& sketch, const gp_Pnt2d& pt)
{
  for (size_t i = 0, n = sketch.m_nodes.size(); i < n; ++i)
  {
    if (sketch.m_nodes[i].deleted)
      continue;

    if (!pts_equal_(sketch.m_nodes[i], pt))
      continue;

    sketch.m_nodes[i].deleted = true;
    sketch.remove_length_dimensions_referencing_node_(i);
    sketch.m_node_marks.remove_at(i);
    return;
  }
}

void Sketch_delta::Impl::restore_prev_linear_edge_(Sketch& sketch, const Prev_edge_rec& rec)
{
  remove_linear_edges_on_segment_(sketch, rec.pt_a, rec.pt_b);

  const size_t          idx_a = sketch.m_nodes.get_node_exact(rec.pt_a);
  const size_t          idx_b = sketch.m_nodes.get_node_exact(rec.pt_b);
  std::optional<size_t> idx_mid;
  if (rec.pt_mid.has_value())
    idx_mid = sketch.m_nodes.get_node_exact(*rec.pt_mid);

  sketch.sketch_json_add_linear_edge_(idx_a, idx_b, idx_mid);
  if (!rec.name.empty())
    for (Sketch::Edge& e : sketch.m_edges.edges())
      if (sketch_edge_is_linear(e) && pts_equal_(sketch.m_nodes[e.node_idx_a], rec.pt_a) &&
          pts_equal_(sketch.m_nodes[*e.node_idx_b], rec.pt_b) &&
          ((!rec.pt_mid.has_value() && !e.node_idx_mid.has_value()) ||
           (rec.pt_mid.has_value() && e.node_idx_mid.has_value() && pts_equal_(sketch.m_nodes[*e.node_idx_mid], *rec.pt_mid))))
      {
        e.name = rec.name;
        break;
      }
}

void Sketch_delta::Impl::restore_length_dim_(Sketch& sketch, const Length_dim_record& rec)
{
  const size_t lo = sketch.m_nodes.get_node_exact(rec.pt_lo);
  const size_t hi = sketch.m_nodes.get_node_exact(rec.pt_hi);
  sketch.json_add_length_dimension_(lo, hi, rec.visible, rec.flyout_offset, rec.name);
}

void Sketch_delta::Impl::restore_prev_operation_axis_(Sketch& sketch, const Prev_edge_rec& rec)
{
  sketch.sketch_json_set_operation_axis_(rec.pt_a, rec.pt_b);
}

void Sketch_delta::Impl::capture_linear_edges_at_start_(Sketch& sketch, std::vector<Prev_edge_rec>& out)
{
  for (const Sketch::Edge& e : sketch.m_edges.edges())
  {
    if (!is_linear_sketch_edge_(e))
      continue;

    Prev_edge_rec rec{sketch.m_nodes[e.node_idx_a], sketch.m_nodes[*e.node_idx_b], std::nullopt, e.name};
    if (e.node_idx_mid.has_value())
      rec.pt_mid = sketch.m_nodes[*e.node_idx_mid];
    if (std::find_if(out.begin(), out.end(), [&](const Prev_edge_rec& x) { return prev_linear_equal_(x, rec); }) == out.end())
      out.push_back(std::move(rec));
  }
}

bool Sketch_delta::Impl::linear_edge_at_op_start_(const std::vector<Prev_edge_rec>& at_start, const Prev_edge_rec& rec)
{
  return std::find_if(at_start.begin(), at_start.end(), [&](const Prev_edge_rec& x) { return prev_linear_equal_(x, rec); }) !=
         at_start.end();
}

namespace
{

bool on_closed_segment_2d_(const gp_Pnt2d& p, const gp_Pnt2d& a, const gp_Pnt2d& b)
{
  if (p.SquareDistance(a) <= Precision::SquareConfusion())
    return true;

  if (p.SquareDistance(b) <= Precision::SquareConfusion())
    return true;

  return point_on_open_segment_2d(p, a, b);
}

bool is_linear_sketch_edge_(const Sketch::Edge& e) { return sketch_edge_is_linear(e); }

bool pts_equal_(const gp_Pnt2d& a, const gp_Pnt2d& b) { return a.SquareDistance(b) <= Precision::SquareConfusion(); }

} // namespace
