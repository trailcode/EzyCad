# Occt_view Class Documentation

## Overview
The `Occt_view` class is a core component of a CAD-like application built with Open CASCADE Technology (OCCT). It manages a 3D view, handling rendering, user interactions, shape operations, sketching, and geometry queries. The class integrates with a GUI for mode management, supports operations like move, rotate, and extrude, and provides functionality for importing/exporting models. It inherits from `AIS_ViewController` to handle input events and uses GLFW for window management, supporting both desktop and WebAssembly (`__EMSCRIPTEN__`) environments.

## Class Details
- **Header File**: `occt_view.h`
- **Source File**: `occt_view.cpp`
- **Base Class**: `AIS_ViewController` (protected inheritance)
- **Namespace**: Not explicitly defined in the provided code
- **Dependencies**: OCCT (`AIS_InteractiveContext`, `V3d_View`, `Graphic3d_MaterialAspect`, `BRepTools`, `STEPControl_Reader`, etc.), GLFW, ImGui, nlohmann JSON, project-specific headers (`gui.h`, `sketch.h`, `utl.h`, `geom.h`, `shp_*.h`)

### Key Structures
- **`Ray`**: Represents a ray with an origin (`gp_Pnt`) and direction (`gp_Dir`), used for hit testing.
- **`Set_parent_mode`**: Enum (`Yes`, `No`) to control whether to revert to the parent mode after cancellation.
- **`Sketch_ptr`**: Alias for `std::shared_ptr<Sketch>`, representing a sketch.
- **`Sketch_list`**: Alias for `std::list<Sketch_ptr>`, storing all sketches.

### Private Members
- `m_gui`: Reference to the `GUI` for mode and interaction management.
- `m_ctx`: `AIS_InteractiveContext_ptr` for managing interactive objects.
- `m_view`: `V3d_View_ptr` for 3D rendering.
- `m_occt_window`: `Occt_glfw_win_ptr` for GLFW window integration.
- `m_curr_view_pln`: `gp_Pln` for the current view plane.
- **Dimension-related**: `m_show_dim_input` (bool), `m_dimension_scale` (double), `m_entered_dim` (optional double) for handling dimension inputs.
- **Extrude-related**: `m_to_extrude` (`AIS_Shape_ptr`), `m_to_extrude_pln` (`gp_Pln`), `m_to_extrude_pt` (optional `gp_Pnt`), `m_extruded` (`ExtrudedShp_ptr`), `m_tmp_dim` (`PrsDim_LengthDimension_ptr`) for extrusion operations.
- `m_shps`: `std::list<ShapeBase_ptr>` storing all shapes.
- `m_sketches`: `Sketch_list` storing all sketches.
- `m_cur_sketch`: `std::shared_ptr<Sketch>` for the current sketch.
- `m_shp_selection_mode`: `TopAbs_ShapeEnum` for the current shape selection mode (e.g., `TopAbs_SHAPE`, `TopAbs_FACE`).
- `m_default_material`: `Graphic3d_MaterialAspect` for default shape material.
- `m_headless_view`: Bool indicating headless mode (e.g., for unit tests).
- **Operations**: Instances of `Shp_move`, `Shp_rotate`, `Shp_chamfer`, `Shp_cut`, `Shp_fuse`, `Shp_common`, `Shp_polar_dup` for shape operations.

## Public Methods
| Method | Description | Return Type |
|--------|-------------|-------------|
| `Occt_view(GUI& gui)` | Constructor; initializes with a GUI reference and operation instances. | - |
| `~Occt_view()` | Destructor; cleans up resources. | - |
| `void init_window(GLFWwindow* GlfwWindow)` | Initializes the GLFW window for rendering. | - |
| `void init_viewer()` | Sets up the OCCT viewer, context, and rendering parameters (e.g., MSAA, shadows). | - |
| `void init_default()` | Initializes default settings (e.g., sketch, view orientation, trihedron). | - |
| `std::string to_json() const` | Serializes sketches and shapes to JSON (BREP format). | `std::string` |
| `void load(const std::string& json_str)` | Loads sketches and shapes from JSON, clearing existing data. | - |
| `bool import_step(const std::string& file_path)` | Imports shapes from STEP data, adding them as `ExtrudedShp`. | `bool` |
| `void do_frame()` | Updates and redraws the view. | - |
| `void on_mode()` | Handles mode changes, updating sketch visibility and selection modes. | - |
| `void on_chamfer_mode()` | Updates selection mode based on chamfer mode (edge, wire, face, shape). | - |
| `Mode get_mode() const` | Returns the current GUI mode. | `Mode` |
| `void cleanup()` | Cleans up view, window, and GLFW resources. | - |
| `void delete_selected()` | Deletes selected shapes and cancels operations. | - |
| `void delete_(std::vector<AIS_Shape_ptr>& to_delete)` | Deletes specified shapes, handling sketch edges. | - |
| `void cancel(Set_parent_mode set_parent_mode)` | Cancels operations (e.g., move, rotate, extrude) and optionally sets parent mode. | - |
| `Sketch_list& get_sketches()` | Returns the list of sketches. | `Sketch_list&` |
| `void remove_sketch(const Sketch_ptr& sketch)` | Removes a sketch, updating the current sketch if needed. | - |
| `Sketch& curr_sketch()` | Returns the current sketch. | `Sketch&` |
| `void set_curr_sketch(const Sketch_ptr& sketch)` | Sets the current sketch, updating visibility. | - |
| `void sketch_face_extrude(const ScreenCoords& screen_coords)` | Handles face extrusion, including dimension input and visualization. | - |
| `std::list<ShapeBase_ptr>& get_shapes()` | Returns the list of shapes. | `std::list<ShapeBase_ptr>&` |
| `Shp_move& shp_move()` | Returns the move operation instance. | `Shp_move&` |
| `Shp_rotate& shp_rotate()` | Returns the rotate operation instance. | `Shp_rotate&` |
| `Shp_chamfer& shp_chamfer()` | Returns the chamfer operation instance. | `Shp_chamfer&` |
| `Shp_cut& shp_cut()` | Returns the cut operation instance. | `Shp_cut&` |
| `Shp_fuse& shp_fuse()` | Returns the fuse operation instance. | `Shp_fuse&` |
| `Shp_common& shp_common()` | Returns the common operation instance. | `Shp_common&` |
| `Shp_polar_dup& shp_polar_dup()` | Returns the polar duplicate operation instance. | `Shp_polar_dup&` |
| `void revolve_selected(const double angle)` | Revolves selected shapes by the specified angle. | - |
| `void on_enter(const ScreenCoords& screen_coords)` | Handles enter key events (e.g., finalize extrusion, sketch actions). | - |
| `bool fit_face_in_view(const TopoDS_Face& face)` | Adjusts the view to fit a face, aligning the camera to its plane. | `bool` |
| `void dimension_input(const ScreenCoords& screen_coords)` | Initiates dimension input for extrusion or sketch modes. | - |
| `double get_dimension_scale() const` | Returns the dimension scale factor. | `double` |
| `void on_resize(int theWidth, int theHeight)` | Handles window resize events, updating the view. | - |
| `void on_mouse_scroll(double theOffsetX, double theOffsetY)` | Handles mouse scroll events, updating zoom. | - |
| `void on_mouse_button(int theButton, int theAction, int theMods)` | Handles mouse button events (e.g., selection, sketch creation). | - |
| `void on_mouse_move(const ScreenCoords& screen_coords)` | Handles mouse movement, updating the view. | - |
| `ScreenCoords get_screen_coords(const gp_Pnt& point)` | Converts a 3D point to screen coordinates. | `ScreenCoords` |
| `std::optional<gp_Pnt> pt3d_on_plane(const ScreenCoords& screen_coords, const gp_Pln& plane) const` | Projects screen coordinates to a 3D point on a plane. | `std::optional<gp_Pnt>` |
| `void bake_transform_into_geometry(AIS_Shape_ptr& shape)` | Applies a shape’s local transformation to its geometry, resetting the transformation. | - |
| `gp_Pln get_view_plane(const gp_Pnt& point_on_plane) const` | Returns the view plane passing through a point. | `gp_Pln` |
| `AIS_Shape_ptr get_shape(const ScreenCoords& screen_coords)` | Returns the shape at the specified screen coordinates. | `AIS_Shape_ptr` |
| `std::optional<gp_Pnt2d> pt_on_plane(const ScreenCoords& screen_coords, const gp_Pln& plane) const` | Projects screen coordinates to a 2D point on a plane. | `std::optional<gp_Pnt2d>` |
| `GUI& gui()` | Returns the GUI instance. | `GUI&` |
| `AIS_InteractiveContext& ctx()` | Returns the interactive context. | `AIS_InteractiveContext&` |
| `std::vector<AIS_Shape_ptr> get_selected() const` | Returns the list of selected shapes. | `std::vector<AIS_Shape_ptr>` |
| `TopAbs_ShapeEnum get_shp_selection_mode() const` | Returns the current shape selection mode. | `TopAbs_ShapeEnum` |
| `void set_shp_selection_mode(const TopAbs_ShapeEnum mode)` | Sets the shape selection mode. | - |
| `const Graphic3d_MaterialAspect& get_default_material() const` | Returns the default material. | `const Graphic3d_MaterialAspect&` |
| `void set_default_material(const Graphic3d_MaterialAspect& mat)` | Sets the default material. | - |
| `bool is_headless() const` | Returns whether the view is in headless mode. | `bool` |
| `void new_file()` | Clears shapes and sketches, creating a default sketch. | - |

### Private Methods
| Method | Description |
|--------|-------------|
| `void create_sketch_from_face_(const ScreenCoords& screen_coords)` | Creates a sketch from a face at the specified coordinates. |
| `void finalize_sketch_extrude_()` | Finalizes face extrusion, adding the result to shapes. |
| `bool cancel_sketch_extrude_()` | Cancels face extrusion, removing temporary objects. |
| `void create_default_sketch_()` | Creates a default sketch on the XY plane. |
| `const TopoDS_Shape* get_(const ScreenCoords& screen_coords) const` | Returns the shape at the specified coordinates. |
| `const TopoDS_Face* get_face_(const ScreenCoords& screen_coords) const` | Returns the face at the specified coordinates. |
| `const TopoDS_Wire* get_wire_(const ScreenCoords& screen_coords) const` | Returns the wire at the specified coordinates. |
| `const TopoDS_Edge* get_edge_(const ScreenCoords& screen_coords) const` | Returns the edge at the specified coordinates. |
| `Ray get_hit_test_ray_(const ScreenCoords& screen_coords) const` | Returns a ray for hit testing at the specified coordinates. |
| `std::optional<gp_Pnt> get_hit_point_(const AIS_Shape_ptr& shp, const ScreenCoords& screen_coords) const` | Returns the hit point on a shape’s face. |
| `void add_shp_(ShapeBase_ptr& shp)` | Adds a shape to the view, setting material and selection mode. |
| `static Aspect_VKeyMouse mouse_button_from_glfw_(int theButton)` | Converts GLFW mouse button to OCCT key. |
| `static Aspect_VKeyFlags key_flags_from_glfw_(int theFlags)` | Converts GLFW modifier flags to OCCT flags. |

## Functionality
- **Rendering**: Manages a 3D view with OCCT, supporting perspective/orthographic projections, MSAA, shadows, and transparency.
- **Shape Operations**: Integrates operations like move, rotate, chamfer, cut, fuse, common, and polar duplicate via member instances.
- **Sketching**: Supports sketch creation, extrusion, and revolve operations, managing multiple sketches.
- **Input Handling**: Processes mouse, keyboard, and resize events via GLFW, supporting selection, navigation, and operation triggers.
- **Geometry Queries**: Provides methods for coordinate projection, shape selection, and hit testing.
- **Import/Export**: Supports JSON serialization of sketches/shapes and STEP file import.
- **Mode Management**: Coordinates with the GUI to handle modes (e.g., sketch, move, rotate), updating visibility and selection.
- **Visualization**: Displays grids, view cubes, trihedrons, and dimension annotations.
- **Headless Mode**: Supports unit testing without a rendering context.

## Usage Example
```cpp
#include "occt_view.h"
#include "gui.h"

// Initialize GUI and view
GUI gui;
Occt_view view(gui);

// Initialize window and viewer
GLFWwindow* window = glfwCreateWindow(800, 600, "CAD App", nullptr, nullptr);
view.init_window(window);
view.init_viewer();
view.init_default();

// Handle mouse input for rotation
ScreenCoords mouse_pos(200, 300);
view.shp_rotate().set_rotation_axis(Rotation_axis::Z_axis);
Status status = view.shp_rotate().rotate_selected(mouse_pos);
if (!status.is_ok()) {
    // Handle error
}

// Import a STEP file
std::string step_data = "STEP file content...";
view.import_step(step_data);

// Save to JSON
std::string json = view.to_json();

// Clean up
view.cleanup();