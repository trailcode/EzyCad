#pragma once

#include "delta.h"

#include <gp_Pnt2d.hxx>

#include <memory>
#include <optional>
#include <string>

class Occt_view;
class Sketch;

/// Records `prev`/`curr` lists during one sketch edit; pushes a `Sketch_delta` on commit.
class Sketch_op_recorder
{
public:
  class Impl;

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
  std::unique_ptr<Impl> m_impl;
};

/// One sketch undo step: `prev_*` is restored on undo; `curr_*` is re-applied on redo.
/// Node indices are never compacted; undo tombstones `curr_node_idxs` and redo revives them
/// through the normal node lookup when edges are added again.
class Sketch_delta : public Delta
{
public:
  Sketch_delta(Sketch& sketch, std::string sketch_name);
  ~Sketch_delta() override;

  Sketch_delta(const Sketch_delta&)            = delete;
  Sketch_delta& operator=(const Sketch_delta&) = delete;

  void                   apply_forward(Occt_view& view) override;
  void                   apply_reverse(Occt_view& view) override;
  std::unique_ptr<Delta> clone() const override;

private:
  friend class Sketch_op_recorder::Impl;

  class Impl;
  std::unique_ptr<Impl> m_impl;
};
