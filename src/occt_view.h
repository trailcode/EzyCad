#pragma once

#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <AIS_ViewController.hxx>
#include <Graphic3d_MaterialAspect.hxx>
#include <gp_Ax3.hxx>
#include <glm/glm.hpp>
#include <list>
#include <memory>
#include <optional>
#include <set>
#include <map>
#include <string>
#include <vector>

#include "occt_glfw_win.h"
#include "shp_chamfer.h"
#include "shp_common.h"
#include "shp_cut.h"
#include "shp_extrude.h"
#include "shp_fillet.h"
#include "shp_fuse.h"
#include "shp_move.h"
#include "shp_polar_dup.h"
#include "shp_rotate.h"
#include "shp_scale.h"
#include "utl_types.h"
#include "ezy_asset_store.h"

class Delta;
class GUI;
class Sketch;
struct Sketch_annotation_refresh;
struct Length_dimension_style;
class Prs3d_Drawer;
class TopoDS_Face;
class TopoDS_Wire;
class TopoDS_Edge;
class TopoDS_Shape;
enum class Mode;
enum class Command;

struct Ray
{
  gp_Pnt origin;
  gp_Dir direction;

  Ray(const gp_Pnt& orig, const gp_Dir& dir)
      : origin(orig)
      , direction(dir)
  {
  }
};

enum class Set_parent_mode
{
  Yes,
  No
};

/// Persisted OCCT viewer grid geometry (model units). The drawn grid is sized to the active sketch bounds plus
/// `grid_padding`; extent is not stored manually.
struct Occt_grid_rect_params
{
  double step{10.};
  double grid_padding{1000.};
  double graphic_z_offset{};
};

class Occt_view : protected AIS_ViewController
{
public:
  DECL_PTR(Occt_view);
  using Sketch_ptr  = std::shared_ptr<Sketch>;
  using Sketch_list = std::list<Sketch_ptr>;

  Occt_view(GUI& gui);
  ~Occt_view();

  // Initialization related.
  void init_window(GLFWwindow* GlfwWindow);
  void init_viewer();
  void init_default();

  std::string          to_json() const;
  void                 load(const std::string& json_str, bool restore_view = true);
  Ezy_asset_store&     asset_store() { return m_assets; }
  const Ezy_asset_store& asset_store() const { return m_assets; }
  [[nodiscard]] Status import_step(const std::string& step_data);
  bool                 import_ply(const std::string& ply_bytes);

  /// Writes STEP, IGES, binary STL, or PLY to \a file_path. Uses selected shapes if any, else all shapes.
  [[nodiscard]] Status export_document(Export_format fmt, const std::string& file_path);

  // Undo / redo (delta steps for sketch edits; full JSON snapshot for other operations).
  /// Saves current document (full JSON) and mode.
  void push_undo_snapshot();
  /// Records a sketch element delta (added/removed nodes and edges).
  void push_undo_delta(std::unique_ptr<Delta> delta);
  /// Removes the last snapshot without restoring (e.g. aborted edit that did not change the document).
  void   pop_undo_snapshot();
  bool   undo();
  bool   redo();
  bool   can_undo() const;
  bool   can_redo() const;
  size_t undo_stack_size() const;
  size_t redo_stack_size() const;

  void do_frame();
  /// Apply pending navigation (pan/zoom/rotate) to the camera before UI uses view projection (e.g. underlay slider bounds).
  void flush_view_events();

  // Mode related.
  void on_mode();
  /// Updates camera perspective/orthographic from current mode and the non-sketch orthographic toggle.
  void apply_camera_projection();
  void on_chamfer_mode();
  void on_fillet_mode();
  Mode get_mode() const;
  /// True while the operational axis is defined and mirror/revolve selection is active.
  bool sketch_snap_suppressed() const;

  void cleanup();

  // Delete related.
  void delete_selected();
  void delete_shapes(std::vector<AIS_Shape_ptr> to_delete);
  void delete_(std::vector<AIS_Shape_ptr>& to_delete);

  //  Member function to delete variable arguments
  template <typename... Args> void remove(Args&&... args);

  void cancel(Set_parent_mode set_parent_mode);

  // Sketch related
  Sketch_list& get_sketches();
  void         remove_sketch(const Sketch_ptr& sketch);
  /// Empty sketch on \a pln; \a base_name is uniquified (e.g. Sketch_xy, Sketch_xy.001).
  void       add_sketch(const gp_Pln& pln, const std::string& base_name);
  Sketch&    curr_sketch();
  Sketch_ptr curr_sketch_shared() const;
  void       set_curr_sketch(const Sketch_ptr& sketch);
  void       sketch_face_extrude(const ScreenCoords& screen_coords, bool is_mouse_move);

  std::list<Shp_ptr>& get_shapes();
  std::string         get_unique_shape_name(const char* base_name) const;
  void                add_box(double ox, double oy, double oz, double width, double length, double height);
  void                add_pyramid(double ox, double oy, double oz, double side);
  void                add_sphere(double ox, double oy, double oz, double radius);
  void                add_cylinder(double ox, double oy, double oz, double radius, double height);
  void                add_cone(double ox, double oy, double oz, double R1, double R2, double height);
  void                add_torus(double ox, double oy, double oz, double R1, double R2);

  // Shape related
  Shp_move&      shp_move();
  Shp_rotate&    shp_rotate();
  Shp_scale&     shp_scale();
  Shp_chamfer&   shp_chamfer();
  Shp_fillet&    shp_fillet();
  Shp_cut&       shp_cut();
  Shp_fuse&      shp_fuse();
  Shp_common&    shp_common();
  Shp_polar_dup& shp_polar_dup();
  Shp_extrude&   shp_extrude();

  // Revolve related
  void revolve_selected(const double angle);

  // View manipulation helpers (used by shape operations for UX; keep interface clean)
  void rotate_view(const gp_Vec& axis, const gp_Pnt& center);
  void redraw_view();

  /// Underlying OCCT view handle (for limited internal helpers such as closest_to_camera).
  V3d_View_ptr view_handle() const;

  void on_enter(const ScreenCoords& screen_coords); // For manual dimension distance keyboard input.
  bool fit_face_in_view(const TopoDS_Face& face);

  // Dimension related
  void                  dimension_input(const ScreenCoords& screen_coords);
  void                  angle_input(const ScreenCoords& screen_coords);
  void                  refresh_sketch_annotations(const Sketch_annotation_refresh& refresh);
  void                  apply_sketch_dimensions_visibility();
  double                get_dimension_scale() const;
  bool                  get_show_dim_input() const;
  void                  set_show_dim_input(bool show);
  std::optional<double> get_entered_dim() const;
  void                  set_entered_dim(const std::optional<double>& dim);

  // Input events.
  void on_resize(int theWidth, int theHeight);
  /// \param shift_finer_zoom If true, Blender-style x0.1 zoom step (held Shift).
  void on_mouse_scroll(double theOffsetX, double theOffsetY, bool shift_finer_zoom = false);
  void on_mouse_button(int theButton, int theAction, int theMods);
  void on_mouse_move(const ScreenCoords& screen_coords);

  // Geometry related
  ScreenCoords          get_screen_coords(const gp_Pnt& point);
  std::optional<gp_Pnt> pt3d_on_plane(const ScreenCoords& screen_coords, const gp_Pln& plane) const;
  void                  bake_transform_into_geometry(AIS_Shape_ptr& shape);
  gp_Pln                get_view_plane(const gp_Pnt& point_on_plane) const;

  // Query related
  AIS_Shape_ptr           get_shape(const ScreenCoords& screen_coords);
  std::optional<gp_Pnt2d> pt_on_plane(const ScreenCoords& screen_coords, const gp_Pln& plane) const;

  /// Axis-aligned bounds in sketch-plane 2D (gp_Pln UV) for the current view frustum intersecting \a pln.
  /// Uses \a display_w / \a display_h in the same pixel space as ImGui / GLFW cursor (full window).
  /// Returns false if the plane is not visible (e.g. headless or parallel view).
  bool sketch_plane_view_aabb_2d(const gp_Pln& pln, double display_w, double display_h, double& out_min_u, double& out_min_v,
                                 double& out_max_u, double& out_max_v) const;
  bool get_camera(gp_Pnt& out_eye, gp_Pnt& out_center, gp_Dir& out_up) const;
  void set_camera(const gp_Pnt& eye, const gp_Pnt& center, const gp_Dir& up);

  /// Roll the view about screen Z (view depth axis) by \a degrees, via \c V3d_View::Turn(\c V3d_Z, ...).
  void roll_view_z_deg(double degrees);

  /// Orbit the view like left-drag on the trihedron: \a yaw_deg about camera up (positive = orbit left),
  /// \a pitch_deg about camera side (positive = orbit up). Matches \c AIS_ViewController orbit axes.
  void orbit_view_screen_step_deg(double yaw_deg, double pitch_deg);

  /// Zoom like one mouse wheel notch at the cursor (\a wheel_notches > 0 zooms in; same units as \c on_mouse_scroll).
  /// \param shift_finer_zoom Blender-style finer step when Shift is held (keyboard or scroll).
  void zoom_view_wheel_notches(double wheel_notches, bool shift_finer_zoom = false);

  /// Clamp and store scroll-scale used by \c on_mouse_scroll / \c zoom_view_wheel_notches (from Settings JSON).
  void set_zoom_scroll_scale(double scale);

  /// Snap orientation to the nearest world-axis orthographic view (+/-X/Y/Z), roll zero; keeps eye-center distance.
  void snap_view_to_nearest_standard_axis();

  GUI&                    gui();
  AIS_InteractiveContext& ctx();

  // Selection related
  std::vector<AIS_Shape_ptr> get_selected() const;
  TopAbs_ShapeEnum           get_shp_selection_mode() const;
  void                       set_shp_selection_mode(const TopAbs_ShapeEnum selection_mode);

  /// Highlight \a shp in the 3D viewer while the Shape List row is hovered (null clears).
  void           set_shape_list_hover(const Shp_ptr& shp);
  const Shp_ptr& shape_list_hover() const { return m_shape_list_hover; }
  /// Highlight \a sketch geometry (including underlay) while its Sketch List row is hovered (null clears).
  void           set_sketch_list_hover(const Sketch_ptr& sketch);
  const Sketch_ptr& sketch_list_hover() const { return m_sketch_list_hover; }
  /// Highlight a sketch length dimension while its Sketch List row is hovered (\a dim_index == SIZE_MAX clears).
  void set_sketch_list_measurement_hover(const Sketch_ptr& sketch, size_t dim_index);
  /// Re-apply list-hover highlight after Settings changes the hover color.
  void refresh_shape_list_hover_highlight();

  // Material related
  const Graphic3d_MaterialAspect& get_default_material() const;
  void                            set_default_material(const Graphic3d_MaterialAspect& mat);
  /// Apply default material/color and wasm GLES presentation fixes; call before Redisplay.
  void refresh_shape_shading_(const Shp_ptr& shp);

  // View presentation (background gradient) and grid colors (0-1 RGB)
  void get_bg_gradient_colors(float color1[3], float color2[3]) const;
  void set_bg_gradient_colors(float r1, float g1, float b1, float r2, float g2, float b2);
  int  get_bg_gradient_method() const;
  void set_bg_gradient_method(int method);
  void get_grid_colors(float color1[3], float color2[3]) const;
  void set_grid_colors(float r1, float g1, float b1, float r2, float g2, float b2);

  void get_occt_grid_rect_params(Occt_grid_rect_params& out) const;
  void set_occt_grid_rect_params(const Occt_grid_rect_params& p);
  bool get_grid_visible() const;
  void set_grid_visible(bool visible);
  /// Recompute grid bounds from the active sketch (call after sketch geometry changes).
  void refresh_active_sketch_grid();

  bool is_headless() const;

  void new_file();

private:
  friend class Shp_operation_base;
  friend class View_access;

  // Sketch related
  void create_sketch_from_planar_face_(const ScreenCoords& screen_coords);
  void finalize_sketch_extrude_();
  bool cancel_sketch_extrude_();
  void create_default_sketch_();
  void ensure_current_sketch_();
  void remove_selected_length_dimensions_from_sketches_();

  // Query related
  const TopoDS_Shape* get_(const ScreenCoords& screen_coords) const;
  const TopoDS_Face*  get_face_(const ScreenCoords& screen_coords) const;
  const TopoDS_Wire*  get_wire_(const ScreenCoords& screen_coords) const;
  const TopoDS_Edge*  get_edge_(const ScreenCoords& screen_coords) const;

  // Hit point related
  Ray                   get_hit_test_ray_(const ScreenCoords& screen_coords) const;
  std::optional<gp_Pnt> get_hit_point_(const AIS_Shape_ptr& shp, const ScreenCoords& screen_coords) const;

  void        add_shp_(Shp_ptr& shp);
  std::string unique_shape_name_(const char* base_name) const;

  TopoDS_Shape         shape_with_local_transform_(const AIS_Shape_ptr& ais) const;
  [[nodiscard]] Status build_export_shape_(TopoDS_Shape& out_shape) const;

  void                         update_view_background_();
  static Occt_grid_rect_params clamp_occt_grid_rect_params_(Occt_grid_rect_params g);
  void                         capture_occt_grid_rect_from_viewer_(const V3d_Viewer_ptr& viewer);
  void                         sync_grid_plane_to_active_sketch_();
  void                         refresh_viewer_grid_();
  void                         apply_occt_grid_rect_to_viewer_();
  void                         apply_grid_visibility_();
  struct Grid_layout
  {
    gp_Ax3 plane;
    double size_x{0.};
    double size_y{0.};
  };
  [[nodiscard]] Grid_layout compute_grid_layout_() const;
  [[nodiscard]] gp_Ax3             grid_display_plane_() const;

  //! GLFW callback redirecting messages into Message::DefaultMessenger().
  // static void errorCallback(int theError, const char* theDescription);
  static Aspect_VKeyMouse mouse_button_from_glfw_(int theButton);
  static Aspect_VKeyFlags key_flags_from_glfw_(int theFlags);

  /// Maps wheel delta to OCCT zoom units using \ref m_zoom_scroll_scale and optional Shift (x0.1).
  int zoom_scroll_delta_int_(double wheel_y, bool shift_finer_zoom) const;

  GUI&                       m_gui;
  AIS_InteractiveContext_ptr m_ctx;
  V3d_View_ptr               m_view;
  Occt_glfw_win_ptr          m_occt_window;
  // Undo / redo
  static constexpr size_t k_max_undo{50};

  struct Undo_entry
  {
    std::unique_ptr<Delta> delta;
    std::string            json; // Full-document snapshot when `delta` is null
    Mode                   mode; // Mode at time of operation; restored when navigating stacks
  };

  std::vector<Undo_entry> m_undo_stack;
  std::vector<Undo_entry> m_redo_stack;
  bool                    m_restoring{false};
  Ezy_asset_store         m_assets;

  // --------------------------------------------------------------------
  // Dimension related
  bool                              m_show_dim_input{false};
  double                            m_dimension_scale{100.0};
  std::optional<double>             m_entered_dim;
  std::list<Shp_ptr>                m_shps;
  Sketch_list                       m_sketches;
  std::shared_ptr<Sketch>           m_cur_sketch;
  TopAbs_ShapeEnum                  m_shp_selection_mode{TopAbs_SHAPE};
  Shp_ptr                                    m_shape_list_hover;
  Sketch_ptr                                 m_sketch_list_hover;
  std::vector<AIS_InteractiveObject_ptr>     m_sketch_list_hover_ais;
  PrsDim_LengthDimension_ptr                 m_sketch_list_measurement_hover;
  opencascade::handle<Prs3d_Drawer>          m_shape_list_hover_drawer;
  void                                       update_shape_list_hover_drawer_();
  void                                       clear_sketch_list_hover_ais_();
  void                                       apply_sketch_list_hover_highlight_();
  void                                       apply_sketch_list_measurement_hover_style_();
  void                                       restore_sketch_list_measurement_hover_style_();
  void                                       refresh_sketch_list_measurement_hover_highlight_();
  Graphic3d_MaterialAspect          m_default_material;
  bool                              m_headless_view{false};
  /// True when LMB press was handled by planar-face sketch creation without AIS_ViewController::PressMouseButton (pair with
  /// release skip).
  bool m_planar_face_lmb_skipped_view_controller{false};
  // OCCT view colors; defaults match what we render (set explicitly in init_viewer())
  glm::vec3             m_bg_color1{0.037552f, 0.040503f, 0.042471f};
  glm::vec3             m_bg_color2{0.043440f, 0.174068f, 0.239382f};
  int                   m_bg_gradient_method{1}; // 0=HOR, 1=VER, 2=DIAG1, ...
  glm::vec3             m_grid_color1{0.112683f, 0.056886f, 0.138996f};
  glm::vec3             m_grid_color2{0.117917f, 0.117917f, 0.135135f};
  Occt_grid_rect_params m_occt_grid_rect{};
  bool                  m_grid_visible{true};
  /// User setting: same role as former literal in `UpdateZoom(Aspect_ScrollDelta(..., int(y * scale)))`.
  double m_zoom_scroll_scale{4.0};
  // --------------------------------------------------------------------
  // Operations
  Shp_move   m_shp_move;
  Shp_rotate m_shp_rotate;
  Shp_scale  m_shp_scale;
  // --------------------------------------------------------------------
  // Commands
  Shp_chamfer   m_shp_chamfer;
  Shp_fillet    m_shp_fillet;
  Shp_cut       m_shp_cut;
  Shp_fuse      m_shp_fuse;
  Shp_common    m_shp_common;
  Shp_polar_dup m_shp_polar_dup;
  Shp_extrude   m_shp_extrude;
  // --------------------------------------------------------------------
  // Selection related
  std::map<Mode, TopAbs_ShapeEnum> m_modes_selection_mode_map;
};

template <typename Shp_ptr_t, typename T>
void show(AIS_InteractiveContext& ctx, Shp_ptr_t& shp, const T& obj, bool redraw = true);

template <typename Shp_ptr_t, typename T>
void show(AIS_InteractiveContext& ctx, Sketch& owner, Shp_ptr_t& shp, const T& obj, bool redraw = true);

#include "occt_view.inl"