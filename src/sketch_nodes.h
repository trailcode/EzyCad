#pragma once

#include <gp_Pln.hxx>
#include <gp_Pnt2d.hxx>
#include <optional>
#include <set>

#include "types.h"

class gp_Pln;
class gp_Pnt2d;
class Occt_view;
class AIS_InteractiveContext;
class Sketch_json;

class Sketch_nodes
{
 public:
  struct Node : public gp_Pnt2d
  {
    bool midpoint = false;
    bool deleted     = false;
    bool permanent   = false;
  };

  // `view` must exist for the lifetime of this object
  Sketch_nodes(Occt_view& view, const gp_Pln& pln);
  ~Sketch_nodes();

  size_t                  add_new_node(const gp_Pnt2d& pt, bool is_edge_mid_point = false, bool is_permanent = false);
  std::optional<gp_Pnt2d> snap(const ScreenCoords& screen_coords);
  // If no node exists at `pt`, appends one; `permanent_for_new` sets Node::permanent on that new node only.
  size_t                  get_node_exact(const gp_Pnt2d& pt, bool permanent_for_new = false);
  std::optional<size_t>   get_node(const ScreenCoords& screen_coords);
  /// Projects the click onto the sketch plane and returns a node index only if within snap range of an
  /// existing non-deleted node. Never creates nodes (unlike `get_node`).
  std::optional<size_t>   try_pick_existing_node(const ScreenCoords& screen_coords);
  std::optional<size_t>   try_get_node_idx_snap(
        gp_Pnt2d&                  pt,  // `pt` could be snapped to a node, an axis of another node, or an outside snap point.
        const std::vector<size_t>& to_exclude = {});

  void get_snap_pts_3d(std::vector<gp_Pnt>& out);
  void hide_snap_annos();

  Node&       operator[](size_t idx);
  const Node& operator[](size_t idx) const;
  Node&       operator[](const std::optional<size_t> idx);
  const Node& operator[](const std::optional<size_t> idx) const;
  bool        empty() const;
  size_t      size() const;
  void        finalize();
  void        cancel();

  void        clear_outside_snap_pnts();
  void        add_outside_snap_pnt(const gp_Pnt& pt3d);

  // Snap distance related
  static void   set_snap_dist(double snap_dist_pixels);
  static double get_snap_dist();

  // Methods for range-based for loop support
  // clang-format off
  auto begin()        { return m_nodes.begin();  }
  auto end()          { return m_nodes.end();    }
  auto begin()  const { return m_nodes.begin();  }
  auto end()    const { return m_nodes.end();    }
  auto cbegin() const { return m_nodes.cbegin(); }
  auto cend()   const { return m_nodes.cend();   }
  // clang-format on

 private:
  friend class Sketch_json;

  /// Resize storage so indices `0..count-1` exist (JSON load).
  void json_resize(size_t count);
  /// Assign slot `idx` (used after `json_resize`).
  void json_set_node(size_t idx, const gp_Pnt2d& pt, bool deleted, bool midpoint, bool permanent);

  void update_node_snap_anno_(const gp_Pnt2d& pt, const double snap_dist);
  /// World-space snap radius at `pt` (same convention as `try_get_node_idx_snap` / `try_pick_existing_node`).
  double snap_radius_world_(const gp_Pnt2d& pt) const;
  bool try_snap_outside_(gp_Pnt2d& pt, const double snap_dist);  // Use points in `m_outside_snap_pts` `pt` will be modified if snapped.

  std::vector<Node>       m_nodes;
  static double           s_snap_dist_pixels;  // Global to all sketches
  std::set<gp_Pnt2d>      m_outside_snap_pts;  // Projected snap points from other sketches.
  AIS_Shape_ptr           m_snap_anno_axis[2];
  std::optional<gp_Pnt2d> m_last_snap_pt;  // Used for snap annotation
  AIS_Shape_ptr           m_snap_anno;
  size_t                  m_prev_num_nodes {0};  // Used when a operation is canceled.

  // Owner related
  Occt_view&              m_view;
  AIS_InteractiveContext& m_ctx;
  const gp_Pln            m_pln;
};