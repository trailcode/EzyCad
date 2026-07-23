#pragma once

#include <Standard_Handle.hxx>
#include <glm/glm.hpp>
#include <memory>

#include "skt_ais.h"

class TopoDS_Shape;
class Sketch;

class AIS_InteractiveObject;
class AIS_InteractiveContext;
class AIS_Shape;
class AIS_TexturedShape;
class AIS_ViewCube;
class Aspect_DisplayConnection;
class Aspect_Grid;
class Aspect_RectangularGrid;
class Cocoa_Window;
class Font_FontMgr;
class Font_SystemFont;
class Geom2d_Curve;
class Geom_Circle;
class Geom_Curve;
class Geom_Line;
class Geom_Plane;
class Geom_Surface;
class Geom_TrimmedCurve;
class Graphic3d_AspectFillArea3d;
class Graphic3d_AspectText3d;
class Graphic3d_Camera;
class Graphic3d_CLight;
class Graphic3d_ClipPlane;
class Image_PixMap;
class Occt_glfw_win;
class OpenGl_GraphicDriver;
class Poly_Triangulation;
class Prs3d_ArrowAspect;
class Prs3d_DimensionAspect;
class Prs3d_Drawer;
class Prs3d_LineAspect;
class Prs3d_ShadingAspect;
class Prs3d_TextAspect;
class PrsDim_LengthDimension;
class StdSelect_BRepOwner;
class V3d_RectangularGrid;
class V3d_View;
class V3d_Viewer;
class Wasm_Window;
class WNT_WClass;
class WNT_Window;
class Xw_Window;
class TDocStd_Document;
class XCAFApp_Application;
class XCAFDoc_ShapeTool;
class TDataStd_Name;

using AIS_InteractiveObject_ptr      = opencascade::handle<AIS_InteractiveObject>;
using AIS_InteractiveContext_ptr     = opencascade::handle<AIS_InteractiveContext>;
using AIS_Shape_ptr                  = opencascade::handle<AIS_Shape>;
using AIS_TexturedShape_ptr          = opencascade::handle<AIS_TexturedShape>;
using AIS_ViewCube_ptr               = opencascade::handle<AIS_ViewCube>;
using Aspect_DisplayConnection_ptr   = opencascade::handle<Aspect_DisplayConnection>;
using Aspect_Grid_ptr                = opencascade::handle<Aspect_Grid>;
using Aspect_RectangularGrid_ptr     = opencascade::handle<Aspect_RectangularGrid>;
using Cocoa_Window_ptr               = opencascade::handle<Cocoa_Window>;
using Font_FontMgr_ptr               = opencascade::handle<Font_FontMgr>;
using Font_SystemFont_ptr            = opencascade::handle<Font_SystemFont>;
using Geom2d_Curve_ptr               = opencascade::handle<Geom2d_Curve>;
using Geom_Circle_ptr                = opencascade::handle<Geom_Circle>;
using Geom_Curve_ptr                 = opencascade::handle<Geom_Curve>;
using Geom_Line_ptr                  = opencascade::handle<Geom_Line>;
using Geom_Plane_ptr                 = opencascade::handle<Geom_Plane>;
using Geom_Surface_ptr               = opencascade::handle<Geom_Surface>;
using Geom_TrimmedCurve_ptr          = opencascade::handle<Geom_TrimmedCurve>;
using Graphic3d_AspectFillArea3d_ptr = opencascade::handle<Graphic3d_AspectFillArea3d>;
using Graphic3d_AspectText3d_ptr     = opencascade::handle<Graphic3d_AspectText3d>;
using Graphic3d_Camera_ptr           = opencascade::handle<Graphic3d_Camera>;
using Graphic3d_CLight_ptr           = opencascade::handle<Graphic3d_CLight>;
using Graphic3d_ClipPlane_ptr        = opencascade::handle<Graphic3d_ClipPlane>;
using Image_PixMap_ptr               = opencascade::handle<Image_PixMap>;
using Occt_glfw_win_ptr              = opencascade::handle<Occt_glfw_win>;
using OpenGl_GraphicDriver_ptr       = opencascade::handle<OpenGl_GraphicDriver>;
using Poly_Triangulation_ptr         = opencascade::handle<Poly_Triangulation>;
using Prs3d_ArrowAspect_ptr          = opencascade::handle<Prs3d_ArrowAspect>;
using Prs3d_DimensionAspect_ptr      = opencascade::handle<Prs3d_DimensionAspect>;
using Prs3d_Drawer_ptr               = opencascade::handle<Prs3d_Drawer>;
using Prs3d_LineAspect_ptr           = opencascade::handle<Prs3d_LineAspect>;
using Prs3d_ShadingAspect_ptr        = opencascade::handle<Prs3d_ShadingAspect>;
using Prs3d_TextAspect_ptr           = opencascade::handle<Prs3d_TextAspect>;
using PrsDim_LengthDimension_ptr     = opencascade::handle<PrsDim_LengthDimension>;
using Sketch_AIS_edge_ptr            = opencascade::handle<Sketch_AIS_edge>;
using Sketch_AIS_node_mark_ptr       = opencascade::handle<Sketch_AIS_node_mark>;
using Sketch_face_shp_ptr            = opencascade::handle<Sketch_face_shp>;
using StdSelect_BRepOwner_ptr        = opencascade::handle<StdSelect_BRepOwner>;
using TDataStd_Name_ptr              = opencascade::handle<TDataStd_Name>;
using TDocStd_Document_ptr           = opencascade::handle<TDocStd_Document>;
using V3d_RectangularGrid_ptr        = opencascade::handle<V3d_RectangularGrid>;
using V3d_View_ptr                   = opencascade::handle<V3d_View>;
using V3d_Viewer_ptr                 = opencascade::handle<V3d_Viewer>;
using Wasm_Window_ptr                = opencascade::handle<Wasm_Window>;
using WNT_WClass_ptr                 = opencascade::handle<WNT_WClass>;
using WNT_Window_ptr                 = opencascade::handle<WNT_Window>;
using XCAFApp_Application_ptr        = opencascade::handle<XCAFApp_Application>;
using XCAFDoc_ShapeTool_ptr          = opencascade::handle<XCAFDoc_ShapeTool>;
using Xw_Window_ptr                  = opencascade::handle<Xw_Window>;

#define DECL_PTR(TypeName)                                                                                                     \
  using uptr = std::unique_ptr<TypeName>;                                                                                      \
  using sptr = std::shared_ptr<TypeName>;                                                                                      \
  using wptr = std::weak_ptr<TypeName>;

// Primary template for non-vector types
template <typename T> class SafeType
{
private:
  T m_value;

public:
  // clang-format off
  explicit SafeType(T initial_value) : m_value(initial_value) {}
        T& unsafe_get()       { return m_value; }
  const T& unsafe_get() const { return m_value; }
  // clang-format on
};

using ScreenCoords = SafeType<glm::dvec2>;

/// Target format for CAD/mesh export (STEP, IGES, binary STL, PLY).
enum class Export_format
{
  Step,
  Iges,
  Stl,
  Ply
};

/// Length unit for CAD/mesh export coordinates (and STEP/IGES declared unit).
enum class Export_unit
{
  Inch,
  Millimeter
};

/// Project display/input length unit. Model space stays inch-scaled
/// (`model = inches * dimension_scale`); this only affects UI and dimensions.
enum class Project_unit
{
  Inch,
  Millimeter
};

/// Millimeters per inch (display conversion and CAD interchange).
inline constexpr double k_mm_per_inch = 25.4;

#include "utl_types.inl"