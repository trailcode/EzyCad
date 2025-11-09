#pragma once

#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <AIS_ViewController.hxx>
#include <Graphic3d_MaterialAspect.hxx>
#include <list>
#include <memory>
#include <optional>

#include "occt_glfw_win.h"
#include "shp_chamfer.h"
#include "shp_common.h"
#include "shp_cut.h"
#include "shp_extrude.h"
#include "shp_fuse.h"
#include "shp_move.h"
#include "shp_polar_dup.h"
#include "shp_rotate.h"

class Sketch;
class GUI;
class TopoDS_Face;
class TopoDS_Wire;
class TopoDS_Edge;
enum class Mode;
enum class Command;

struct Ray
{
  gp_Pnt origin;
  gp_Dir direction;

  Ray(const gp_Pnt& orig, const gp_Dir& dir)
      : origin(orig), direction(dir) {}
};

enum class Set_parent_mode
{
  Yes,
  No
};

class Occt_view : protected AIS_ViewController
{
 public:
  using Sketch_ptr  = std::shared_ptr<Sketch>;
  using Sketch_list = std::list<Sketch_ptr>;

  Occt_view(GUI& gui);
  ~Occt_view();

  // Initialization related.
  void init_window(GLFWwindow* GlfwWindow);
  void init_viewer();
  void init_default();

  std::string to_json() const;
  void        load(const std::string& json_str);
  bool        import_step(const std::string& file_path);

  void do_frame();

  // Mode related.
  void on_mode();
  void on_chamfer_mode();
  Mode get_mode() const;

  void cleanup();

  // Delete related.
  void delete_selected();
  void delete_(std::vector<AIS_Shape_ptr>& to_delete);

  //  Member function to delete variable arguments
  template <typename... Args> void remove(Args&&... args);

  void cancel(Set_parent_mode set_parent_mode);

  // Sketch related
  Sketch_list& get_sketches();
  void         remove_sketch(const Sketch_ptr& sketch);
  Sketch&      curr_sketch();
  void         set_curr_sketch(const Sketch_ptr& sketch);
  void         sketch_face_extrude(const ScreenCoords& screen_coords, bool is_mouse_move);

  std::list<ShapeBase_ptr>& get_shapes();

  // Shape related
  Shp_move&      shp_move();
  Shp_rotate&    shp_rotate();
  Shp_chamfer&   shp_chamfer();
  Shp_cut&       shp_cut();
  Shp_fuse&      shp_fuse();
  Shp_common&    shp_common();
  Shp_polar_dup& shp_polar_dup();
  Shp_extrude&   shp_extrude();

  // Revolve related
  void revolve_selected(const double angle);

  void on_enter(const ScreenCoords& screen_coords);  // For manual dimension distance keyboard input.
  bool fit_face_in_view(const TopoDS_Face& face);

  // Dimension related
  void                    dimension_input(const ScreenCoords& screen_coords);
  double                  get_dimension_scale() const;
  bool                    get_show_dim_input() const;
  void                    set_show_dim_input(bool show);
  std::optional<double>   get_entered_dim() const;
  void                    set_entered_dim(const std::optional<double>& dim);

  // Input events.
  void on_resize(int theWidth, int theHeight);
  void on_mouse_scroll(double theOffsetX, double theOffsetY);
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

  GUI&                    gui();
  AIS_InteractiveContext& ctx();

  // Selection related
  std::vector<AIS_Shape_ptr> get_selected() const;
  TopAbs_ShapeEnum           get_shp_selection_mode() const;
  void                       set_shp_selection_mode(const TopAbs_ShapeEnum mode);

  // Material related
  const Graphic3d_MaterialAspect& get_default_material() const;
  void                            set_default_material(const Graphic3d_MaterialAspect& mat);

  bool is_headless() const;

  void new_file();

 private:
  friend class Shp_operation_base;
  friend class Shp_chamfer;  // TODO remove
  friend class Shp_extrude;  // TODO remove
  friend class View_access;

  // Sketch related
  void create_sketch_from_face_(const ScreenCoords& screen_coords);
  void finalize_sketch_extrude_();
  bool cancel_sketch_extrude_();
  void create_default_sketch_();

  // Query related
  const TopoDS_Shape* get_(const ScreenCoords& screen_coords) const;
  const TopoDS_Face*  get_face_(const ScreenCoords& screen_coords) const;
  const TopoDS_Wire*  get_wire_(const ScreenCoords& screen_coords) const;
  const TopoDS_Edge*  get_edge_(const ScreenCoords& screen_coords) const;

  // Hit point related
  Ray                   get_hit_test_ray_(const ScreenCoords& screen_coords) const;
  std::optional<gp_Pnt> get_hit_point_(const AIS_Shape_ptr& shp, const ScreenCoords& screen_coords) const;

  // TODO group in .cpp file
  void add_shp_(ShapeBase_ptr& shp);

  //! GLFW callback redirecting messages into Message::DefaultMessenger().
  // static void errorCallback(int theError, const char* theDescription);
  static Aspect_VKeyMouse mouse_button_from_glfw_(int theButton);
  static Aspect_VKeyFlags key_flags_from_glfw_(int theFlags);

  GUI&                       m_gui;
  AIS_InteractiveContext_ptr m_ctx;
  V3d_View_ptr               m_view;
  Occt_glfw_win_ptr          m_occt_window;
  // --------------------------------------------------------------------
  // Dimension related
  bool                       m_show_dim_input {false};
  double                     m_dimension_scale {100.0};
  std::optional<double>      m_entered_dim;
  std::list<ShapeBase_ptr>   m_shps;
  Sketch_list                m_sketches;
  std::shared_ptr<Sketch>    m_cur_sketch;
  TopAbs_ShapeEnum           m_shp_selection_mode {TopAbs_SHAPE};
  Graphic3d_MaterialAspect   m_default_material;
  bool                       m_headless_view {false};
  // --------------------------------------------------------------------
  // Operations
  Shp_move                   m_shp_move;
  Shp_rotate                 m_shp_rotate;
  // --------------------------------------------------------------------
  // Commands
  Shp_chamfer                m_shp_chamfer;
  Shp_cut                    m_shp_cut;
  Shp_fuse                   m_shp_fuse;
  Shp_common                 m_shp_common;
  Shp_polar_dup              m_shp_polar_dup;
  Shp_extrude                m_shp_extrude;
};

template <typename Shp_ptr_t, typename T>
void show(AIS_InteractiveContext& ctx, Shp_ptr_t& shp, const T& obj, bool redraw = true);

template <typename Shp_ptr_t, typename T>
void show(AIS_InteractiveContext& ctx, Sketch& owner, Shp_ptr_t& shp, const T& obj, bool redraw = true);

#include "occt_view.inl"