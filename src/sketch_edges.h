#pragma once

#include <functional>
#include <gp_Pnt2d.hxx>
#include <list>
#include <optional>
#include <vector>

#include "sketch_edge.h"
#include "utl_types.h"

class Sketch;
class Sketch_op_recorder;

/// Persistent sketch edge list and edge CRUD (add, pick, remove, JSON load).
class Sketch_edges
{
public:
  explicit Sketch_edges(Sketch& sketch);

  void add_edge(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b);
  void add_edge(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, Sketch_op_recorder& rec);
  void sketch_json_add_linear_edge(size_t idx_a, size_t idx_b, std::optional<size_t> idx_mid);

  void update_end_pt(Sketch_edge& edge, size_t end_pt_idx);
  void add_arc_circle_edges(const std::vector<size_t>& node_idxs);

  void remove_by_ais(const Sketch_AIS_edge& to_remove);
  void remove_displayed();

  [[nodiscard]] std::list<Sketch_edge>::iterator get_at(const ScreenCoords& screen_coords);
  [[nodiscard]] std::vector<Sketch_edge>         get_selected() const;

  [[nodiscard]] bool   empty() const { return m_edges.empty(); }
  [[nodiscard]] size_t size() const { return m_edges.size(); }

  [[nodiscard]] const std::list<Sketch_edge>& edges() const { return m_edges; }
  [[nodiscard]] std::list<Sketch_edge>&       edges() { return m_edges; }

  static bool is_linear(const Sketch_edge& e) { return sketch_edge_is_linear(e); }

  using Linear_visitor = std::function<void(const Sketch_edge_linear&)>;
  using Arc_visitor    = std::function<void(const Sketch_edge_arc&)>;

  void for_each_linear(const Linear_visitor& fn) const;
  void for_each_arc(const Arc_visitor& fn) const;

private:
  friend class Sketch;
  friend class Sketch_topo;
  friend class Sketch_delta;

  void add_edge_raw_(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b);
  void add_edge_impl_(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, Sketch_op_recorder* rec);
  void split_arc_at_node_(std::list<Sketch_edge>::iterator itr, size_t split_idx, Sketch_op_recorder* rec);

  Sketch&                m_sketch;
  std::list<Sketch_edge> m_edges;
};
