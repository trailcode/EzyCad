#pragma once

#include <Standard_Handle.hxx>
#include <glm/glm.hpp>

class AIS_InteractiveObject;
class AIS_InteractiveContext;
class Occt_glfw_win;
class V3d_View;
class V3d_Viewer;
class AIS_Shape;
class Geom_TrimmedCurve;
class Geom_Plane;
class Geom_Line;
class Geom_Curve;
class Geom_Surface;
class PrsDim_LengthDimension;
class Graphic3d_Camera;
class StdSelect_BRepOwner;

using AIS_InteractiveObject_ptr  = opencascade::handle<AIS_InteractiveObject>;
using AIS_InteractiveContext_ptr = opencascade::handle<AIS_InteractiveContext>;
using Occt_glfw_win_ptr          = opencascade::handle<Occt_glfw_win>;
using V3d_View_ptr               = opencascade::handle<V3d_View>;
using V3d_Viewer_ptr             = opencascade::handle<V3d_Viewer>;
using AIS_Shape_ptr              = opencascade::handle<AIS_Shape>;
using Geom_TrimmedCurve_ptr      = opencascade::handle<Geom_TrimmedCurve>;
using Geom_Plane_ptr             = opencascade::handle<Geom_Plane>;
using Geom_Line_ptr              = opencascade::handle<Geom_Line>;
using Geom_Curve_ptr             = opencascade::handle<Geom_Curve>;
using Geom_Surface_ptr           = opencascade::handle<Geom_Surface>;
using PrsDim_LengthDimension_ptr = opencascade::handle<PrsDim_LengthDimension>;
using Graphic3d_Camera_ptr       = opencascade::handle<Graphic3d_Camera>;
using StdSelect_BRepOwner_ptr    = opencascade::handle<StdSelect_BRepOwner>;

// Primary template for non-vector types
template <typename T>
class SafeType
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

#include "types.inl"