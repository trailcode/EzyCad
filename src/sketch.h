#pragma once

#include <AIS_Shape.hxx>
#include <gp_Pnt2d.hxx>
#include <list>
#include <set>

#include "shp.h"
#include "sketch_nodes.h"
#include "types.h"
#include "utl.h"

class Occt_view;
class gp_Pln;
class TopoDS_Wire;
class Sketch;
enum class Mode;

struct Sketch_AIS_edge : public AIS_Shape
{
  Sketch_AIS_edge(Sketch& owner, const TopoDS_Shape& shp);
  Sketch& owner_sketch;
};

struct Sketch_face_shp : public AIS_Shape
{
  Sketch_face_shp(Sketch& owner, const TopoDS_Shape& face);

  Sketch&             owner_sketch;
  std::vector<gp_Pnt> verts_3d;  // Used for extruding the face
  std::vector<size_t> vert_idxs;
};

using Sketch_AIS_edge_ptr = opencascade::handle<Sketch_AIS_edge>;
using Sketch_face_shp_ptr = opencascade::handle<Sketch_face_shp>;

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

  // Revolve related
  RevolvedShp_rslt revolve_selected(const double angle);

  bool is_current() const;
  void set_current();  // Make current in m_view
  void set_edge_style(Edge_style style);
  void remove_edge(const Sketch_AIS_edge& edge);
  void mirror_selected_edges();

  void toggle_edge_dim(const ScreenCoords& screen_coords);
  void dbg_append_str(std::string& out) const;

  // Sketch name related.
  const std::string& get_name() const;
  void               set_name(const std::string& name);

  void          on_mode();
  Mode          get_mode() const;
  const gp_Pln& get_plane() const;
  Sketch_nodes& get_nodes();

 //private:
  friend class Sketch_json;
  friend class Sketch_access;
  friend class Sketch_test; // TEST_F(Sketch_test, JsonSerializationDeserialization)


  struct Edge
  {
    size_t                node_idx_a;
    std::optional<size_t> node_idx_b;
    std::optional<size_t> node_idx_arc;  // Only valid for circle arc edges.
    std::optional<size_t> node_idx_mid;  // Midpoint of edge, used for snapping.

    //  Used to identify the two `Edges` defining a circle arc.
    Geom_TrimmedCurve_ptr      circle_arc;
    Sketch_AIS_edge_ptr        shp;  // Current edge annotation.
    PrsDim_LengthDimension_ptr dim;  // Edge dimension annotation.

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
  template <typename Callback>
  void last_edge_(Callback&& callback);
  void check_dimension_seg_(Linestring_type linestring_type);
  void add_edge_(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, bool add_dim_anno = false);

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
  gp_Vec2d            edge_outgoing_dir_(size_t idx_a, size_t idx_b, const Edge& edge) const;

  // 3D point related
  void   get_snap_pts_3d_(std::vector<gp_Pnt>& out);
  void   get_originating_face_snp_pts_3d_(std::vector<gp_Pnt>& out);
  gp_Pnt to_3d_(size_t node_idx) const;
  gp_Pnt to_3d_(const std::optional<size_t>& node_idx) const;

  // Style related
  void update_edge_style_(AIS_Shape_ptr& shp);
  void update_originating_face_style();
  void update_face_style_(AIS_Shape_ptr& shp) const;
  void set_edge_dim_anno_visible_(Edge& e, bool visible);

  // Query related
  std::list<Edge>::iterator get_edge_at_(const ScreenCoords& screen_coords);

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

  std::optional<gp_Pnt2d>          m_last_pt;
  Sketch_nodes                     m_nodes;
  std::list<Edge>                  m_edges;
  std::vector<Sketch_face_shp_ptr> m_faces;
  std::vector<size_t>              m_tmp_node_idxs;
  std::vector<Edge>                m_tmp_edges;
  AIS_Shape_ptr                    m_tmp_shp;
  PrsDim_LengthDimension_ptr       m_tmp_dim_anno;
  bool                             m_show_faces {true};
};
