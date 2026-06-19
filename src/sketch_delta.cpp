#include "sketch_delta.h"

#include <Precision.hxx>
#include <algorithm>
#include <utility>

#include "dbg.h"
#include "geom.h"
#include "occt_view.h"
#include "sketch.h"
#include "sketch_nodes.h"

namespace
{

// Forward declarations; definitions are at the bottom of this file (reader-first order).
bool on_closed_segment_2d_(const gp_Pnt2d& p, const gp_Pnt2d& a, const gp_Pnt2d& b);
bool prev_linear_equal_(const Sketch_delta::Prev_edge_rec& x, const Sketch_delta::Prev_edge_rec& y);
bool curr_linear_equal_(const Sketch_delta::Curr_linear_edge_record& x, const Sketch_delta::Curr_linear_edge_record& y);
bool arc_equal_(const Sketch_delta::Arc_edge_record& x, const Sketch_delta::Arc_edge_record& y);
bool length_dim_equal_(const Sketch_delta::Length_dim_record& x, const Sketch_delta::Length_dim_record& y);
void remove_linear_edges_on_segment_(Sketch& sketch, const gp_Pnt2d& seg_a, const gp_Pnt2d& seg_b);
void remove_linear_edges_on_node_segment_(Sketch& sketch, size_t node_a, size_t node_b);
void remove_arc_edge_(Sketch& sketch, const Sketch_delta::Arc_edge_record& rec);
void remove_length_dim_(Sketch& sketch, const Sketch_delta::Length_dim_record& rec);
void tombstone_node_(Sketch& sketch, size_t node_idx);
void restore_prev_linear_edge_(Sketch& sketch, const Sketch_delta::Prev_edge_rec& rec);
void restore_length_dim_(Sketch& sketch, const Sketch_delta::Length_dim_record& rec);
void restore_prev_operation_axis_(Sketch& sketch, const Sketch_delta::Prev_edge_rec& rec);
bool is_linear_sketch_edge_(const Sketch::Edge& e);
void capture_linear_edges_at_start_(Sketch& sketch, std::vector<Sketch_delta::Prev_edge_rec>& out);
bool linear_edge_at_op_start_(const std::vector<Sketch_delta::Prev_edge_rec>& at_start, const Sketch_delta::Prev_edge_rec& rec);

} // namespace

class Sketch_delta::Impl
{
public:
  friend class Sketch_op_recorder;

  Sketch*                                        m_sketch{nullptr};
  std::string                                    m_sketch_name;
  std::vector<Sketch_delta::Prev_edge_rec>       prev_linear_edges;
  std::vector<Sketch_delta::Curr_linear_edge_record> curr_linear_edges;
  std::vector<Sketch_delta::Arc_edge_record>     prev_arc_edges;
  std::vector<Sketch_delta::Arc_edge_record>     curr_arc_edges;
  std::vector<size_t>                            curr_node_idxs;
  std::vector<Sketch_delta::Length_dim_record>   prev_length_dims;
  std::vector<Sketch_delta::Length_dim_record>   curr_length_dims;
  std::optional<Sketch_delta::Prev_edge_rec>     prev_operation_axis;
  std::optional<Sketch_delta::Curr_linear_edge_record> curr_operation_axis;

  Impl(Sketch& sketch, std::string sketch_name);

  Sketch* resolve_sketch_(Occt_view& view) const;
  void    apply_forward_(Sketch& sketch) const;
  void    apply_reverse_(Sketch& sketch) const;
  std::unique_ptr<Sketch_delta> clone() const;
};

Sketch_delta::Sketch_delta(Sketch& sketch, std::string sketch_name)
    : m_impl(std::make_unique<Impl>(sketch, std::move(sketch_name)))
{
}

Sketch_delta::~Sketch_delta() = default;

void Sketch_delta::apply_forward(Occt_view& view)
{
  Sketch* sketch = m_impl->resolve_sketch_(view);
  EZY_ASSERT(sketch);
  m_impl->apply_forward_(*sketch);
}

void Sketch_delta::apply_reverse(Occt_view& view)
{
  Sketch* sketch = m_impl->resolve_sketch_(view);
  EZY_ASSERT(sketch);
  m_impl->apply_reverse_(*sketch);
}

std::unique_ptr<Delta> Sketch_delta::clone() const { return m_impl->clone(); }

Sketch_op_recorder::Sketch_op_recorder(Occt_view& view, Sketch& sketch)
    : m_view(view)
    , m_sketch(sketch)
{
  for (size_t i = 0, n = sketch.m_nodes.size(); i < n; ++i)
    if (!sketch.m_nodes[i].deleted)
      m_live_nodes_at_start.insert(i);

  capture_linear_edges_at_start_(sketch, m_linear_edges_at_start);

  m_delta                = std::make_unique<Sketch_delta>(sketch, sketch.get_name());
  sketch.m_undo_recorder = this;
}

Sketch_op_recorder::~Sketch_op_recorder()
{
  if (m_sketch.m_undo_recorder == this)
    m_sketch.m_undo_recorder = nullptr;

  if (m_active && !m_committed)
    cancel();
}

void Sketch_op_recorder::note_prev_linear_edge(size_t node_idx_a, size_t node_idx_b, std::optional<size_t> node_idx_mid,
                                               const std::string& name)
{
  if (!m_active || !m_delta)
    return;

  Sketch_delta::Prev_edge_rec rec{node_idx_a, node_idx_b, node_idx_mid, name};
  if (!linear_edge_at_op_start_(m_linear_edges_at_start, rec))
    return;

  Sketch_delta::Impl& d = *m_delta->m_impl;
  for (const Sketch_delta::Prev_edge_rec& x : d.prev_linear_edges)
    if (prev_linear_equal_(x, rec))
      return;

  d.prev_linear_edges.push_back(std::move(rec));
}

void Sketch_op_recorder::note_curr_linear_edge(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b)
{
  if (!m_active || !m_delta)
    return;

  Sketch_delta::Curr_linear_edge_record rec{pt_a, pt_b};
  for (const Sketch_delta::Curr_linear_edge_record& x : m_delta->m_impl->curr_linear_edges)
    if (curr_linear_equal_(x, rec))
      return;

  m_delta->m_impl->curr_linear_edges.push_back(rec);
}

void Sketch_op_recorder::note_prev_arc_edge(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, const gp_Pnt2d& pt_c)
{
  if (!m_active || !m_delta)
    return;

  Sketch_delta::Arc_edge_record rec{pt_a, pt_b, pt_c};
  for (const Sketch_delta::Arc_edge_record& x : m_delta->m_impl->prev_arc_edges)
    if (arc_equal_(x, rec))
      return;

  m_delta->m_impl->prev_arc_edges.push_back(rec);
}

void Sketch_op_recorder::note_curr_arc_edge(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, const gp_Pnt2d& pt_c)
{
  if (!m_active || !m_delta)
    return;

  Sketch_delta::Arc_edge_record rec{pt_a, pt_b, pt_c};
  for (const Sketch_delta::Arc_edge_record& x : m_delta->m_impl->curr_arc_edges)
    if (arc_equal_(x, rec))
      return;

  m_delta->m_impl->curr_arc_edges.push_back(rec);
}

void Sketch_op_recorder::note_curr_node(size_t node_idx)
{
  if (!m_active || !m_delta)
    return;

  if (m_live_nodes_at_start.count(node_idx))
    return;

  std::vector<size_t>& nodes = m_delta->m_impl->curr_node_idxs;
  if (std::find(nodes.begin(), nodes.end(), node_idx) != nodes.end())
    return;

  nodes.push_back(node_idx);
}

void Sketch_op_recorder::note_prev_length_dim(size_t lo, size_t hi, bool visible, std::optional<double> flyout,
                                              const std::string& name)
{
  if (!m_active || !m_delta)
    return;

  Sketch_delta::Length_dim_record rec{lo, hi, visible, flyout, name};
  for (const Sketch_delta::Length_dim_record& x : m_delta->m_impl->prev_length_dims)
    if (length_dim_equal_(x, rec))
      return;

  m_delta->m_impl->prev_length_dims.push_back(std::move(rec));
}

void Sketch_op_recorder::note_curr_length_dim(size_t lo, size_t hi, bool visible, std::optional<double> flyout,
                                              const std::string& name)
{
  if (!m_active || !m_delta)
    return;

  Sketch_delta::Length_dim_record rec{lo, hi, visible, flyout, name};
  for (const Sketch_delta::Length_dim_record& x : m_delta->m_impl->curr_length_dims)
    if (length_dim_equal_(x, rec))
      return;

  m_delta->m_impl->curr_length_dims.push_back(std::move(rec));
}

void Sketch_op_recorder::note_prev_operation_axis(size_t node_idx_a, size_t node_idx_b)
{
  if (!m_active || !m_delta)
    return;

  m_delta->m_impl->prev_operation_axis = Sketch_delta::Prev_edge_rec{node_idx_a, node_idx_b, std::nullopt, {}};
}

void Sketch_op_recorder::note_curr_operation_axis(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b)
{
  if (!m_active || !m_delta)
    return;

  m_delta->m_impl->curr_operation_axis = Sketch_delta::Curr_linear_edge_record{pt_a, pt_b};
}

bool Sketch_op_recorder::empty_() const
{
  if (!m_delta)
    return true;

  const Sketch_delta::Impl& d = *m_delta->m_impl;
  return d.prev_linear_edges.empty() && d.curr_linear_edges.empty() && d.prev_arc_edges.empty() &&
         d.curr_arc_edges.empty() && d.curr_node_idxs.empty() && d.prev_length_dims.empty() &&
         d.curr_length_dims.empty() && !d.prev_operation_axis.has_value() && !d.curr_operation_axis.has_value();
}

void Sketch_op_recorder::commit()
{
  if (!m_active || m_committed)
    return;

  m_committed = true;
  m_active    = false;

  if (m_sketch.m_undo_recorder == this)
    m_sketch.m_undo_recorder = nullptr;

  if (!empty_())
    m_view.push_undo_delta(std::move(m_delta));
}

void Sketch_op_recorder::cancel()
{
  m_active    = false;
  m_committed = true;

  if (m_sketch.m_undo_recorder == this)
    m_sketch.m_undo_recorder = nullptr;
}

Sketch_delta::Impl::Impl(Sketch& sketch, std::string sketch_name)
    : m_sketch(&sketch)
    , m_sketch_name(std::move(sketch_name))
{
}

Sketch* Sketch_delta::Impl::resolve_sketch_(Occt_view& view) const
{
  if (m_sketch)
    return m_sketch;

  for (const Sketch::sptr& s : view.get_sketches())
    if (s->get_name() == m_sketch_name)
      return s.get();

  return nullptr;
}

void Sketch_delta::Impl::apply_forward_(Sketch& sketch) const
{
  for (const Sketch_delta::Curr_linear_edge_record& e : curr_linear_edges)
    sketch.add_edge_(e.pt_a, e.pt_b);

  for (const Sketch_delta::Arc_edge_record& e : curr_arc_edges)
    sketch.add_arc_circle_(e.pt_a, e.pt_b, e.pt_c);

  for (const Sketch_delta::Length_dim_record& d : curr_length_dims)
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

  for (const Sketch_delta::Length_dim_record& d : curr_length_dims)
    remove_length_dim_(sketch, d);

  for (const Sketch_delta::Arc_edge_record& e : curr_arc_edges)
    remove_arc_edge_(sketch, e);

  for (const Sketch_delta::Curr_linear_edge_record& e : curr_linear_edges)
    remove_linear_edges_on_segment_(sketch, e.pt_a, e.pt_b);

  for (const Sketch_delta::Prev_edge_rec& e : prev_linear_edges)
    restore_prev_linear_edge_(sketch, e);

  for (const Sketch_delta::Arc_edge_record& e : prev_arc_edges)
    sketch.add_arc_circle_(e.pt_a, e.pt_b, e.pt_c);

  if (prev_operation_axis.has_value())
    restore_prev_operation_axis_(sketch, *prev_operation_axis);

  for (const Sketch_delta::Length_dim_record& d : prev_length_dims)
    restore_length_dim_(sketch, d);

  for (size_t node_idx : curr_node_idxs)
    tombstone_node_(sketch, node_idx);

  sketch.m_nodes.hide_snap_annos();
  sketch.update_faces_();
}

std::unique_ptr<Sketch_delta> Sketch_delta::Impl::clone() const
{
  auto copy     = std::make_unique<Sketch_delta>(*m_sketch, m_sketch_name);
  Impl& copy_impl = *copy->m_impl;
  copy_impl.m_sketch            = m_sketch;
  copy_impl.prev_linear_edges   = prev_linear_edges;
  copy_impl.curr_linear_edges   = curr_linear_edges;
  copy_impl.prev_arc_edges      = prev_arc_edges;
  copy_impl.curr_arc_edges      = curr_arc_edges;
  copy_impl.curr_node_idxs      = curr_node_idxs;
  copy_impl.prev_length_dims    = prev_length_dims;
  copy_impl.curr_length_dims    = curr_length_dims;
  copy_impl.prev_operation_axis = prev_operation_axis;
  copy_impl.curr_operation_axis = curr_operation_axis;
  return copy;
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

bool prev_linear_equal_(const Sketch_delta::Prev_edge_rec& x, const Sketch_delta::Prev_edge_rec& y)
{
  return x.node_idx_a == y.node_idx_a && x.node_idx_b == y.node_idx_b && x.node_idx_mid == y.node_idx_mid;
}

bool curr_linear_equal_(const Sketch_delta::Curr_linear_edge_record& x, const Sketch_delta::Curr_linear_edge_record& y)
{
  return x.pt_a.SquareDistance(y.pt_a) <= Precision::SquareConfusion() &&
         x.pt_b.SquareDistance(y.pt_b) <= Precision::SquareConfusion();
}

bool arc_equal_(const Sketch_delta::Arc_edge_record& x, const Sketch_delta::Arc_edge_record& y)
{
  return x.pt_a.SquareDistance(y.pt_a) <= Precision::SquareConfusion() &&
         x.pt_b.SquareDistance(y.pt_b) <= Precision::SquareConfusion() &&
         x.pt_c.SquareDistance(y.pt_c) <= Precision::SquareConfusion();
}

bool length_dim_equal_(const Sketch_delta::Length_dim_record& x, const Sketch_delta::Length_dim_record& y)
{
  return x.node_idx_lo == y.node_idx_lo && x.node_idx_hi == y.node_idx_hi;
}

void remove_linear_edges_on_segment_(Sketch& sketch, const gp_Pnt2d& seg_a, const gp_Pnt2d& seg_b)
{
  for (auto itr = sketch.m_edges.begin(); itr != sketch.m_edges.end();)
  {
    if (itr->circle_arc || !itr->node_idx_b.has_value())
    {
      ++itr;
      continue;
    }

    const gp_Pnt2d& pa = sketch.m_nodes[itr->node_idx_a];
    const gp_Pnt2d& pb = sketch.m_nodes[*itr->node_idx_b];
    if (on_closed_segment_2d_(pa, seg_a, seg_b) && on_closed_segment_2d_(pb, seg_a, seg_b))
    {
      sketch.m_ctx.Remove(itr->shp, false);
      itr = sketch.m_edges.erase(itr);
    }
    else
      ++itr;
  }
}

void remove_linear_edges_on_node_segment_(Sketch& sketch, size_t node_a, size_t node_b)
{
  remove_linear_edges_on_segment_(sketch, sketch.m_nodes[node_a], sketch.m_nodes[node_b]);
}

void remove_arc_edge_(Sketch& sketch, const Sketch_delta::Arc_edge_record& rec)
{
  std::vector<Sketch_AIS_edge_ptr> shps_to_remove;

  for (auto itr = sketch.m_edges.begin(); itr != sketch.m_edges.end(); ++itr)
  {
    if (!itr->circle_arc || itr->shp.IsNull())
      continue;

    const Sketch::Edge* e1 = &*itr;
    const Sketch::Edge* e2 = nullptr;

    for (auto itr2 = sketch.m_edges.begin(); itr2 != sketch.m_edges.end(); ++itr2)
      if (itr2 != itr && itr2->shp == e1->shp)
      {
        e2 = &*itr2;
        break;
      }

    if (!e2)
      continue;

    const Sketch::Edge* first  = e1->node_idx_arc.has_value() ? e1 : (e2->node_idx_arc.has_value() ? e2 : nullptr);
    const Sketch::Edge* second = first == e1 ? e2 : e1;
    if (!first || !first->node_idx_arc.has_value() || !second->node_idx_b.has_value())
      continue;

    const gp_Pnt2d start = sketch.m_nodes[first->node_idx_a];
    const gp_Pnt2d arc   = sketch.m_nodes[*first->node_idx_arc];
    const gp_Pnt2d end   = sketch.m_nodes[*second->node_idx_b];

    if (!arc_equal_({start, arc, end}, rec))
      continue;

    const Sketch_AIS_edge_ptr& shp = e1->shp;
    if (std::find(shps_to_remove.begin(), shps_to_remove.end(), shp) == shps_to_remove.end())
      shps_to_remove.push_back(shp);
  }

  for (const Sketch_AIS_edge_ptr& shp : shps_to_remove)
  {
    sketch.m_ctx.Remove(shp, false);
    for (auto itr = sketch.m_edges.begin(); itr != sketch.m_edges.end();)
      if (itr->shp == shp)
        itr = sketch.m_edges.erase(itr);
      else
        ++itr;
  }
}

void remove_length_dim_(Sketch& sketch, const Sketch_delta::Length_dim_record& rec)
{
  for (auto it = sketch.m_length_dimensions.begin(); it != sketch.m_length_dimensions.end(); ++it)
    if (it->node_idx_lo == rec.node_idx_lo && it->node_idx_hi == rec.node_idx_hi)
    {
      if (!it->dim.IsNull())
        sketch.m_ctx.Remove(it->dim, true);

      sketch.m_length_dimensions.erase(it);
      return;
    }
}

void tombstone_node_(Sketch& sketch, size_t node_idx)
{
  EZY_ASSERT(node_idx < sketch.m_nodes.size());
  sketch.m_nodes[node_idx].deleted = true;
  sketch.remove_length_dimensions_referencing_node_(node_idx);
  if (node_idx < sketch.m_permanent_node_marks.size() && sketch.m_permanent_node_marks[node_idx])
  {
    sketch.m_ctx.Remove(sketch.m_permanent_node_marks[node_idx], false);
    sketch.m_permanent_node_marks[node_idx].Nullify();
  }
}

void restore_prev_linear_edge_(Sketch& sketch, const Sketch_delta::Prev_edge_rec& rec)
{
  remove_linear_edges_on_node_segment_(sketch, rec.node_idx_a, rec.node_idx_b);
  sketch.sketch_json_add_linear_edge_(rec.node_idx_a, rec.node_idx_b, rec.node_idx_mid);
  if (!rec.name.empty())
    for (Sketch::Edge& e : sketch.m_edges)
      if (!e.circle_arc && e.node_idx_a == rec.node_idx_a && e.node_idx_b.has_value() && *e.node_idx_b == rec.node_idx_b &&
          e.node_idx_mid == rec.node_idx_mid)
      {
        e.name = rec.name;
        break;
      }
}

void restore_length_dim_(Sketch& sketch, const Sketch_delta::Length_dim_record& rec)
{
  sketch.json_add_length_dimension_(rec.node_idx_lo, rec.node_idx_hi, rec.visible, rec.flyout_offset, rec.name);
}

void restore_prev_operation_axis_(Sketch& sketch, const Sketch_delta::Prev_edge_rec& rec)
{
  const gp_Pnt2d pt_a = sketch.m_nodes[rec.node_idx_a];
  const gp_Pnt2d pt_b = sketch.m_nodes[rec.node_idx_b];
  sketch.sketch_json_set_operation_axis_(pt_a, pt_b);
}

bool is_linear_sketch_edge_(const Sketch::Edge& e)
{
  return !e.circle_arc && e.node_idx_b.has_value() && !e.node_idx_arc.has_value();
}

void capture_linear_edges_at_start_(Sketch& sketch, std::vector<Sketch_delta::Prev_edge_rec>& out)
{
  for (const Sketch::Edge& e : sketch.m_edges)
  {
    if (!is_linear_sketch_edge_(e))
      continue;

    Sketch_delta::Prev_edge_rec rec{e.node_idx_a, *e.node_idx_b, e.node_idx_mid, e.name};
    if (std::find_if(out.begin(), out.end(),
                     [&](const Sketch_delta::Prev_edge_rec& x) { return prev_linear_equal_(x, rec); }) == out.end())
      out.push_back(std::move(rec));
  }
}

bool linear_edge_at_op_start_(const std::vector<Sketch_delta::Prev_edge_rec>& at_start, const Sketch_delta::Prev_edge_rec& rec)
{
  return std::find_if(at_start.begin(), at_start.end(),
                      [&](const Sketch_delta::Prev_edge_rec& x) { return prev_linear_equal_(x, rec); }) != at_start.end();
}

} // namespace
