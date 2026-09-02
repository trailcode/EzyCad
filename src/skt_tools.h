#pragma once

#include <gp_Pnt2d.hxx>
#include <gp_Vec2d.hxx>
#include <optional>
#include <utility>
#include <vector>

#include "skt_edge.h"
#include "utl_types.h"

class Sketch;
class Sketch_op_recorder;

/// Interactive sketch drawing session: tmp edges/nodes, previews, finalize/cancel.
class Sketch_tools
{
public:
  enum class Linestring_type
  {
    Single,
    Two,
    Multiple
  };

  explicit Sketch_tools(Sketch& sketch);

  void on_click(const ScreenCoords& screen_coords);
  void on_move(const ScreenCoords& screen_coords);
  void on_enter();
  void finalize();
  bool cancel();

  [[nodiscard]] bool edge_from_center_active() const;

  /// Session state used while drawing (e.g. typed dimension input).
  [[nodiscard]] std::vector<Sketch_edge>&       tmp_edges() { return m_tmp_edges; }
  [[nodiscard]] const std::vector<Sketch_edge>& tmp_edges() const { return m_tmp_edges; }
  [[nodiscard]] std::optional<gp_Pnt2d>&        last_pt() { return m_last_pt; }
  /// Clears rubber-band / tmp geometry. Returns true if tmp edges were present (used by `cancel()`).
  bool clear_tmps();

  void clear_tmp_node_idxs() { m_tmp_node_idxs.clear(); }

  [[nodiscard]] size_t tmp_node_count() const { return m_tmp_node_idxs.size(); }
  [[nodiscard]] size_t tmp_edge_count() const { return m_tmp_edges.size(); }

private:
  friend class Sketch_dims;

  bool complete_edge_from_center_(const ScreenCoords& screen_coords);

  void add_line_string_pt_(const ScreenCoords& screen_coords, Linestring_type linestring_type);
  void move_line_string_pt_(const ScreenCoords& screen_coords);
  void finalize_edges_(Sketch_op_recorder& rec);

  void add_node_pt_(const ScreenCoords& screen_coords);
  void move_add_node_pt_(const ScreenCoords& screen_coords);

  void add_arc_circle_pt_(const ScreenCoords& screen_coords);
  void move_arc_circle_pt_(const ScreenCoords& screen_coords);

  void move_square_pt_(const ScreenCoords& screen_coords);
  void finalize_square_(Sketch_op_recorder& rec);

  void move_circle_pt_(const ScreenCoords& screen_coords);
  void finalize_circle_(Sketch_op_recorder& rec);

  void move_rectangle_pt_(const ScreenCoords& screen_coords);
  void finalize_rectangle_(Sketch_op_recorder& rec);

  void move_slot_pt_(const ScreenCoords& screen_coords);
  void finalize_slot_(Sketch_op_recorder& rec);

  void add_bone_pt_(const ScreenCoords& screen_coords);
  void move_bone_pt_(const ScreenCoords& screen_coords);
  void finalize_bone_();
  void bone_begin_next_edge_from_(const gp_Pnt2d& origin);
  void bone_on_centers_ready_(const gp_Pnt2d& c1, const gp_Pnt2d& c2);
  [[nodiscard]] bool bone_try_commit_(double waist);
  void bone_update_preview_();
  [[nodiscard]] std::optional<gp_Vec2d> bone_axis_perp_() const;
  [[nodiscard]] std::optional<double>   bone_waist_from_pt_(const gp_Pnt2d& pt) const;

  void add_operation_axis_pt_(const ScreenCoords& screen_coords);
  void finalize_operation_axis_(Sketch_op_recorder& rec);

  template <typename Callback>
  void add_sketch_pt_(const ScreenCoords& screen_coords, size_t required_num_pts, Callback&& callback);
  template <typename Callback> void move_sketch_pt_(const ScreenCoords& screen_coords, Callback&& callback);
  template <typename Callback> void if_edge_pt_valid_(Callback&& callback);

  void finalize_add_node_elm_cleanup_();

  Sketch&                  m_sketch;
  std::optional<size_t>    m_start_arc_circle_node_idx;
  std::optional<gp_Pnt2d>  m_last_pt;
  std::vector<size_t>      m_tmp_node_idxs;
  std::vector<Sketch_edge> m_tmp_edges;
  AIS_Shape_ptr            m_tmp_shp;
  std::optional<std::pair<gp_Pnt2d, gp_Pnt2d>> m_bone_centers;
  std::optional<double>                        m_bone_r1;
  std::optional<double>                        m_bone_r2;
};
