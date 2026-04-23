#pragma once

#include <AIS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Vec2d.hxx>
#include <list>
#include <memory>
#include <optional>
#include <set>
#include <vector>

#include "shp.h"
#include "sketch_nodes.h"
#include "types.h"
#include "utl.h"

class Occt_view;
class gp_Pln;
class TopoDS_Wire;
class Sketch;
class Sketch_underlay;
enum class Mode;

struct Sketch_AIS_edge : public AIS_Shape
{
  Sketch_AIS_edge(Sketch& owner, const TopoDS_Shape& shp);
  Sketch& owner_sketch;
};

/// Selectable "+" marker for user-placed permanent sketch nodes (add-node tool).
struct Sketch_AIS_node_mark : public AIS_Shape
{
  Sketch_AIS_node_mark(Sketch& owner, size_t node_idx, const TopoDS_Shape& shp);
  Sketch& owner_sketch;
  size_t  node_idx {};
};

struct Sketch_face_shp : public AIS_Shape
{
  Sketch_face_shp(Sketch& owner, const TopoDS_Shape& face);

  Sketch&             owner_sketch;
  std::vector<gp_Pnt> verts_3d;  // Used for extruding the face
  std::vector<size_t> vert_idxs;
};

using Sketch_AIS_edge_ptr      = opencascade::handle<Sketch_AIS_edge>;
using Sketch_AIS_node_mark_ptr = opencascade::handle<Sketch_AIS_node_mark>;
using Sketch_face_shp_ptr      = opencascade::handle<Sketch_face_shp>;

// The Sketch class provides a comprehensive set of methods for creating and manipulating 2D sketches in a 3D environment.
// It supports adding and moving points, creating line segments, arcs, and mirror lines, and updating the sketch's
// visibility and style. The class also includes helper methods for snapping points, updating edge shapes,
// and extracting faces from the sketch.
class Sketch
{
 public:
  using Sketch_ptr = std::shared_ptr<Sketch>;

  // `view` must exist for the lifetime of this `Sketch`
  Sketch(const std::string& name, Occt_view& view, const gp_Pln& pln);
  Sketch(const std::string& name, Occt_view& view, const gp_Pln& pln, const TopoDS_Wire& outer_wire);

  ~Sketch();

  enum Edge_style
  {
    Full,
    Background,
    Hidden
  };

  // Add sketch related
  void add_sketch_pt(const ScreenCoords& screen_coords);
  void sketch_pt_move(const ScreenCoords& screen_coords);
  void dimension_input(const ScreenCoords& screen_coords);
  void angle_input(const ScreenCoords& screen_coords);
  void finalize_elm();
  bool cancel_elm();
  void clear_operation_axis();
  bool has_operation_axis() const;
  void on_enter();  // For finalizing manual distance input.

  // Visibility related
  void set_visible(bool state);
  bool is_visible() const;
  void set_show_faces(bool show);
  void set_show_edges(bool show);
  void set_show_dims(bool show);

  /// Apply global dimension line width to edge annotations and in-progress rubber-band dim.
  void refresh_edge_dimension_line_widths(double line_width);

  // Revolve related
  Shp_rslt revolve_selected(const double angle);

  bool is_current() const;
  void set_current();  // Make current in m_view
  void set_edge_style(Edge_style style);
  void remove_edge(const Sketch_AIS_edge& edge);
  void remove_permanent_node_mark(Sketch_AIS_node_mark& mark);
  void mirror_selected_edges();

  void toggle_edge_dim(const ScreenCoords& screen_coords);
  /// Remove a length dimension by OCCT object (used when deleting selection).
  bool try_remove_length_dimension(PrsDim_LengthDimension* dim);
  void dbg_append_str(std::string& out) const;

  // Error messages
  static constexpr const char* ERROR_NO_EDGES_SELECTED = "Error: No edges selected. Please select edges to mirror.";

  // Sketch name related.
  const std::string& get_name() const;
  void               set_name(const std::string& name);

  /// True if this sketch has at least one edge (used e.g. to pick mode after undo/redo).
  bool has_edges() const;

  void          on_mode();
  Mode          get_mode() const;
  const gp_Pln& get_plane() const;
  Sketch_nodes& get_nodes();

  [[nodiscard]] bool  has_underlay() const;
  [[nodiscard]] int   underlay_image_w() const;
  [[nodiscard]] int   underlay_image_h() const;
  [[nodiscard]] bool  load_underlay_image(const std::string& file_bytes);
  void                clear_underlay();
  void                underlay_set_center_extents_rotation(double cx, double cy, double half_w, double half_h, double rot_deg);
  void                underlay_set_opacity(float opaque01);
  void                underlay_set_visible(bool v);
  [[nodiscard]] float underlay_opacity() const;
  [[nodiscard]] bool  underlay_visible() const;
  void                underlay_set_key_white_transparent(bool on);
  [[nodiscard]] bool  underlay_key_white_transparent() const;
  void                underlay_set_line_tint_enabled(bool on);
  void                underlay_set_line_tint_rgb(uint8_t r, uint8_t g, uint8_t b);
  [[nodiscard]] bool  underlay_line_tint_enabled() const;
  void                underlay_line_tint_rgb(uint8_t& r, uint8_t& g, uint8_t& b) const;
  void                underlay_ui_params(double& cx, double& cy, double& half_w, double& half_h, double& rot_deg) const;
  void                underlay_rebuild_display();

  /// Same snap / plane rules as the line-edge tool (for underlay calibration clicks).
  [[nodiscard]] std::optional<gp_Pnt2d> pick_point_for_underlay_calib(const ScreenCoords& screen_coords);
  /// Set underlay from texture corner \a base, U edge vector \a axis_u, V edge vector \a axis_v (plane 2D).
  void                                  underlay_set_affine_plane(const gp_Pnt2d& base, const gp_Vec2d& axis_u, const gp_Vec2d& axis_v);
  /// Uniformly scale texture axes so plane segment \a p0-\a p1 has length \a target_len; UV at \a p0 stays fixed.
  [[nodiscard]] bool                    underlay_rescale_uv_chord_to_length(const gp_Pnt2d& p0, const gp_Pnt2d& p1, double target_len);
  /// Keep U axis; adjust V and base so segment \a y0-\a y1 has length \a target_len (after X calibration).
  [[nodiscard]] bool                    underlay_rescale_v_chord_to_length(const gp_Pnt2d& y0, const gp_Pnt2d& y1, double target_len);
  [[nodiscard]] gp_Vec2d                underlay_axis_u_vec() const;

  // private:
  friend class Sketch_json;
  friend class Sketch_access;
  friend class Sketch_test;  // TEST_F(Sketch_test, JsonSerializationDeserialization)

  /// Linear distance between two sketch nodes (independent of which edge connects them).
  struct Length_dimension
  {
    size_t                     node_idx_lo {};
    size_t                     node_idx_hi {};
    PrsDim_LengthDimension_ptr dim;
  };

  struct Edge
  {
    size_t                node_idx_a;
    std::optional<size_t> node_idx_b;
    std::optional<size_t> node_idx_arc;  // Only valid for circle arc edges.
    std::optional<size_t> node_idx_mid;  // Midpoint of edge, used for snapping.

    //  Used to identify the two `Edges` defining a circle arc.
    Geom_TrimmedCurve_ptr circle_arc;
    Sketch_AIS_edge_ptr   shp;  // Current edge annotation.

    bool reversed(size_t idx_a, size_t idx_b) const;
  };

  struct Face_edge
  {
    const Edge& edge;
    bool        reversed;
    size_t      start_nd_idx() const;
    size_t      end_nd_idx() const;
  };

  // When user types in a edge length.
  struct Edge_len
  {
    gp_Dir2d dir;
    double   len;
  };

  using Face       = std::vector<size_t>;
  using Face_edges = std::vector<Face_edge>;

  // Line string / segment related
  enum class Linestring_type
  {
    Single,
    Two,
    Multiple
  };

  // Line string related
  void add_line_string_pt_(const ScreenCoords& screen_coords, Linestring_type linestring_type);
  void move_line_string_pt_(const ScreenCoords& screen_coords);
  void finalize_edges_();

  /// Add a single node (no new edge); splits any linear edge the node lies on in its interior.
  void   add_node_pt_(const ScreenCoords& screen_coords);
  void   move_add_node_pt_(const ScreenCoords& screen_coords);
  void   split_linear_edges_at_node_if_interior_(size_t node_idx);
  
  /// Move a newly placed node onto the nearest linear edge within pick tolerance so split + `used_nodes` sees it.
  void   snap_placed_node_to_closest_linear_edge_interior_(size_t node_idx);
  double plane_pick_snap_radius_world_() const;

  // Arc circle related
  void add_arc_circle_pt_(const ScreenCoords& screen_coords);
  void move_arc_circle_pt_(const ScreenCoords& screen_coords);
  void add_arc_circle_(const std::vector<size_t>& node_idxs);
  void add_arc_circle_(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, const gp_Pnt2d& pt_c);

  // Circle related
  void move_circle_pt_(const ScreenCoords& screen_coords);
  void finalize_circle_();

  // Square related
  void move_square_pt_(const ScreenCoords& screen_coords);
  void finalize_square_();

  // Rectangle related
  void move_rectangle_pt_(const ScreenCoords& screen_coords);
  void finalize_rectangle_();

  // Slot related
  void move_slot_pt_(const ScreenCoords& screen_coords);
  void finalize_slot_();

  // Operation axis related
  void add_operation_axis_pt_(const ScreenCoords& screen_coords);
  void finalize_operation_axis_();

  // General sketch point related
  template <typename Callback>
  void add_sketch_pt_(const ScreenCoords& screen_coords, size_t required_num_pts, Callback&& callback);
  template <typename Callback>
  void move_sketch_pt_(const ScreenCoords& screen_coords, Callback&& callback);
  /// Invokes callback(e, pt_a, pt_b) with the last tmp edge only when it exists and is non-degenerate.
  template <typename Callback>
  void if_edge_pt_valid_(Callback&& callback);
  void check_dimension_seg_(Linestring_type linestring_type);
  /// Typed distance (Enter) while add-node rubber band is active - places node B, no edge.
  /// Enter key / dist popup commit for rubber-band tmp edge (add node, square, circle, rectangle, ...).
  void check_dimension_rubber_();
  /// Right-click / finalize: drop incomplete add-node preview (same idea as incomplete line).
  void finalize_add_node_elm_cleanup_();
  void add_edge_(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b);
  /// JSON load: linear edge using existing node indices (`idx_mid` is the edge midpoint node).
  void sketch_json_add_linear_edge_(size_t idx_a, size_t idx_b, size_t idx_mid);

  // Selected related
  std::vector<Edge>                get_selected_edges_() const;
  std::vector<Sketch_face_shp_ptr> get_selected_faces_() const;

  void update_edge_end_pt_(Edge& e, size_t end_pt_idx);
  bool clear_tmps_();

  // Function to extract faces from the planar graph
  void                update_faces_();
  void                update_edge_shp_(Edge& edge, const gp_Pnt2d& pt_t, const gp_Pnt2d& pt_b);
  bool                is_face_clockwise_(const Face_edges& face) const;
  Sketch_face_shp_ptr create_face_shape_(const Face_edges& face);
  gp_Vec2d            edge_incoming_dir_(size_t idx_a, size_t idx_b, const Edge& edge) const;

  // 3D point related
  void   get_snap_pts_3d_(std::vector<gp_Pnt>& out);
  void   get_originating_face_snp_pts_3d_(std::vector<gp_Pnt>& out);
  gp_Pnt to_3d_(size_t node_idx) const;
  gp_Pnt to_3d_(const std::optional<size_t>& node_idx) const;

  // Style related
  void update_edge_style_(AIS_Shape_ptr& shp);
  void update_node_mark_style_(AIS_Shape_ptr& shp);
  void sync_permanent_node_annos_();
  void update_originating_face_style();
  void update_face_style_(AIS_Shape_ptr& shp) const;

  void rebuild_length_dimension_display_(Length_dimension& d);
  void purge_stale_length_dimensions_();
  void refresh_all_length_dimensions_();
  void remove_length_dimensions_referencing_node_(size_t node_idx);
  /// Add if missing, remove if present (same unordered node pair).
  void add_or_toggle_length_dim_between_node_indices_(size_t node_a, size_t node_b);
  void json_add_length_dimension_(size_t node_a, size_t node_b);

  /// Average of non-deleted node positions (3D on sketch plane); used to place edge dimensions outside loops.
  std::optional<gp_Pnt> approx_sketch_interior_ref_3d_() const;

  /// `m_faces` as `TopoDS_Face` for flyout tests (rebuilt in `update_faces_`).
  void rebuild_dim_classifier_face_cache_();

  // Query related
  std::list<Edge>::iterator get_edge_at_(const ScreenCoords& screen_coords);
  
  // Dimension input related
  void                      clear_len_dim_rubber_line_();
  void                      update_len_dim_rubber_line_(const ScreenCoords& screen_coords);
  void                      clear_len_dim_pick_state_();

  // Style related
  Edge_style m_edge_style {Edge_style::Full};

  // Arc circle related.
  std::optional<size_t> m_start_arc_circle_node_idx;

  // Outside sketch snapping related.
  std::vector<gp_Pnt> m_tmp_pts_3d;

  // Owner related
  Occt_view&              m_view;
  AIS_InteractiveContext& m_ctx;

  std::string m_dbg_str;
  std::string m_name;
  bool        m_visible {true};

  // Extrusion related.
  std::optional<gp_Pnt> m_to_extrude_pt;
  gp_Pln                m_curr_view_pln;

  // Mirror edges related;
  std::optional<Edge> m_operation_axis;

  // Dimensions related
  std::optional<Edge_len> m_entered_edge_len;
  bool                    m_show_dim_input {false};
  std::optional<double>   m_entered_edge_angle;  // Angle in degrees
  bool                    m_show_angle_input {false};

  // Geometry related
  AIS_Shape_ptr m_originating_face;  // If this sketch was created from a face.
  gp_Pln        m_pln;               // Plane this sketch is on.

  std::optional<gp_Pnt2d>               m_last_pt;
  Sketch_nodes                          m_nodes;
  /// One entry per node index; only indices with permanent, non-deleted nodes hold a displayed + marker.
  std::vector<Sketch_AIS_node_mark_ptr> m_permanent_node_marks;
  std::list<Edge>                       m_edges;
  std::vector<Sketch_face_shp_ptr>      m_faces;
  std::vector<TopoDS_Face>              m_dim_classifier_faces;
  std::vector<Length_dimension>       m_length_dimensions;
  std::optional<size_t>                 m_len_dim_pick_anchor_node;
  /// Preview segment from anchor node to cursor while picking the second node (dim mode).
  AIS_Shape_ptr                         m_len_dim_rubber_shp;
  std::vector<size_t>                   m_tmp_node_idxs;
  std::vector<Edge>                     m_tmp_edges;
  AIS_Shape_ptr                         m_tmp_shp;
  PrsDim_LengthDimension_ptr            m_tmp_dim_anno;
  bool                                  m_show_faces {true};

  std::unique_ptr<Sketch_underlay> m_underlay;
};
