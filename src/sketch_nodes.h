#pragma once

#include <gp_Pnt2d.hxx>
#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "types.h"

class gp_Pln;
class Occt_view;
class Sketch_json;

class Sketch_nodes
{
public:
  enum class Snap_guide_mode : int
  {
    Traditional = 0,
    Fullscreen  = 1,
    Both        = 2
  };

  struct Node : public gp_Pnt2d
  {
    bool        midpoint  = false;
    bool        deleted   = false;
    bool        permanent = false;
    std::string name;
  };

  // `view` must exist for the lifetime of this object
  Sketch_nodes(Occt_view& view, const gp_Pln& pln);
  ~Sketch_nodes();

  Sketch_nodes(const Sketch_nodes&)            = delete;
  Sketch_nodes& operator=(const Sketch_nodes&) = delete;
  Sketch_nodes(Sketch_nodes&&)                 = delete;
  Sketch_nodes& operator=(Sketch_nodes&&)      = delete;

  size_t                  add_new_node(const gp_Pnt2d& pt, bool is_edge_mid_point = false, bool is_permanent = false);
  std::optional<gp_Pnt2d> snap(const ScreenCoords& screen_coords);
  // If no node exists at `pt`, appends one; `permanent_for_new` sets Node::permanent on that new node only.
  size_t                get_node_exact(const gp_Pnt2d& pt, bool permanent_for_new = false);
  std::optional<size_t> get_node(const ScreenCoords& screen_coords);
  /// Projects the click onto the sketch plane and returns a node index only if within snap range of an
  /// existing non-deleted node. Never creates nodes (unlike `get_node`).
  std::optional<size_t> try_pick_existing_node(const ScreenCoords& screen_coords);
  std::optional<size_t>
  try_get_node_idx_snap(gp_Pnt2d& pt, // `pt` could be snapped to a node, an axis of another node, or an outside snap point.
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

  void clear_outside_snap_pnts();
  void add_outside_snap_pnt(const gp_Pnt& pt3d);

  // Snap distance related
  static void            set_snap_dist(double snap_dist_pixels);
  static double          get_snap_dist();
  static void            set_snap_guide_mode(Snap_guide_mode mode);
  static Snap_guide_mode get_snap_guide_mode();
  static void            set_snap_guide_color(float r, float g, float b);
  static void            get_snap_guide_color(float& r, float& g, float& b);
  static void            set_snap_guide_line_width(float width);
  static float           get_snap_guide_line_width();

  // When true, axis snap annotations (small markers) are shown for *all* nodes sharing the
  // same X or Y (within snap tolerance) in the current sketch (and outside points from other
  // visible sketches). When false (default), only the closest node per axis is annotated.
  static void set_annotate_all_coaxial_nodes(bool enable);
  static bool get_annotate_all_coaxial_nodes();

  // Methods for range-based for loop support
  std::vector<Node>::iterator       begin();
  std::vector<Node>::iterator       end();
  std::vector<Node>::const_iterator begin() const;
  std::vector<Node>::const_iterator end() const;
  std::vector<Node>::const_iterator cbegin() const;
  std::vector<Node>::const_iterator cend() const;

private:
  friend class Sketch_json;

  /// Resize storage so indices `0..count-1` exist (JSON load).
  void json_resize(size_t count);
  /// Assign slot `idx` (used after `json_resize`).
  void json_set_node(size_t idx, const gp_Pnt2d& pt, bool deleted, bool midpoint, bool permanent, const std::string& name = {});

  class Impl;
  std::unique_ptr<Impl> m_impl;
};
