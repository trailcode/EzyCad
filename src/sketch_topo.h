#pragma once

#include <TopoDS_Face.hxx>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "sketch_edge.h"
#include "utl_types.h"

class Sketch;
class Sketch_op_recorder;

/// Planar-graph topology for a sketch: face extraction, edge splitting, and face caches.
class Sketch_topo
{
public:
  explicit Sketch_topo(Sketch& sketch);

  void update_faces();
  void split_linear_edges_at_node_if_interior(size_t node_idx);
  void split_linear_edges_at_node_if_interior(size_t node_idx, Sketch_op_recorder& rec);
  void split_linear_edges_at_node_if_interior(size_t node_idx, Sketch_op_recorder* rec);
  void split_arcs_at_node_if_interior(size_t node_idx);
  void split_arcs_at_node_if_interior(size_t node_idx, Sketch_op_recorder& rec);
  void split_arcs_at_node_if_interior(size_t node_idx, Sketch_op_recorder* rec);

  [[nodiscard]] const std::vector<Sketch_face_shp_ptr>& faces() const { return m_faces; }
  [[nodiscard]] std::vector<Sketch_face_shp_ptr>&       faces() { return m_faces; }

  [[nodiscard]] const std::vector<TopoDS_Face>& dim_classifier_faces() const { return m_dim_classifier_faces; }

  void remove_displayed_faces();

  [[nodiscard]] double plane_pick_snap_radius_world() const;

private:
  struct Face_edge;
  using Face_edges = std::vector<Face_edge>;

  void                snap_placed_node_to_closest_linear_edge_interior_(size_t node_idx);
  [[nodiscard]] std::unordered_set<const Sketch_edge*> exclude_dangling_edges_(
      std::unordered_map<size_t, std::vector<std::pair<size_t, const Sketch_edge*>>>& adj_list);
  bool                is_face_ccw_(const Face_edges& face) const;
  Sketch_face_shp_ptr create_face_shape_(const Face_edges& face);
  void                rebuild_dim_classifier_face_cache_();

  Sketch&                          m_sketch;
  std::vector<Sketch_face_shp_ptr> m_faces;
  std::vector<TopoDS_Face>         m_dim_classifier_faces;
};
