#pragma once

#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Vec2d.hxx>
#include <list>
#include <optional>
#include <string>
#include <vector>

#include "shp.h"
#include "sketch_dims.h"
#include "sketch_edge.h"
#include "sketch_edges.h"
#include "sketch_node_marks.h"
#include "sketch_nodes.h"
#include "sketch_topo.h"
#include "sketch_tools.h"
#include "sketch_underlay.h"
#include "utl_types.h"

struct Length_dimension_style;
class Occt_view;
class gp_Pln;
class Sketch_op_recorder;
class TopoDS_Wire;
class Sketch;
enum class Mode;

/// Selective refresh of sketch annotations after global settings or geometry changes.
struct Sketch_annotation_refresh
{
  bool length_dimensions    = false;
  bool permanent_node_marks = false;
};

// The Sketch class provides a comprehensive set of methods for creating and manipulating 2D sketches in a 3D environment.
// It supports adding and moving points, creating line segments, arcs, and mirror lines, and updating the sketch's
// visibility and style. The class also includes helper methods for snapping points, updating edge shapes,
// and extracting faces from the sketch.
class Sketch
{
public:
  DECL_PTR(Sketch);
  using Sketch_ptr = sptr; // Compatibility alias for existing code.

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

  using Edge            = Sketch_edge;
  using Linestring_type = Sketch_tools::Linestring_type;

  // Add sketch related
  void add_sketch_pt(const ScreenCoords& screen_coords);
  void sketch_pt_move(const ScreenCoords& screen_coords);
  void dimension_input(const ScreenCoords& screen_coords);
  void angle_input(const ScreenCoords& screen_coords);
  void finalize_elm();
  bool cancel_elm();
  void clear_operation_axis();
  bool has_operation_axis() const;
  void on_enter(); // For finalizing manual distance input.

  // Visibility related
  void        set_visible(bool state);
  bool        is_visible() const;
  void        set_show_faces(bool show);
  void        set_show_edges(bool show);
  void        set_show_dims(bool show);
  bool        shows_dimensions() const;
  bool        dimension_visible(size_t dim_index) const;
  void        set_dimension_visible(size_t dim_index, bool visible);
  size_t      dimension_node_lo(size_t dim_index) const;
  size_t      dimension_node_hi(size_t dim_index) const;
  double      dimension_offset(size_t dim_index) const;
  void        set_dimension_offset(size_t dim_index, double offset);
  std::string dimension_name(size_t dim_index) const;
  void        set_dimension_name(size_t dim_index, const std::string& name);
  /// OCCT length-dimension object for list hover / viewer highlight (may be null while rebuilding).
  PrsDim_LengthDimension_ptr length_dimension_handle(size_t dim_index) const;

  /// AIS objects to emphasize while this sketch's Sketch List row is hovered.
  void append_list_hover_ais(std::vector<AIS_InteractiveObject_ptr>& out) const;

  /// Rebuild length dimensions and/or permanent node '+' markers (e.g. after settings changes).
  void refresh_annotations(const Sketch_annotation_refresh& refresh);

  // Revolve related
  Shp_rslt revolve_selected(const double angle);

  bool is_current() const;
  void set_current(); // Make current in m_view
  void set_edge_style(Edge_style style);
  void remove_edge(const Sketch_AIS_edge& edge);
  void remove_permanent_node_mark(Sketch_AIS_node_mark& mark);
  void mirror_selected_edges();

  void toggle_edge_dim_anno(const ScreenCoords& screen_coords);
  /// Remove a length dimension by OCCT object (used when deleting selection).
  bool try_remove_length_dimension(PrsDim_LengthDimension* dim);
  void dbg_append_str(std::string& out) const;

  // Error messages
  static constexpr const char* ERROR_NO_EDGES_SELECTED = "Error: No edges selected. Please select edges to mirror.";

  // Sketch name related.
  const std::string& get_name() const;
  void               set_name(const std::string& name);

  /// Stable identity for undo deltas and file I/O (display names may duplicate).
  size_t get_id() const;

  /// True if this sketch has at least one edge (used e.g. to pick mode after undo/redo).
  bool   has_edges() const;
  size_t edge_count() const;
  size_t face_count() const;
  size_t length_dimension_count() const;

  std::vector<std::string> inspector_edge_labels() const;
  std::vector<std::string> inspector_face_labels() const;
  std::vector<std::string> inspector_dimension_labels() const;
  std::vector<std::string> inspector_node_labels() const;

  void          on_mode();
  Mode          get_mode() const;
  const gp_Pln& get_plane() const;
  Sketch_nodes& get_nodes();

  Sketch_underlay&                     underlay() { return m_underlay; }
  [[nodiscard]] const Sketch_underlay& underlay() const { return m_underlay; }

  static void set_add_mid_pt_edges(bool on);
  static bool get_add_mid_pt_edges();
  static void set_edge_from_center(bool on);
  static bool get_edge_from_center();

  /// Add a linear edge between plane points (scripting / import).
  void add_linear_edge(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b);
  /// Rebuild closed-face topology after bulk edge import.
  void rebuild_faces();

private:
  friend class Sketch_json;
  friend class Sketch_access;
  friend class Sketch_delta;
  friend class Sketch_op_recorder;
  friend class Sketch_topo;
  friend class Sketch_dims;
  friend class Sketch_edges;
  friend class Sketch_node_marks;
  friend class Sketch_tools;

  static bool s_add_mid_pt_edges;
  static bool s_edge_from_center;

  void add_edge_(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b);
  void add_edge_(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, Sketch_op_recorder& rec);
  void add_edge_raw_(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b);
  /// Helper used when walking edges for splitting / intersection logic.
  static bool is_linear_edge_(const Edge& e) { return Sketch_edges::is_linear(e); }
  /// JSON load: linear edge using existing node indices (`idx_mid` is the edge midpoint node).
  void sketch_json_add_linear_edge_(size_t idx_a, size_t idx_b, std::optional<size_t> idx_mid);
  /// JSON load: restore the sketch operation axis from two plane points.
  void sketch_json_set_operation_axis_(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b);

  void add_arc_circle_(const std::vector<size_t>& node_idxs);
  void add_arc_circle_(const std::vector<size_t>& node_idxs, Sketch_op_recorder& rec);
  void add_arc_circle_(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, const gp_Pnt2d& pt_c);
  void add_arc_circle_(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, const gp_Pnt2d& pt_c, Sketch_op_recorder& rec);

  // Selected related
  std::vector<Edge>                get_selected_edges_() const;
  std::vector<Sketch_face_shp_ptr> get_selected_faces_() const;

  void update_edge_end_pt_(Edge& e, size_t end_pt_idx);

  void     update_faces_();
  void     update_edge_shp_(Edge& edge, const gp_Pnt2d& pt_t, const gp_Pnt2d& pt_b);
  gp_Vec2d edge_incoming_dir_(size_t idx_a, size_t idx_b, const Edge& edge) const;

  // 3D point related
  void get_snap_pts_3d_(std::vector<gp_Pnt>& out);
  void get_originating_face_snp_pts_3d_(std::vector<gp_Pnt>& out);

  // Style related
  void update_edge_style_(AIS_Shape_ptr& shp);
  void sync_operation_axis_display_();
  bool show_operation_axis_() const;
  bool operation_axis_suppresses_sketch_snap_() const;
  void update_originating_face_style();
  void ensure_origin_node_();

  void json_add_length_dimension_(size_t node_a, size_t node_b, bool visible = true,
                                  std::optional<double> flyout_offset = std::nullopt, const std::string& name = {});
  void remove_length_dimensions_referencing_node_(size_t node_idx);

  // Query related
  std::list<Edge>::iterator get_edge_at_(const ScreenCoords& screen_coords);

  gp_Pnt to_3d_(size_t node_idx) const;
  gp_Pnt to_3d_(const std::optional<size_t>& node_idx) const;

  Edge_style m_edge_style{Edge_style::Full};

  // Outside sketch snapping related.
  std::vector<gp_Pnt> m_tmp_pts_3d;

  // Owner related
  Occt_view&              m_view;
  AIS_InteractiveContext& m_ctx;

  std::string m_dbg_str;
  std::string m_name;
  size_t      m_id{0};
  bool        m_visible{true};

  // Extrusion related.
  std::optional<gp_Pnt> m_to_extrude_pt;
  gp_Pln                m_curr_view_pln;

  // Mirror edges related;
  std::optional<Edge> m_operation_axis;

  // Geometry related
  AIS_Shape_ptr m_originating_face; // If this sketch was created from a face.
  gp_Pln        m_pln;              // Plane this sketch is on.

  Sketch_nodes      m_nodes;
  Sketch_node_marks m_node_marks;
  Sketch_edges      m_edges;
  Sketch_topo       m_topo;
  Sketch_dims       m_dims;
  Sketch_tools      m_tools;
  Sketch_underlay   m_underlay;
  bool              m_show_faces{true};
};
