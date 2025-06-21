# EzyCad - 3D CAD Application

This is a **3D CAD modeling application** built with OpenCascade (OCCT) and modern C++. The codebase implements a feature-rich CAD system with both 2D sketching and 3D modeling capabilities.

## Core Architecture

**Main Components:**
- **`main.cpp`** - Application entry point with GLFW window management
- **`gui.cpp/h`** - ImGui-based user interface with file operations, toolbars, and mode management
- **`occt_view.cpp/h`** - Core 3D view management using OpenCascade, handles rendering, selection, and camera controls

## Key Features

### 1. File Operations
- New, Open, Save, Save As functionality (Ctrl+N, Ctrl+O, Ctrl+S)
- Import STEP files
- JSON-based project serialization

### 2. 2D Sketching System
- **`sketch.cpp/h`** - Complete 2D sketching with nodes, edges, and faces
- **`sketch_nodes.cpp/h`** - Node management with snapping functionality
- Support for lines, circles, arcs, rectangles, squares, slots
- Dimension annotations and constraints
- Operation axis for mirroring and revolving

### 3. 3D Modeling Operations
- **`shp_rotate.cpp/h`** - Shape rotation with multiple axis options (X, Y, Z, View-to-object)
- **`shp_move.cpp/h`** - Shape translation with axis constraints
- **`shp_chamfer.cpp/h`** - Chamfer operations on edges/wires/faces
- **`shp_cut.cpp/h`** - Boolean subtraction operations
- **`shp_fuse.cpp/h`** - Boolean union operations  
- **`shp_common.cpp/h`** - Boolean intersection operations
- **`shp_polar_dup.cpp/h`** - Polar array duplication

### 4. Geometry & Utilities
- **`geom.cpp/h`** - Geometric operations (2D/3D conversions, plane operations, area calculations)
- **`utl.cpp/h/inl`** - Utility functions including the `clear_all()` variadic template for type-safe clearing
- **`utl_json.cpp/h`** - JSON serialization for geometric primitives

### 5. Shape Management
- **`shp.cpp/h`** - Base shape class with display modes and selection
- **`shp_operation.cpp/h`** - Base class for shape operations

## Technical Highlights

### Modern C++ Features:
- Variadic templates (`clear_all()` utility)
- Type traits and SFINAE
- Smart pointers and RAII
- Template metaprogramming for type-safe operations

### OpenCascade Integration:
- Full 3D rendering with OpenGL
- Advanced selection and picking
- Boolean operations
- STEP file import/export

### User Interface:
- ImGui for immediate mode GUI
- GLFW for window management
- Multi-mode operation system
- Real-time preview and feedback

### Cross-Platform:
- Windows, macOS, Linux support
- Emscripten/WebGL support for web deployment

## Design Patterns

- **Command Pattern** - For shape operations
- **State Pattern** - For different interaction modes
- **Observer Pattern** - For GUI updates
- **Factory Pattern** - For shape creation

## Key Utilities

### `clear_all()` Function
A sophisticated variadic template function that provides type-safe clearing of multiple variables:

```cpp
template <typename T, typename... Args>
void clear_all(T& arg, Args&... args);
```

**Supported types:**
- Containers with `clear()` method (e.g., `std::vector`, `std::string`)
- Smart pointers with `reset()` method (e.g., `std::unique_ptr`, `std::shared_ptr`)
- Arithmetic types (set to zero)
- `std::optional` (set to `std::nullopt`)
- Raw pointers (set to `nullptr`)

**Usage examples:**
```cpp
// Clear multiple variables of different types
clear_all(m_shps, m_sketches, m_cur_sketch, m_angle, m_center);

// Clear operation state
clear_all(m_to_extrude_pt, m_to_extrude, m_extruded, m_tmp_dim);
```

This is a sophisticated CAD application that demonstrates advanced C++ techniques while providing professional-grade 3D modeling capabilities. The code is well-structured with clear separation of concerns between UI, geometry, and rendering layers.