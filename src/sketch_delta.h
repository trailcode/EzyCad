#pragma once

#include "delta.h"

#include <gp_Pnt2d.hxx>

#include <cstddef>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

class Occt_view;
class Sketch;

/// One sketch undo step: `prev_*` is restored on undo; `curr_*` is re-applied on redo.
/// Node indices are never compacted; undo tombstones `curr_node_idxs` and redo revives them
/// through the normal node lookup when edges are added again.
class Sketch_delta : public Delta
{
public:
  struct Prev_edge_rec
  {
    size_t                node_idx_a{};
    size_t                node_idx_b{};
    std::optional<size_t> node_idx_mid;
    std::string           name;
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
    size_t                node_idx_lo{};
    size_t                node_idx_hi{};
    bool                  visible{true};
    std::optional<double> flyout_offset;
    std::string           name;
  };

  Sketch_delta(Sketch& sketch, std::string sketch_name);

  void                   apply_forward(Occt_view& view) override;
  void                   apply_reverse(Occt_view& view) override;
  std::unique_ptr<Delta> clone() const override;

  std::vector<Prev_edge_rec>             prev_linear_edges;
  std::vector<Curr_linear_edge_record>   curr_linear_edges;
  std::vector<Arc_edge_record>           prev_arc_edges;
  std::vector<Arc_edge_record>           curr_arc_edges;
  std::vector<size_t>                    curr_node_idxs;
  std::vector<Length_dim_record>         prev_length_dims;
  std::vector<Length_dim_record>         curr_length_dims;
  std::optional<Prev_edge_rec>           prev_operation_axis;
  std::optional<Curr_linear_edge_record> curr_operation_axis;

private:
  Sketch* resolve_sketch_(Occt_view& view) const;
  void    apply_forward_(Sketch& sketch) const;
  void    apply_reverse_(Sketch& sketch) const;

  Sketch*     m_sketch{nullptr};
  std::string m_sketch_name;
};

class Sketch_op_recorder;

/// Records `prev`/`curr` lists during one sketch edit; pushes a `Sketch_delta` on commit.
class Sketch_op_recorder
{
public:
  Sketch_op_recorder(Occt_view& view, Sketch& sketch);
  ~Sketch_op_recorder();

  Sketch_op_recorder(const Sketch_op_recorder&)            = delete;
  Sketch_op_recorder& operator=(const Sketch_op_recorder&) = delete;

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
  friend class Sketch;

  bool empty_() const;

  Occt_view&                               m_view;
  Sketch&                                  m_sketch;
  bool                                     m_active{true};
  bool                                     m_committed{false};
  std::set<size_t>                         m_live_nodes_at_start;
  std::vector<Sketch_delta::Prev_edge_rec> m_linear_edges_at_start;
  std::unique_ptr<Sketch_delta>            m_delta;
};
