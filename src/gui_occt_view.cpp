#include "gui_occt_view.h"

#include <AIS_ViewCube.hxx>
#include <Aspect_GradientFillMethod.hxx>
#include <Aspect_Grid.hxx>
#include <Aspect_RectangularGrid.hxx>
#include <V3d_RectangularGrid.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <GeomAPI_IntCS.hxx>
#include <Geom_Line.hxx>
#include <Geom_Plane.hxx>
#include <Graphic3d_Camera.hxx>
#include <NCollection_IndexedDataMap.hxx>
#include <NCollection_Vec2.hxx>
#include <IGESControl_Writer.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <Precision.hxx>
#include <Prs3d_DatumAspect.hxx>
#include <Prs3d_Drawer.hxx>
#include <Graphic3d_AspectFillArea3d.hxx>
#include <Prs3d_LineAspect.hxx>
#include <PrsDim_LengthDimension.hxx>
#include <STEPControl_Reader.hxx>
#include <STEPControl_Writer.hxx>
#include <StdSelect_BRepOwner.hxx>
#include <StlAPI_Writer.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <V3d_TypeOfAxe.hxx>
#include <V3d_View.hxx>
#include <WNT_WClass.hxx>
#include <WNT_Window.hxx>
#include <algorithm>
#include <cmath>
#include <gp_Ax1.hxx>
#include <gp_Trsf.hxx>

#include "utl_dbg.h"
#include "delta.h"
#include "utl_geom.h"
#include "gui.h"
#include "utl_ply_io.h"
#include "shp.h"
#include "shp_create.h"
#include "sketch.h"
#include "sketch_ais.h"
#include "sketch_json.h"
#include "utl_types.h"
#include "utl.h"
#include "utl_json.h"
#include "utl_occt.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <Font_FontMgr.hxx>
#include <Font_NameOfFont.hxx>
#include <TCollection_AsciiString.hxx>
#include <Wasm_Window.hxx>
#endif

#include <GLFW/glfw3.h>

#include <iostream>
#include <sstream>

using namespace glm;

Occt_view::Occt_view(GUI& gui)
    : m_gui(gui)
    , m_shp_move(*this)
    , m_shp_rotate(*this)
    , m_shp_scale(*this)
    , m_shp_chamfer(*this)
    , m_shp_fillet(*this)
    , m_shp_cut(*this)
    , m_shp_fuse(*this)
    , m_shp_common(*this)
    , m_shp_polar_dup(*this)
    , m_shp_extrude(*this)
{
}

Occt_view::~Occt_view() {}

// Initialization related.
void Occt_view::init_window(GLFWwindow* GlfwWindow)
{
#ifndef __EMSCRIPTEN__
  m_occt_window = new Occt_glfw_win(GlfwWindow);
#endif
}

void Occt_view::init_viewer()
{
  double myDevicePixelRatio = 1.0;
#ifdef __EMSCRIPTEN__
  myDevicePixelRatio = emscripten_get_device_pixel_ratio();
#endif
#ifndef __EMSCRIPTEN__
  if (m_occt_window.IsNull() || m_occt_window->getGlfwWindow() == nullptr)
  {
    // create graphic driver
    Handle(Aspect_DisplayConnection) aDisp = new Aspect_DisplayConnection();
    Handle(OpenGl_GraphicDriver) aDriver   = new OpenGl_GraphicDriver(aDisp, true);
    aDriver->ChangeOptions().swapInterval  = 0; // no window, no swap
    Handle(V3d_Viewer) myViewer            = new V3d_Viewer(aDriver);

    // create offscreen window
    const TCollection_AsciiString aWinName("OCCT offscreen window");
    NCollection_Vec2<int>         aWinSize(512, 512);
#if defined(_WIN32)
    const TCollection_AsciiString aClassName("OffscreenClass");
    // empty callback!
    Handle(WNT_WClass) aWinClass = new WNT_WClass(aClassName.ToCString(), nullptr, 0);
    Handle(WNT_Window) aWindow   = new WNT_Window(aWinName.ToCString(), aWinClass, 0x80000000, // WS_POPUP
                                                  64, 64, aWinSize.x(), aWinSize.y(), Quantity_NOC_BLACK);
#elif defined(__APPLE__)
    Handle(Cocoa_Window) aWindow = new Cocoa_Window(aWinName.ToCString(), 64, 64, aWinSize.x(), aWinSize.y());
#else
    Handle(Xw_Window) aWindow = new Xw_Window(aDisp, aWinName.ToCString(), 64, 64, aWinSize.x(), aWinSize.y());
#endif
    aWindow->SetVirtual(true);
    m_view          = new V3d_View(myViewer);
    m_ctx           = new AIS_InteractiveContext(myViewer);
    m_headless_view = true;
    return;
  }

  Handle(OpenGl_GraphicDriver) aGraphicDriver = new OpenGl_GraphicDriver(m_occt_window->GetDisplay(), false);

  aGraphicDriver->ChangeOptions().buffersNoSwap      = true;
  aGraphicDriver->ChangeOptions().buffersOpaqueAlpha = true;
  aGraphicDriver->ChangeOptions().sRGBDisable        = true; // Otherwise colors are wrong when native.

  Handle(V3d_Viewer) aViewer = new V3d_Viewer(aGraphicDriver);

  aViewer->SetDefaultLights();
  aViewer->SetLightOn();
  aViewer->SetDefaultTypeOfView(V3d_PERSPECTIVE);
  m_view = aViewer->CreateView();
  m_view->SetWindow(m_occt_window, m_occt_window->NativeGlContext());

  // Enable anti-aliasing through the viewer
  aViewer->SetDefaultComputedMode(true);
  aViewer->SetDefaultShadingModel(Graphic3d_TypeOfShadingModel_Phong);

  m_ctx = new AIS_InteractiveContext(aViewer);
#else // __EMSCRIPTEN__
  Handle(Aspect_DisplayConnection) aDisp;
  Handle(OpenGl_GraphicDriver) aDriver        = new OpenGl_GraphicDriver(aDisp, false);
  aDriver->ChangeOptions().buffersNoSwap      = true; // swap has no effect in WebGL
  aDriver->ChangeOptions().buffersOpaqueAlpha = true; // avoid unexpected blending of canvas with page background
  // Match native OpenGL path (sRGBDisable) so shading/material gamma is consistent vs desktop.
  aDriver->ChangeOptions().sRGBDisable = true;
  if (!aDriver->InitContext())
  {
    Message::DefaultMessenger()->Send(TCollection_AsciiString("Error: EGL initialization failed"), Message_Fail);
    return;
  }

  Handle(V3d_Viewer) aViewer = new V3d_Viewer(aDriver);
  aViewer->SetDefaultComputedMode(true); // Enable better quality rendering
  aViewer->SetDefaultShadingModel(Graphic3d_TypeOfShadingModel_Phong);
  aViewer->SetDefaultLights();
  aViewer->SetLightOn();
  for (NCollection_List<Handle(Graphic3d_CLight)>::Iterator aLightIter(aViewer->ActiveLights()); aLightIter.More();
       aLightIter.Next())
  {
    const Handle(Graphic3d_CLight)& aLight = aLightIter.Value();
    if (aLight->Type() == Graphic3d_TypeOfLightSource_Directional)
      aLight->SetCastShadows(true);
  }

  // GLFW (GLFW_SCALE_TO_MONITOR) owns canvas backing-store scaling on wasm; do not resize here.
  Handle(Wasm_Window) aWindow = new Wasm_Window("#canvas", false);

  m_view = new V3d_View(aViewer);
  m_view->Camera()->SetProjectionType(Graphic3d_Camera::Projection_Perspective);

  m_view->SetWindow(aWindow);
  m_ctx = new AIS_InteractiveContext(aViewer);

  // Browser/WASM has no system font directories; OCCT still needs font files for the view cube,
  // dimensions, etc. (CSS "serif" / OS fonts are not exposed as paths to WASM.)
  // Preload matches CMake --preload-file ... DroidSans.ttf@/DroidSans.ttf (shared with ImGui).
  {
    Handle(Font_FontMgr) font_mgr    = Font_FontMgr::GetInstance();
    Handle(Font_SystemFont) sys_font = font_mgr->CheckFont("/DroidSans.ttf");
    if (!sys_font.IsNull())
    {
      font_mgr->RegisterFont(sys_font, true);
      const TCollection_AsciiString& family = sys_font->FontName();
      // OCCT asks for generic families (Font_NOF_*); default aliases point at missing system fonts.
      for (const char* alias : {Font_NOF_SERIF, Font_NOF_SANS_SERIF, Font_NOF_MONOSPACE})
      {
        const TCollection_AsciiString alias_str(alias);
        (void)font_mgr->RemoveFontAlias(alias_str, TCollection_AsciiString());
        (void)font_mgr->AddFontAlias(alias_str, family);
      }
    }
  }
#endif

  m_view->SetImmediateUpdate(false);
  auto& params                 = m_view->ChangeRenderingParams();
  params.ToShowStats           = true;
  params.ShadowMapResolution   = 1024;
  params.OitDepthFactor        = 0.0;
  params.Resolution            = (unsigned int)(96.0 * myDevicePixelRatio + 0.5);
  params.NbMsaaSamples         = 8;
  params.RenderResolutionScale = 2.0;
  params.IsShadowEnabled       = true;
  params.TransparencyMethod    = Graphic3d_RTM_BLEND_UNORDERED;

  capture_occt_grid_rect_from_viewer_(aViewer);

  aViewer->SetDefaultComputedMode(true);
  aViewer->SetDefaultShadingModel(Graphic3d_TypeOfShadingModel_Phong);
  aViewer->SetDefaultLights();
  aViewer->SetLightOn();

  m_ctx->SetAutoActivateSelection(true); // Enable automatic selection

  auto highlight_style                           = m_ctx->HighlightStyle();
  Handle(Graphic3d_AspectFillArea3d) fill_aspect = new Graphic3d_AspectFillArea3d();
  fill_aspect->SetAlphaMode(Graphic3d_AlphaMode::Graphic3d_AlphaMode_Blend);
  fill_aspect->SetColor(Quantity_Color(0.1, 0.1, 0.1, Quantity_TOC_RGB));
  highlight_style->SetColor(Quantity_NOC_YELLOW);
  highlight_style->SetBasicFillAreaAspect(fill_aspect);
  highlight_style->SetTransparency(0.8f); // Seems to not do anything

  m_ctx->SetHighlightStyle(highlight_style);
  m_ctx->SetPixelTolerance(10); // Picking?

  m_ctx->UpdateCurrentViewer();

  // Set initial colors to match bundled defaults (dark gradient + grid)
  m_bg_color1          = glm::vec3(0.037552f, 0.040503f, 0.042471f);
  m_bg_color2          = glm::vec3(0.043440f, 0.174068f, 0.239382f);
  m_bg_gradient_method = 1; // Vertical
  m_grid_color1        = glm::vec3(0.112683f, 0.056886f, 0.138996f);
  m_grid_color2        = glm::vec3(0.117917f, 0.117917f, 0.135135f);
  update_view_background_();

  Handle(AIS_ViewCube) myViewCube = new AIS_ViewCube();
  myViewCube->SetTransformPersistence(
      new Graphic3d_TransformPers(Graphic3d_TMF_TriedronPers, Aspect_TOTP_RIGHT_LOWER, NCollection_Vec2<int>(100, 100)));

  myViewCube->Attributes()->SetDatumAspect(new Prs3d_DatumAspect());

  // Animation parameters
  myViewCube->SetViewAnimation(myViewAnimation);
  myViewCube->SetFixedAnimationLoop(false);
  myViewCube->SetAutoStartAnimation(true);
  m_ctx->Display(myViewCube, false);

  m_default_material = Graphic3d_MaterialAspect(Graphic3d_NOM_CHROME);
}

void Occt_view::roll_view_z_deg(double degrees)
{
  if (m_view.IsNull())
    return;

  m_view->Turn(V3d_Z, to_radians(degrees), true);
  m_view->Redraw();
}

void Occt_view::orbit_view_screen_step_deg(double yaw_deg, double pitch_deg)
{
  if (is_headless() || m_view.IsNull())
    return;

  const Graphic3d_Camera_ptr cam = m_view->Camera();
  if (cam.IsNull())
    return;

  const double yaw_rad   = to_radians(yaw_deg);
  const double pitch_rad = to_radians(pitch_deg);
  if (std::abs(yaw_rad) <= Precision::Angular() && std::abs(pitch_rad) <= Precision::Angular())
    return;

  const gp_Pnt pivot(cam->Center());
  const gp_Dir aCamDir(cam->Direction().Reversed());
  const gp_Dir aCamUp(cam->Up());
  const gp_Dir aCamSide(aCamUp.Crossed(aCamDir));

  gp_Trsf aTrsf;
  if (std::abs(yaw_rad) > Precision::Angular())
  {
    gp_Trsf yawTrsf;
    yawTrsf.SetRotation(gp_Ax1(pivot, aCamUp), yaw_rad);
    aTrsf.Multiply(yawTrsf);
  }

  if (std::abs(pitch_rad) > Precision::Angular())
  {
    gp_Trsf pitchTrsf;
    pitchTrsf.SetRotation(gp_Ax1(pivot, aCamSide), pitch_rad);
    aTrsf.Multiply(pitchTrsf);
  }

  cam->Transform(aTrsf);
  cam->OrthogonalizeUp();
  m_view->Invalidate();
  m_view->Redraw();
}

void Occt_view::snap_view_to_nearest_standard_axis()
{
  if (is_headless() || m_view.IsNull())
    return;

  gp_Pnt eye;
  gp_Pnt center;
  gp_Dir up_unused;
  if (!get_camera(eye, center, up_unused))
    return;

  gp_Vec       to_center(eye, center);
  const double dist = to_center.Magnitude();
  if (dist <= Precision::Confusion())
    return;

  gp_Dir fwd(to_center);

  static const gp_Dir k_axes[6] = {
      gp_Dir(1, 0, 0), gp_Dir(-1, 0, 0), gp_Dir(0, 1, 0), gp_Dir(0, -1, 0), gp_Dir(0, 0, 1), gp_Dir(0, 0, -1),
  };

  int    best_i   = 0;
  double best_dot = -2.0;
  for (int i = 0; i < 6; ++i)
  {
    const double d = fwd.X() * k_axes[i].X() + fwd.Y() * k_axes[i].Y() + fwd.Z() * k_axes[i].Z();
    if (d > best_dot)
    {
      best_dot = d;
      best_i   = i;
    }
  }

  const gp_Dir f = k_axes[best_i];

  gp_Dir new_up;
  {
    const double ax = std::abs(f.X());
    const double ay = std::abs(f.Y());
    const double az = std::abs(f.Z());
    if (az >= ax && az >= ay)
      new_up = gp_Dir(0, 1, 0);
    else
      new_up = gp_Dir(0, 0, 1);
  }

  gp_Vec offset(f);
  offset.Multiply(-dist);
  const gp_Pnt new_eye = center.Translated(offset);
  set_camera(new_eye, center, new_up);
}

void Occt_view::init_default()
{
  create_default_sketch_();

  if (m_ctx.IsNull())
    return;

  m_view->TriedronDisplay(Aspect_TOTP_LEFT_LOWER, Quantity_NOC_GOLD,
                          // 0.08,
                          0.01 / 5,
                          // 0.5,
                          V3d_WIREFRAME);

  TCollection_AsciiString aGlInfo;
  {
    NCollection_IndexedDataMap<TCollection_AsciiString, TCollection_AsciiString> aRendInfo;
    m_view->DiagnosticInformation(aRendInfo, Graphic3d_DiagnosticInfo_Basic);
    for (int i = 1; i <= aRendInfo.Extent(); ++i)
    {
      const TCollection_AsciiString& aKey   = aRendInfo.FindKey(i);
      const TCollection_AsciiString& aValue = aRendInfo.FindFromIndex(i);
      if (!aGlInfo.IsEmpty())
        aGlInfo += "\n";

      aGlInfo += TCollection_AsciiString("  ") + aKey + ": " + aValue;
    }
  }

  Message::DefaultMessenger()->Send(TCollection_AsciiString("OpenGL info:\n") + aGlInfo, Message_Info);

  SetStickToRayOnRotation(true);
  SetStickToRayOnZoom(true);
  SetRotationMode(AIS_RotationMode_PickLast);

  m_view->SetProj(0, 0, 1); // Look up the Z-axis (top view)
  m_view->SetUp(0, 1, 0);   // Up direction along Y-axis
  m_view->SetZoom(2.0);

  apply_grid_visibility_();
}

// Geometry related
ScreenCoords Occt_view::get_screen_coords(const gp_Pnt& point)
{
  int screen_x;
  int screen_y;
  m_view->Convert(point.X(), point.Y(), point.Z(), screen_x, screen_y);

  return ScreenCoords(dvec2(screen_x, screen_y));
}

std::optional<gp_Pnt> Occt_view::pt3d_on_plane(const ScreenCoords& screen_coords, const gp_Pln& plane) const
{
  // TODO there must be a way to do this using the MVP matrixes.
  // Convert 2D screen coordinates to 3D world coordinates near the camera
  double x_near, y_near, z_near;
  m_view->Convert((int)screen_coords.unsafe_get_x(), (int)screen_coords.unsafe_get_y(), x_near, y_near, z_near);
  gp_Pnt near_point(x_near, y_near, z_near);

  const Graphic3d_Camera_ptr& camera = m_view->Camera();

  auto get_intersection = [&](Geom_Line_ptr& ray_line) -> std::optional<gp_Pnt>
  {
    // Intersect the ray with the plane
    Geom_Plane_ptr geom_plane = new Geom_Plane(plane);
    GeomAPI_IntCS  intersection(ray_line, geom_plane);

    if (intersection.NbPoints() > 0)
      return intersection.Point(1); // First intersection point

    // No intersection with plane.
    return std::nullopt;
  };

  switch (camera->ProjectionType())
  {
  case Graphic3d_Camera::Projection::Projection_Orthographic:
  {
    gp_Dir view_dir = camera->Direction().Reversed(); // Reverse to point into the scene

    // Define the ray as a line from near_point along view_dir
    Geom_Line_ptr ray_line = new Geom_Line(near_point, view_dir);
    return get_intersection(ray_line);
  }

  case Graphic3d_Camera::Projection::Projection_Perspective:
  {
    gp_Pnt eye_pos = m_view->Camera()->Eye();
    // Define the ray direction from eye through the near point
    gp_Vec ray_dir(eye_pos, near_point);
    ray_dir.Normalize();

    Geom_Line_ptr ray_line = new Geom_Line(eye_pos, gp_Dir(ray_dir));
    return get_intersection(ray_line);
  }

  default:
    EZY_ASSERT(false);
  }

  // Should not get here.
  return std::nullopt;
}

void Occt_view::bake_transform_into_geometry(AIS_Shape_ptr& shape)
{
  // Function to bake the local transformation into the geometry
  // Get the current local transformation
  gp_Trsf current_transform = shape->LocalTransformation();

  // Get the current TopoDS_Shape
  TopoDS_Shape original_shape = shape->Shape();

  // Apply the transformation to the geometry
  BRepBuilderAPI_Transform transformer(original_shape, current_transform, true);

  TopoDS_Shape transformed_shape = transformer.Shape();

  // Update the AIS_Shape with the new geometry
  shape->Set(transformed_shape);

  // Reset the local transformation to identity
  gp_Trsf identity_transform;
  shape->SetLocalTransformation(identity_transform);

  // Redisplay to update the viewer and selection
  m_ctx->Redisplay(shape, true);
}

gp_Pln Occt_view::get_view_plane(const gp_Pnt& point_on_plane) const
{
  // Get the view vector from the camera
  Graphic3d_Camera_ptr camera = m_view->Camera();
  return gp_Pln(point_on_plane, camera->Direction());
}

void Occt_view::on_enter(const ScreenCoords& screen_coords)
{
  switch (get_mode())
  {
  case Mode::Sketch_face_extrude:
    // Apply typed distance (dist edit) and refresh preview only. LMB finalizes the extrusion (see on_mouse_button / GUI).
    sketch_face_extrude(screen_coords, true);
    break;

  default:
    curr_sketch().on_enter();
    break;
  }
}

void Occt_view::cancel(Set_parent_mode set_parent_mode)
{
  bool operation_canceled = false;

  switch (get_mode())
  {
  case Mode::Move:
    shp_move().cancel();
    gui().set_mode(Mode::Normal);
    break;

  case Mode::Rotate:
    shp_rotate().cancel();
    gui().set_mode(Mode::Normal);
    break;

  case Mode::Scale:
    shp_scale().cancel();
    gui().set_mode(Mode::Normal);
    break;

  default:
    operation_canceled |= cancel_sketch_extrude_();
    operation_canceled |= curr_sketch().cancel_elm();
    break;
  }

  if (set_parent_mode == Set_parent_mode::Yes)
    if (!operation_canceled)
      gui().set_parent_mode();
}

// Revolve related
void Occt_view::revolve_selected(const double angle)
{
  push_undo_snapshot();
  Shp_rslt revolved = curr_sketch().revolve_selected(angle);
  if (revolved.has_value())
  {
    add_shp_(*revolved);
    // Switch to none mode because displaying shapes in sketch modes is disabled.
    gui().set_mode(Mode::Normal);
  }
  else
  {
    gui().show_message("Revolve failed, ensure edges or faces on one side of operation axis.");
    DBG_MSG(revolved.message());
  }
}

// Sketch related
void Occt_view::create_sketch_from_planar_face_(const ScreenCoords& screen_coords)
{
  if (auto face = get_face_(screen_coords); face)
  {
    if (auto pln = plane_from_face(*face); pln)
    {
      // Push only when we will create a sketch (avoids no-op undo on misclick).
      push_undo_snapshot();
      // Get the outer wire of the face
      TopoDS_Wire outer_wire = BRepTools::OuterWire(*face);
      EZY_ASSERT(!outer_wire.IsNull());
      m_cur_sketch = std::make_shared<Sketch>("Sketch from face", *this, *pln, outer_wire);
      m_sketches.push_back(m_cur_sketch);
      m_cur_sketch->set_current();
      refresh_viewer_grid_();
      // fit_face_in_view(*face);
      m_gui.set_mode(Mode::Sketch_inspection_mode);
    }
    else
    {
      gui().show_message("Error: Selected face is not planar. Please select a planar face.");
    }
  }
}

void Occt_view::finalize_sketch_extrude_()
{
  push_undo_snapshot();
  m_shp_extrude.finalize();
}

bool Occt_view::cancel_sketch_extrude_() { return m_shp_extrude.cancel(); }

void Occt_view::create_default_sketch_()
{
  if (!m_sketches.empty() || m_cur_sketch)
    return;

  m_cur_sketch = std::make_shared<Sketch>("Sketch", *this, xy_plane());
  m_sketches.push_back(m_cur_sketch);
  m_cur_sketch->set_current();
}

void Occt_view::ensure_current_sketch_()
{
  if (m_cur_sketch)
  {
    for (const Sketch_ptr& s : m_sketches)
      if (s == m_cur_sketch)
        return;

    m_cur_sketch.reset();
  }

  if (!m_sketches.empty())
  {
    m_cur_sketch = m_sketches.front();
    m_cur_sketch->set_current();
    return;
  }

  create_default_sketch_();
}

void Occt_view::add_sketch(const gp_Pln& pln, const std::string& base_name)
{
  push_undo_snapshot();
  std::vector<std::string> existing;
  existing.reserve(m_sketches.size());
  for (const Sketch_ptr& s : m_sketches)
    existing.push_back(s->get_name());

  const std::string name = unique_sequential_name(base_name, existing);
  m_cur_sketch           = std::make_shared<Sketch>(name, *this, pln);
  m_sketches.push_back(m_cur_sketch);
  m_cur_sketch->set_current();
  refresh_viewer_grid_();
  m_gui.set_mode(Mode::Sketch_inspection_mode);
}

void Occt_view::add_sketch_on_ref_plane(Sketch_ref_plane plane, double offset_display, const std::string& base_name)
{
  const gp_Pln pln = sketch_reference_plane(plane, offset_display * get_dimension_scale());
  add_sketch(pln, base_name);
}

void Occt_view::curr_sketch_add_edge(double x1, double y1, double x2, double y2)
{
  curr_sketch().add_linear_edge(gp_Pnt2d(x1, y1), gp_Pnt2d(x2, y2));
}

void Occt_view::curr_sketch_rebuild_faces() { curr_sketch().rebuild_faces(); }

// Query related
AIS_Shape_ptr Occt_view::get_shape(const ScreenCoords& screen_coords)
{
  // Move the selection to the clicked point
  m_ctx->MoveTo(int(screen_coords.unsafe_get_x()), int(screen_coords.unsafe_get_y()), m_view, true);

  // Initialize the selection process
  m_ctx->SelectDetected(AIS_SelectionScheme_Replace);

  // Get the selected object
  m_ctx->InitSelected();
  if (!m_ctx->MoreSelected())
    return nullptr;

  // Get the selected interactive object
  const AIS_InteractiveObject_ptr& selected_object = m_ctx->SelectedInteractive();
  if (selected_object.IsNull())
    return nullptr;

  // Check if the selected object is an AIS_Shape, otherwise null is returned.
  return AIS_Shape_ptr::DownCast(selected_object);
}

std::optional<gp_Pnt2d> Occt_view::pt_on_plane(const ScreenCoords& screen_coords, const gp_Pln& plane) const
{
  if (is_headless())
    // In headless mode (unit tests), we bypass 3D projection and directly use screen coordinates
    // This allows testing 2D plane operations without requiring OpenGL context
    return gp_Pnt2d(screen_coords.unsafe_get_x(), screen_coords.unsafe_get_y());

  if (std::optional<gp_Pnt> pt = pt3d_on_plane(screen_coords, plane); pt)
    return to_2d(plane, *pt);

  // No plane intersection.
  return std::nullopt;
}

bool Occt_view::sketch_plane_view_aabb_2d(const gp_Pln& pln, double display_w, double display_h, double& out_min_u,
                                          double& out_min_v, double& out_max_u, double& out_max_v) const
{
  if (is_headless() || display_w < 4.0 || display_h < 4.0)
    return false;

  const double margin = 1.0;
  const double x0     = margin;
  const double y0     = margin;
  const double x1     = display_w - 1.0 - margin;
  const double y1     = display_h - 1.0 - margin;

  struct
  {
    double x, y;
  } corners[4] = {{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}};

  bool       any   = false;
  double     min_u = 0., min_v = 0., max_u = 0., max_v = 0.;
  const auto consider = [&](const std::optional<gp_Pnt2d>& p2)
  {
    if (!p2)
      return;

    const double u = p2->X();
    const double v = p2->Y();
    if (!any)
    {
      min_u = max_u = u;
      min_v = max_v = v;
      any           = true;
    }
    else
    {
      min_u = std::min(min_u, u);
      max_u = std::max(max_u, u);
      min_v = std::min(min_v, v);
      max_v = std::max(max_v, v);
    }
  };

  for (const auto& c : corners)
    consider(pt_on_plane(ScreenCoords(dvec2(c.x, c.y)), pln));

  if (!any)
    return false;

  constexpr double k_eps = 1e-9;
  if (max_u - min_u < k_eps)
  {
    min_u -= 1.0;
    max_u += 1.0;
  }
  if (max_v - min_v < k_eps)
  {
    min_v -= 1.0;
    max_v += 1.0;
  }

  out_min_u = min_u;
  out_min_v = min_v;
  out_max_u = max_u;
  out_max_v = max_v;
  return true;
}

bool Occt_view::get_camera(gp_Pnt& out_eye, gp_Pnt& out_center, gp_Dir& out_up) const
{
  if (is_headless() || m_view.IsNull())
    return false;

  const Graphic3d_Camera_ptr camera = m_view->Camera();
  if (camera.IsNull())
    return false;

  out_eye    = camera->Eye();
  out_center = camera->Center();
  out_up     = camera->Up();
  return true;
}

void Occt_view::set_camera(const gp_Pnt& eye, const gp_Pnt& center, const gp_Dir& up)
{
  if (is_headless() || m_view.IsNull())
    return;

  const Graphic3d_Camera_ptr camera = m_view->Camera();
  if (camera.IsNull())
    return;

  camera->SetEyeAndCenter(eye, center);
  camera->SetUp(up);
  m_view->SetCamera(camera);
  m_view->Invalidate();
  m_view->Redraw();
}

const TopoDS_Shape* Occt_view::get_(const ScreenCoords& screen_coords) const
{
  AIS_StatusOfDetection detection_status =
      m_ctx->MoveTo(int(screen_coords.unsafe_get_x()), int(screen_coords.unsafe_get_y()), m_view, true);

  if (detection_status == AIS_SOD_Nothing)
    return nullptr;

  auto detected_owner = m_ctx->DetectedOwner();

  // Cast to StdSelect_BRepOwner
  auto brep_owner = StdSelect_BRepOwner_ptr::DownCast(detected_owner);

  // Get the shape from the BRep owner
  const TopoDS_Shape& shp = brep_owner->Shape();
  if (shp.IsNull())
    return nullptr;

  return &shp;
}

const TopoDS_Face* Occt_view::get_face_(const ScreenCoords& screen_coords) const
{
  const TopoDS_Shape* shp = get_(screen_coords);

  if (!shp || shp->ShapeType() != TopAbs_FACE)
    return nullptr;

  return &TopoDS::Face(*shp);
}

const TopoDS_Wire* Occt_view::get_wire_(const ScreenCoords& screen_coords) const
{
  const TopoDS_Shape* shp = get_(screen_coords);

  if (!shp || shp->ShapeType() != TopAbs_WIRE)
    return nullptr;

  return &TopoDS::Wire(*shp);
}

const TopoDS_Edge* Occt_view::get_edge_(const ScreenCoords& screen_coords) const
{
  const TopoDS_Shape* shp = get_(screen_coords);

  if (!shp || shp->ShapeType() != TopAbs_EDGE)
    return nullptr;

  return &TopoDS::Edge(*shp);
}

// Hit point related
Ray Occt_view::get_hit_test_ray_(const ScreenCoords& screen_coords) const
{
  Graphic3d_Camera_ptr camera  = m_view->Camera();
  gp_Pnt               eye_pos = camera->Eye();
  double               x_near, y_near, z_near;
  m_view->Convert((int)screen_coords.unsafe_get_x(), (int)screen_coords.unsafe_get_y(), x_near, y_near, z_near);
  gp_Pnt near_point(x_near, y_near, z_near);
  gp_Vec ray_dir(eye_pos, near_point);
  ray_dir.Normalize();
  return Ray(eye_pos, gp_Dir(ray_dir));
}

std::optional<gp_Pnt> Occt_view::get_hit_point_(const AIS_Shape_ptr& shp, const ScreenCoords& screen_coords) const
{
  Ray           hit_ray  = get_hit_test_ray_(screen_coords);
  Geom_Line_ptr ray_line = new Geom_Line(hit_ray.origin, hit_ray.direction);

  TopExp_Explorer explorer(shp->Shape(), TopAbs_FACE);
  while (explorer.More())
  {
    TopoDS_Face face             = TopoDS::Face(explorer.Current());
    Handle(Geom_Surface) surface = BRep_Tool::Surface(face);
    GeomAPI_IntCS intersection(ray_line, surface);
    if (intersection.NbPoints() > 0)
      return intersection.Point(1);

    explorer.Next();
  }

  // No hit points found.
  return std::nullopt;
}

void Occt_view::refresh_shape_shading_(const Shp_ptr& shp)
{
  if (shp.IsNull())
    return;

#ifdef __EMSCRIPTEN__
  // OCCT 8 GLES: Phong needs explicit color; UNLIT fallback was removed in 8.0.
  shp->SetColor(Quantity_Color(0.78, 0.78, 0.80, Quantity_TOC_RGB));

  const Handle(Prs3d_Drawer)& drawer = shp->Attributes();
  if (!drawer.IsNull())
  {
    drawer->SetFaceBoundaryDraw(false);
    const Handle(Prs3d_ShadingAspect)& shading = drawer->ShadingAspect();
    if (!shading.IsNull())
    {
      const Handle(Graphic3d_AspectFillArea3d)& aspect = shading->Aspect();
      if (!aspect.IsNull())
      {
#if OCC_VERSION_HEX >= 0x080000
        aspect->SetUseVertexColorForBackFaces(false);
#endif
      }
    }
  }
#endif
}

void Occt_view::add_shp_(Shp_ptr& shp)
{
  shp->SetMaterial(m_default_material);
  refresh_shape_shading_(shp);
  shp->set_selection_mode(m_shp_selection_mode);
  m_ctx->Redisplay(shp, true);
  m_ctx->UpdateCurrentViewer();
  m_shps.push_back(shp);
}

std::string Occt_view::unique_shape_name_(const char* base_name) const
{
  std::vector<std::string> existing;
  existing.reserve(m_shps.size());
  for (const Shp_ptr& s : m_shps)
    existing.push_back(s->get_name());

  return unique_sequential_name(base_name, existing);
}

std::string Occt_view::get_unique_shape_name(const char* base_name) const { return unique_shape_name_(base_name); }

void Occt_view::add_box(double ox, double oy, double oz, double width, double length, double height)
{
  push_undo_snapshot();
  TopoDS_Shape box = shp_create::create_box(ox, oy, oz, width, length, height);
  Shp_ptr      shp = new Shp(*m_ctx, box);
  shp->set_name(unique_shape_name_("Box"));
  add_shp_(shp);
  m_ctx->Display(shp, shp->get_disp_mode(), AIS_Shape::SelectionMode(m_shp_selection_mode), true);
  m_view->Redraw();
}

void Occt_view::add_pyramid(double ox, double oy, double oz, double side)
{
  push_undo_snapshot();
  TopoDS_Shape pyramid = shp_create::create_pyramid(side);
  if (pyramid.IsNull())
    return;

  Shp_ptr shp = new Shp(*m_ctx, pyramid);
  shp->set_name(unique_shape_name_("Pyramid"));
  if (ox != 0 || oy != 0 || oz != 0)
  {
    gp_Trsf trsf;
    trsf.SetTranslation(gp_Vec(ox, oy, oz));
    shp->SetLocalTransformation(trsf);
  }

  add_shp_(shp);
  m_ctx->Display(shp, shp->get_disp_mode(), AIS_Shape::SelectionMode(m_shp_selection_mode), true);
  m_view->Redraw();
}

void Occt_view::add_sphere(double ox, double oy, double oz, double radius)
{
  push_undo_snapshot();
  TopoDS_Shape sphere = shp_create::create_sphere(radius);
  Shp_ptr      shp    = new Shp(*m_ctx, sphere);
  shp->set_name(unique_shape_name_("Sphere"));
  if (ox != 0 || oy != 0 || oz != 0)
  {
    gp_Trsf trsf;
    trsf.SetTranslation(gp_Vec(ox, oy, oz));
    shp->SetLocalTransformation(trsf);
  }

  add_shp_(shp);
  m_ctx->Display(shp, shp->get_disp_mode(), AIS_Shape::SelectionMode(m_shp_selection_mode), true);
  m_view->Redraw();
}

void Occt_view::add_cylinder(double ox, double oy, double oz, double radius, double height)
{
  push_undo_snapshot();
  TopoDS_Shape shape = shp_create::create_cylinder(radius, height);
  Shp_ptr      shp   = new Shp(*m_ctx, shape);
  shp->set_name(unique_shape_name_("Cylinder"));
  if (ox != 0 || oy != 0 || oz != 0)
  {
    gp_Trsf trsf;
    trsf.SetTranslation(gp_Vec(ox, oy, oz));
    shp->SetLocalTransformation(trsf);
  }

  add_shp_(shp);
  m_ctx->Display(shp, shp->get_disp_mode(), AIS_Shape::SelectionMode(m_shp_selection_mode), true);
  m_view->Redraw();
}

void Occt_view::add_cone(double ox, double oy, double oz, double R1, double R2, double height)
{
  push_undo_snapshot();
  TopoDS_Shape shape = shp_create::create_cone(R1, R2, height);
  Shp_ptr      shp   = new Shp(*m_ctx, shape);
  shp->set_name(unique_shape_name_("Cone"));
  if (ox != 0 || oy != 0 || oz != 0)
  {
    gp_Trsf trsf;
    trsf.SetTranslation(gp_Vec(ox, oy, oz));
    shp->SetLocalTransformation(trsf);
  }

  add_shp_(shp);
  m_ctx->Display(shp, shp->get_disp_mode(), AIS_Shape::SelectionMode(m_shp_selection_mode), true);
  m_view->Redraw();
}

void Occt_view::add_torus(double ox, double oy, double oz, double R1, double R2)
{
  push_undo_snapshot();
  TopoDS_Shape shape = shp_create::create_torus(R1, R2);
  Shp_ptr      shp   = new Shp(*m_ctx, shape);
  shp->set_name(unique_shape_name_("Torus"));
  if (ox != 0 || oy != 0 || oz != 0)
  {
    gp_Trsf trsf;
    trsf.SetTranslation(gp_Vec(ox, oy, oz));
    shp->SetLocalTransformation(trsf);
  }

  add_shp_(shp);
  m_ctx->Display(shp, shp->get_disp_mode(), AIS_Shape::SelectionMode(m_shp_selection_mode), true);
  m_view->Redraw();
}

bool Occt_view::fit_face_in_view(const TopoDS_Face& face)
{
  EZY_ASSERT(!face.IsNull());
  auto pln = plane_from_face(face);
  if (!pln)
    return false;

  // Set camera direction to look down the normal
  gp_Dir normal = pln->Axis().Direction();
  gp_Pnt center = pln->Location(); // Use plane origin as initial center

  // Compute the bounding box of the face to refine the center and size
  Bnd_Box bbox;
  BRepBndLib::Add(face, bbox);
  if (bbox.IsVoid())
  {
    std::cout << "Error: Bounding box is void" << std::endl;
    return false;
  }

  // Get the center of the bounding box
  double x_min, y_min, z_min, x_max, y_max, z_max;
  bbox.Get(x_min, y_min, z_min, x_max, y_max, z_max);
  center.SetCoord((x_min + x_max) / 2.0, (y_min + y_max) / 2.0, (z_min + z_max) / 2.0);

  // Set the view direction (looking "down" means opposite to normal)
  gp_Dir view_dir = normal.Reversed(); // Look down at the face
  m_view->SetAt(center.X(), center.Y(), center.Z());
  gp_Pnt eye = center.Translated(gp_Vec(view_dir).Multiplied(100.0)); // Target point (center of face)
  m_view->SetEye(eye.X(), eye.Y(), eye.Z());                          // Eye 100 units above
  m_view->SetUp(0, 0, 1);
  // Fit the face in view
  m_view->FitAll(bbox, 0.5); // 0.5 is margin factor (50% padding)
  m_view->Redraw();          // Update the view
  return true;
}

// Dimension related
void Occt_view::refresh_sketch_annotations(const Sketch_annotation_refresh& refresh)
{
  for (const Sketch_ptr& sk : m_sketches)
    if (sk)
      sk->refresh_annotations(refresh);

  if (refresh.length_dimensions)
    m_shp_extrude.refresh_tmp_dimension_style(m_gui.length_dimension_style());
}

void Occt_view::apply_sketch_dimensions_visibility()
{
  if (!m_gui.show_sketch_dimensions())
  {
    for (const Sketch_ptr& s : m_sketches)
      if (s)
        s->set_show_dims(false);

    return;
  }

  if (is_sketch_mode(get_mode()))
  {
    if (get_mode() == Mode::Sketch_operation_axis)
    {
      for (const Sketch_ptr& s : m_sketches)
        if (s)
          s->set_show_dims(s == m_cur_sketch);
    }
    else
    {
      for (const Sketch_ptr& s : m_sketches)
        if (s)
          s->set_show_dims(true);
    }
  }
  else
  {
    const bool show = get_mode() == Mode::Shape_polar_duplicate;
    for (const Sketch_ptr& s : m_sketches)
      if (s)
        s->set_show_dims(show);
  }
}

void Occt_view::dimension_input(const ScreenCoords& screen_coords)
{
  switch (get_mode())
  {
  case Mode::Sketch_face_extrude:
    m_show_dim_input = true;
    sketch_face_extrude(screen_coords, true);
    break;

  default:
    curr_sketch().dimension_input(screen_coords);
    break;
  }
}

void Occt_view::angle_input(const ScreenCoords& screen_coords) { curr_sketch().angle_input(screen_coords); }

double Occt_view::get_dimension_scale() const { return m_dimension_scale; }

bool Occt_view::get_show_dim_input() const { return m_show_dim_input; }

void Occt_view::set_show_dim_input(bool show) { m_show_dim_input = show; }

std::optional<double> Occt_view::get_entered_dim() const { return m_entered_dim; }

void Occt_view::set_entered_dim(const std::optional<double>& dim) { m_entered_dim = dim; }

void Occt_view::sketch_face_extrude(const ScreenCoords& screen_coords, bool is_mouse_move)
{
  m_shp_extrude.sketch_face_extrude(screen_coords, is_mouse_move);
}

void Occt_view::delete_selected() { delete_shapes(get_selected()); }

void Occt_view::delete_shapes(std::vector<AIS_Shape_ptr> to_delete)
{
  if (to_delete.empty())
    return;

  std::erase_if(to_delete, [](const AIS_Shape_ptr& shp) { return is_sketch_origin_node_mark(shp.get()); });
  if (to_delete.empty())
    return;

  push_undo_snapshot();
  remove_selected_length_dimensions_from_sketches_();
  delete_(to_delete);
  cancel(Set_parent_mode::No); // In case we are in the middle of a operation.
}

void Occt_view::remove_selected_length_dimensions_from_sketches_()
{
  std::vector<PrsDim_LengthDimension_ptr> selected_dims;
  m_ctx->InitSelected();
  while (m_ctx->MoreSelected())
  {
    const AIS_InteractiveObject_ptr obj = m_ctx->SelectedInteractive();
    if (!obj.IsNull())
    {
      const PrsDim_LengthDimension_ptr dim = PrsDim_LengthDimension_ptr::DownCast(obj);
      if (!dim.IsNull())
        selected_dims.push_back(dim);
    }

    m_ctx->NextSelected();
  }
  for (const PrsDim_LengthDimension_ptr& dim : selected_dims)
    for (Sketch_ptr& s : m_sketches)
      if (s->try_remove_length_dimension(dim.get()))
        break;
}

void Occt_view::delete_(std::vector<AIS_Shape_ptr>& to_delete)
{
  for (AIS_Shape_ptr& shp : to_delete)
    try_remove_sketch_permanent_node_mark(shp.get());

  for (AIS_Shape_ptr& shp : to_delete)
    if (auto wire = dynamic_cast<Sketch_AIS_edge*>(shp.get()); wire)
      wire->owner_sketch.remove_edge(*wire);

  for (auto itr = m_shps.begin(); itr != m_shps.end();)
    if (std::find(to_delete.begin(), to_delete.end(), *itr) != to_delete.end())
      itr = m_shps.erase(itr);
    else
      ++itr;

  for (const AIS_Shape_ptr& shp : to_delete)
    if (m_shape_list_hover == shp)
    {
      set_shape_list_hover(nullptr);
      break;
    }

  remove(to_delete);
}

namespace
{

void set_grid_colors_on_viewer_(const V3d_Viewer_ptr& viewer, const glm::vec3& color1, const glm::vec3& color2)
{
  if (viewer.IsNull() || viewer->Grid().IsNull())
    return;

  Quantity_Color cc(color1.x, color1.y, color1.z, Quantity_TOC_RGB);
  Quantity_Color cd(color2.x, color2.y, color2.z, Quantity_TOC_RGB);
  viewer->Grid()->SetColors(cc, cd);
}

} // namespace

Occt_view::Grid_layout Occt_view::compute_grid_layout_() const
{
  Grid_layout  layout{};
  const gp_Pln ref_pln = m_cur_sketch ? m_cur_sketch->get_plane() : gp_Pln(grid_display_plane_());
  const gp_Ax3 base_ax = ref_pln.Position();

  const double step = m_occt_grid_rect.step > 0.0 ? m_occt_grid_rect.step : 10.0;
  const double pad  = m_occt_grid_rect.grid_padding >= 0.0 ? m_occt_grid_rect.grid_padding : 0.0;

  auto expand_pt = [](double& min_u, double& min_v, double& max_u, double& max_v, bool& any, const gp_Pnt2d& pt)
  {
    const double u = pt.X();
    const double v = pt.Y();
    if (!any)
    {
      min_u = max_u = u;
      min_v = max_v = v;
      any           = true;
    }
    else
    {
      min_u = std::min(min_u, u);
      max_u = std::max(max_u, u);
      min_v = std::min(min_v, v);
      max_v = std::max(max_v, v);
    }
  };

  bool   has_bounds = false;
  double min_u = 0., min_v = 0., max_u = 0., max_v = 0.;

  if (m_cur_sketch)
  {
    for (const Sketch_nodes::Node& node : m_cur_sketch->get_nodes())
    {
      if (node.deleted)
        continue;

      expand_pt(min_u, min_v, max_u, max_v, has_bounds, node);
    }
  }

  if (!has_bounds && !is_headless())
  {
    const ImGuiIO& io = ImGui::GetIO();
    if (sketch_plane_view_aabb_2d(ref_pln, static_cast<double>(io.DisplaySize.x), static_cast<double>(io.DisplaySize.y), min_u,
                                  min_v, max_u, max_v))
      has_bounds = true;
  }

  if (!has_bounds)
  {
    const double half = std::max(step * 5.0, 1.0);
    min_u             = -half;
    max_u             = half;
    min_v             = -half;
    max_v             = half;
  }

  min_u -= pad;
  max_u += pad;
  min_v -= pad;
  max_v += pad;

  // Snap outer bounds to grid lines so edges align with major/minor spacing.
  min_u = std::floor(min_u / step) * step;
  max_u = std::ceil(max_u / step) * step;
  min_v = std::floor(min_v / step) * step;
  max_v = std::ceil(max_v / step) * step;

  constexpr double k_min_span = 1e-6;
  if (max_u - min_u < k_min_span)
  {
    min_u -= step;
    max_u += step;
  }
  if (max_v - min_v < k_min_span)
  {
    min_v -= step;
    max_v += step;
  }

  const double center_u = (min_u + max_u) * 0.5;
  const double center_v = (min_v + max_v) * 0.5;
  layout.size_x         = max_u - min_u;
  layout.size_y         = max_v - min_v;

  const gp_Pnt center_3d = to_3d(ref_pln, gp_Pnt2d(center_u, center_v));
  layout.plane           = gp_Ax3(center_3d, base_ax.Direction(), base_ax.XDirection());
  return layout;
}

static Aspect_GradientFillMethod gradient_method_from_int(int i)
{
  static const Aspect_GradientFillMethod methods[] = {Aspect_GFM_HOR,     Aspect_GFM_VER,     Aspect_GFM_DIAG1,
                                                      Aspect_GFM_DIAG2,   Aspect_GFM_CORNER1, Aspect_GFM_CORNER2,
                                                      Aspect_GFM_CORNER3, Aspect_GFM_CORNER4};
  const int                              n         = static_cast<int>(sizeof(methods) / sizeof(methods[0]));
  if (i < 0 || i >= n)
    return Aspect_GFM_VER;

  return methods[i];
}

void Occt_view::update_view_background_()
{
  if (m_view.IsNull())
    return;

  m_view->SetBgGradientColors(Quantity_Color(m_bg_color1.x, m_bg_color1.y, m_bg_color1.z, Quantity_TOC_RGB),
                              Quantity_Color(m_bg_color2.x, m_bg_color2.y, m_bg_color2.z, Quantity_TOC_RGB),
                              gradient_method_from_int(m_bg_gradient_method), true);
}

void Occt_view::get_bg_gradient_colors(float color1[3], float color2[3]) const
{
  for (int i = 0; i < 3; ++i)
  {
    color1[i] = m_bg_color1[i];
    color2[i] = m_bg_color2[i];
  }
}

void Occt_view::set_bg_gradient_colors(float r1, float g1, float b1, float r2, float g2, float b2)
{
  m_bg_color1 = glm::vec3(r1, g1, b1);
  m_bg_color2 = glm::vec3(r2, g2, b2);
  update_view_background_();
}

int Occt_view::get_bg_gradient_method() const { return m_bg_gradient_method; }

void Occt_view::set_bg_gradient_method(int method)
{
  m_bg_gradient_method = method;
  update_view_background_();
}

void Occt_view::get_grid_colors(float color1[3], float color2[3]) const
{
  for (int i = 0; i < 3; ++i)
  {
    color1[i] = m_grid_color1[i];
    color2[i] = m_grid_color2[i];
  }
}

void Occt_view::set_grid_colors(float r1, float g1, float b1, float r2, float g2, float b2)
{
  m_grid_color1 = glm::vec3(r1, g1, b1);
  m_grid_color2 = glm::vec3(r2, g2, b2);
  update_view_background_();
  refresh_viewer_grid_();
}

Occt_grid_rect_params Occt_view::clamp_occt_grid_rect_params_(Occt_grid_rect_params g)
{
  constexpr double min_padding     = 0.0;
  constexpr double min_step        = 1e-6;
  constexpr double default_step    = 10.0;
  constexpr double default_padding = 1000.0;

  if (!std::isfinite(g.grid_padding) || g.grid_padding < 0.0)
    g.grid_padding = default_padding;
  if (!std::isfinite(g.graphic_z_offset))
    g.graphic_z_offset = 0.0;

  g.grid_padding = std::max(g.grid_padding, min_padding);

  if (!std::isfinite(g.step) || g.step <= 0.0)
    g.step = default_step;

  g.step = std::max(g.step, min_step);

  return g;
}

void Occt_view::capture_occt_grid_rect_from_viewer_(const V3d_Viewer_ptr& viewer)
{
  if (viewer.IsNull() || viewer->Grid().IsNull())
    return;

  Handle(Aspect_Grid) ag            = viewer->Grid();
  Handle(Aspect_RectangularGrid) rg = Handle(Aspect_RectangularGrid)::DownCast(ag);
  Handle(V3d_RectangularGrid) vrg   = Handle(V3d_RectangularGrid)::DownCast(ag);
  if (rg.IsNull())
    return;

  m_occt_grid_rect.step = rg->XStep();

  // OCCT 7.x wasm: Z offset comes from V3d_RectangularGrid::GraphicValues, not Aspect_RectangularGrid::ZOffset().
  if (!vrg.IsNull())
  {
    double gx{}, gy{}, gz{};
    vrg->GraphicValues(gx, gy, gz);
    (void)gx;
    (void)gy;
    m_occt_grid_rect.graphic_z_offset = gz;
  }
}

gp_Ax3 Occt_view::grid_display_plane_() const
{
  if (m_cur_sketch)
    return m_cur_sketch->get_plane().Position();

  if (!m_view.IsNull())
  {
    Handle(V3d_Viewer) viewer = m_view->Viewer();
    if (!viewer.IsNull())
      return viewer->PrivilegedPlane();
  }

  return gp_Ax3(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(0.0, 0.0, 1.0));
}

void Occt_view::refresh_active_sketch_grid() { refresh_viewer_grid_(); }

bool Occt_view::get_grid_visible() const { return m_grid_visible; }

void Occt_view::set_grid_visible(const bool visible)
{
  m_grid_visible = visible;
  apply_grid_visibility_();
}

void Occt_view::sync_grid_plane_to_active_sketch_()
{
  if (is_headless() || m_ctx.IsNull() || !m_cur_sketch)
    return;

  Handle(V3d_Viewer) viewer = m_ctx->CurrentViewer();
  if (viewer.IsNull())
    return;

  viewer->SetPrivilegedPlane(m_cur_sketch->get_plane().Position());
}

void Occt_view::refresh_viewer_grid_()
{
  if (is_headless() || m_view.IsNull())
    return;

  if (!m_grid_visible)
    return;

  Handle(V3d_Viewer) viewer = m_view->Viewer();
  if (viewer.IsNull() || !viewer->IsGridActive())
    return;

  apply_occt_grid_rect_to_viewer_();
  set_grid_colors_on_viewer_(viewer, m_grid_color1, m_grid_color2);
  m_view->Invalidate();
  m_view->Redraw();
}

void Occt_view::apply_grid_visibility_()
{
  if (is_headless() || m_view.IsNull())
    return;

  Handle(V3d_Viewer) viewer = m_view->Viewer();
  if (viewer.IsNull())
    return;

  if (m_grid_visible)
  {
    if (!viewer->IsGridActive())
    {
      viewer->ActivateGrid(Aspect_GT_Rectangular, Aspect_GDM_Lines);
      viewer->SetGridEcho(false);
    }

    apply_occt_grid_rect_to_viewer_();
    set_grid_colors_on_viewer_(viewer, m_grid_color1, m_grid_color2);
  }
  else if (viewer->IsGridActive())
    viewer->DeactivateGrid();

  m_view->Invalidate();
  m_view->Redraw();
}

void Occt_view::apply_occt_grid_rect_to_viewer_()
{
  if (is_headless() || m_view.IsNull() || !m_grid_visible)
    return;

  m_occt_grid_rect = clamp_occt_grid_rect_params_(m_occt_grid_rect);

  Handle(V3d_Viewer) viewer = m_view->Viewer();
  if (viewer.IsNull() || viewer->Grid().IsNull())
    return;

  const Grid_layout layout = compute_grid_layout_();
  viewer->SetPrivilegedPlane(layout.plane);

  Handle(Aspect_Grid) ag            = viewer->Grid();
  Handle(Aspect_RectangularGrid) rg = Handle(Aspect_RectangularGrid)::DownCast(ag);
  Handle(V3d_RectangularGrid) vrg   = Handle(V3d_RectangularGrid)::DownCast(ag);
  if (rg.IsNull())
    return;

  rg->SetGridValues(0., 0., m_occt_grid_rect.step, m_occt_grid_rect.step, 0.);

  if (!vrg.IsNull())
  {
    const double step   = m_occt_grid_rect.step > 0.0 ? m_occt_grid_rect.step : 10.0;
    const double half_x = layout.size_x * 0.5;
    const double half_y = layout.size_y * 0.5;
    const double z_off  = m_occt_grid_rect.graphic_z_offset - step / 50.0;
    vrg->SetGraphicValues(half_x, half_y, z_off);
  }

  set_grid_colors_on_viewer_(viewer, m_grid_color1, m_grid_color2);
  m_view->Invalidate();
}

void Occt_view::get_occt_grid_rect_params(Occt_grid_rect_params& out) const { out = m_occt_grid_rect; }

void Occt_view::set_occt_grid_rect_params(const Occt_grid_rect_params& p)
{
  m_occt_grid_rect = clamp_occt_grid_rect_params_(p);
  apply_occt_grid_rect_to_viewer_();
}

void Occt_view::flush_view_events()
{
  if (!m_view.IsNull())
    FlushViewEvents(m_ctx, m_view, true);
}

void Occt_view::do_frame()
{
  flush_view_events();
  if (!m_view.IsNull())
    m_view->Redraw();
}

void Occt_view::cleanup()
{
  if (!m_view.IsNull())
    m_view->Remove();

  if (!m_occt_window.IsNull())
    m_occt_window->Close();

  glfwTerminate();
}

void Occt_view::on_resize(int theWidth, int theHeight)
{
  if (theWidth != 0 && theHeight != 0 && !m_view.IsNull())
  {
    m_view->Window()->DoResize();
    m_view->MustBeResized();
    m_view->Invalidate();
    m_view->Redraw();
  }
}

namespace
{
// Blender-style: Shift held while zooming uses a finer step (same idea as precision transforms).
constexpr double k_zoom_shift_finer_factor = 0.1;
} // namespace

void Occt_view::set_zoom_scroll_scale(double scale)
{
  m_zoom_scroll_scale = std::clamp(scale, k_gui_view_zoom_scroll_scale_min, k_gui_view_zoom_scroll_scale_max);
}

int Occt_view::zoom_scroll_delta_int_(double wheel_y, bool shift_finer_zoom) const
{
  const double scaled = wheel_y * m_zoom_scroll_scale * (shift_finer_zoom ? k_zoom_shift_finer_factor : 1.0);
  long         r      = std::lround(scaled);
  if (r == 0 && wheel_y != 0.0)
    r = wheel_y > 0.0 ? 1L : -1L;

  return static_cast<int>(r);
}

void Occt_view::on_mouse_scroll(double theOffsetX, double theOffsetY, bool shift_finer_zoom)
{
  (void)theOffsetX;
  if (!m_view.IsNull())
    UpdateZoom(Aspect_ScrollDelta(m_occt_window->CursorPosition(), zoom_scroll_delta_int_(theOffsetY, shift_finer_zoom)));
}

void Occt_view::zoom_view_wheel_notches(double wheel_notches, bool shift_finer_zoom)
{
  if (m_view.IsNull())
    return;

  UpdateZoom(Aspect_ScrollDelta(m_occt_window->CursorPosition(), zoom_scroll_delta_int_(wheel_notches, shift_finer_zoom)));
}

void Occt_view::on_mouse_button(int theButton, int theAction, int theMods)
{
  if (m_view.IsNull())
    return;

  const NCollection_Vec2<int> pos = m_occt_window->CursorPosition();
  if (theAction == GLFW_PRESS)
  {
    // Planar-face picking uses InteractiveContext::MoveTo() in get_face_(). Run it before
    // AIS_ViewController::PressMouseButton(), which can alter detection state and block face hits on solids.
    if (get_mode() == Mode::Sketch_from_planar_face && theButton == GLFW_MOUSE_BUTTON_LEFT)
    {
      flush_view_events();
      create_sketch_from_planar_face_(ScreenCoords(dvec2(pos.x(), pos.y())));
      m_planar_face_lmb_skipped_view_controller = true;
      return;
    }

    PressMouseButton(pos, mouse_button_from_glfw_(theButton), key_flags_from_glfw_(theMods), false);

    if (m_shp_extrude.has_active_extrusion())
    {
      finalize_sketch_extrude_();
      return;
    }

    switch (get_mode())
    {
    case Mode::Sketch_dim_anno:
      return curr_sketch().toggle_edge_dim_anno(ScreenCoords(dvec2(pos.x(), pos.y())));

    default:
      break;
    }
  }
  else
  {
    if (m_planar_face_lmb_skipped_view_controller && theButton == GLFW_MOUSE_BUTTON_LEFT)
    {
      m_planar_face_lmb_skipped_view_controller = false;
      return;
    }

    ReleaseMouseButton(pos, mouse_button_from_glfw_(theButton), key_flags_from_glfw_(theMods), false);
  }
}

void Occt_view::on_mouse_move(const ScreenCoords& screen_coords)
{
  EZY_ASSERT(!m_view.IsNull());
  UpdateMousePosition(NCollection_Vec2<int>(int(screen_coords.unsafe_get_x()), int(screen_coords.unsafe_get_y())),
                      PressedMouseButtons(), key_flags_from_glfw_window_(), false);
}

// Selection related
std::vector<AIS_Shape_ptr> Occt_view::get_selected() const
{
  // Initialize selection iteration
  m_ctx->InitSelected();
  // Iterate over selected objects
  std::vector<AIS_Shape_ptr> shapes;
  while (m_ctx->MoreSelected())
  {
    AIS_InteractiveObject_ptr selected_obj = m_ctx->SelectedInteractive();
    if (!selected_obj.IsNull())
      if (selected_obj->IsKind(STANDARD_TYPE(AIS_Shape)))
      {
        auto selected_shape = AIS_Shape_ptr::DownCast(selected_obj);
        shapes.emplace_back(std::move(selected_shape));
      }

    m_ctx->NextSelected(); // Move to the next selected object
  }

  return shapes;
}

void Occt_view::update_shape_list_hover_drawer_()
{
  uint8_t r{}, g{}, b{}, a{};
  gui().elm_list_hover_color_rgba(r, g, b, a);
  (void)a;
  const Quantity_Color qc(static_cast<double>(r) / 255.0, static_cast<double>(g) / 255.0, static_cast<double>(b) / 255.0,
                          Quantity_TOC_RGB);

  if (m_shape_list_hover_drawer.IsNull())
    m_shape_list_hover_drawer = new Prs3d_Drawer();

  m_shape_list_hover_drawer->SetColor(qc);
  Handle(Graphic3d_AspectFillArea3d) fill_aspect = new Graphic3d_AspectFillArea3d();
  fill_aspect->SetAlphaMode(Graphic3d_AlphaMode_Blend);
  fill_aspect->SetColor(Quantity_Color(0.1, 0.1, 0.1, Quantity_TOC_RGB));
  m_shape_list_hover_drawer->SetBasicFillAreaAspect(fill_aspect);
  Handle(Prs3d_LineAspect) wire_aspect = new Prs3d_LineAspect(qc, Aspect_TOL_SOLID, 2.0);
  m_shape_list_hover_drawer->SetWireAspect(wire_aspect);
}

void Occt_view::clear_sketch_list_hover_ais_()
{
  if (is_headless() || m_ctx.IsNull())
    return;

  for (const AIS_InteractiveObject_ptr& ais : m_sketch_list_hover_ais)
    if (!ais.IsNull())
      m_ctx->Unhilight(ais, false);

  m_sketch_list_hover_ais.clear();
}

void Occt_view::apply_sketch_list_hover_highlight_()
{
  if (is_headless() || m_ctx.IsNull() || !m_sketch_list_hover || !m_sketch_list_hover->is_visible())
    return;

  update_shape_list_hover_drawer_();
  m_sketch_list_hover->append_list_hover_ais(m_sketch_list_hover_ais);

  for (const AIS_InteractiveObject_ptr& ais : m_sketch_list_hover_ais)
    if (!ais.IsNull())
      m_ctx->HilightWithColor(ais, m_shape_list_hover_drawer, false);
}

void Occt_view::restore_sketch_list_measurement_hover_style_()
{
  if (m_sketch_list_measurement_hover.IsNull())
    return;

  apply_length_dimension_style(m_sketch_list_measurement_hover, gui().length_dimension_style());
  m_ctx->Redisplay(m_sketch_list_measurement_hover, false);
}

void Occt_view::apply_sketch_list_measurement_hover_style_()
{
  if (m_sketch_list_measurement_hover.IsNull())
    return;

  uint8_t r{}, g{}, b{}, a{};
  gui().elm_list_hover_color_rgba(r, g, b, a);
  (void)a;
  const float hover_rgb[3] = {static_cast<float>(r) / 255.f, static_cast<float>(g) / 255.f, static_cast<float>(b) / 255.f};

  const Length_dimension_style& style   = gui().length_dimension_style();
  const double                  base_w  = static_cast<double>(style.line_width);
  const double                  hover_w = std::max(3.0, base_w * 3.0);

  apply_length_dimension_list_hover_style(m_sketch_list_measurement_hover, hover_rgb, hover_w);
  m_ctx->Redisplay(m_sketch_list_measurement_hover, false);
}

void Occt_view::refresh_sketch_list_measurement_hover_highlight_() { apply_sketch_list_measurement_hover_style_(); }

void Occt_view::refresh_shape_list_hover_highlight()
{
  if (is_headless() || m_ctx.IsNull())
    return;

  update_shape_list_hover_drawer_();

  if (!m_shape_list_hover.IsNull())
  {
    m_ctx->Unhilight(m_shape_list_hover, false);
    if (m_shape_list_hover->get_visible())
      m_ctx->HilightWithColor(m_shape_list_hover, m_shape_list_hover_drawer, false);
  }

  if (!m_sketch_list_measurement_hover.IsNull())
    apply_sketch_list_measurement_hover_style_();

  if (m_sketch_list_hover)
  {
    clear_sketch_list_hover_ais_();
    apply_sketch_list_hover_highlight_();
  }

  m_ctx->UpdateCurrentViewer();
}

void Occt_view::set_sketch_list_measurement_hover(const Sketch_ptr& sketch, const size_t dim_index)
{
  if (is_headless() || m_ctx.IsNull())
    return;

  PrsDim_LengthDimension_ptr dim;
  if (sketch && dim_index != SIZE_MAX && dim_index < sketch->length_dimension_count())
  {
    dim = sketch->length_dimension_handle(dim_index);
    if (dim.IsNull() || !sketch->is_visible() || !sketch->shows_dimensions() || !sketch->dimension_visible(dim_index))
      dim.Nullify();
  }

  if (m_sketch_list_measurement_hover == dim)
    return;

  if (!m_sketch_list_measurement_hover.IsNull())
    restore_sketch_list_measurement_hover_style_();

  m_sketch_list_measurement_hover = dim;

  if (!m_sketch_list_measurement_hover.IsNull())
    apply_sketch_list_measurement_hover_style_();

  m_ctx->UpdateCurrentViewer();
}

void Occt_view::set_sketch_list_hover(const Sketch_ptr& sketch)
{
  if (is_headless() || m_ctx.IsNull())
    return;

  if (m_sketch_list_hover == sketch)
    return;

  clear_sketch_list_hover_ais_();
  m_sketch_list_hover = sketch;

  if (m_sketch_list_hover && m_sketch_list_hover->is_visible())
    apply_sketch_list_hover_highlight_();
  else if (m_sketch_list_hover)
    m_sketch_list_hover.reset();

  m_ctx->UpdateCurrentViewer();
}

void Occt_view::set_shape_list_hover(const Shp_ptr& shp)
{
  if (is_headless() || m_ctx.IsNull())
    return;

  if (m_shape_list_hover == shp)
    return;

  if (!m_shape_list_hover.IsNull())
  {
    m_ctx->Unhilight(m_shape_list_hover, false);
    m_shape_list_hover.Nullify();
  }

  m_shape_list_hover = shp;

  if (!m_shape_list_hover.IsNull() && m_shape_list_hover->get_visible())
  {
    update_shape_list_hover_drawer_();
    m_ctx->HilightWithColor(m_shape_list_hover, m_shape_list_hover_drawer, false);
  }

  m_ctx->UpdateCurrentViewer();
}

TopAbs_ShapeEnum Occt_view::get_shp_selection_mode() const { return m_shp_selection_mode; }

void Occt_view::set_shp_selection_mode(const TopAbs_ShapeEnum selection_mode)
{
  m_modes_selection_mode_map[get_mode()] = selection_mode;

  if (m_shp_selection_mode == selection_mode)
    return;

  m_shp_selection_mode  = selection_mode;
  const std::size_t idx = static_cast<std::size_t>(selection_mode);
  EZY_ASSERT(idx < c_names_TopAbs_ShapeEnum.size());
  m_gui.log_message(std::string("Selection mode: ") + std::string(c_names_TopAbs_ShapeEnum[idx]));

  for (auto& shp : m_shps)
    shp->set_selection_mode(selection_mode);
}

// Material related
const Graphic3d_MaterialAspect& Occt_view::get_default_material() const { return m_default_material; }

void Occt_view::set_default_material(const Graphic3d_MaterialAspect& mat) { m_default_material = mat; }

void Occt_view::rotate_view(const gp_Vec& axis, const gp_Pnt& center)
{
  if (m_view)
    m_view->Rotate(axis.X(), axis.Y(), axis.Z(), center.X(), center.Y(), center.Z());
}

void Occt_view::redraw_view()
{
  if (m_view)
    m_view->Redraw();
}

V3d_View_ptr Occt_view::view_handle() const { return m_view; }

bool Occt_view::is_headless() const { return m_headless_view; }

// Mode related
Mode Occt_view::get_mode() const { return m_gui.get_mode(); }

bool Occt_view::sketch_snap_suppressed() const
{
  return m_cur_sketch && get_mode() == Mode::Sketch_operation_axis && m_cur_sketch->has_operation_axis();
}

void Occt_view::apply_camera_projection()
{
  if (is_headless())
    return;

  const bool ortho = is_sketch_mode(get_mode()) || m_gui.inspection_orthographic();

  Graphic3d_Camera_ptr camera = m_view->Camera();
  if (ortho)
    camera->SetProjectionType(Graphic3d_Camera::Projection_Orthographic);
  else
    camera->SetProjectionType(Graphic3d_Camera::Projection_Perspective);

  m_view->Redraw();
  m_ctx->UpdateCurrentViewer();
}

void Occt_view::on_mode()
{
  DBG_MSG(c_mode_strs[int(get_mode())]);

  shp_polar_dup().reset();

  for (Sketch_ptr& s : m_sketches)
    s->on_mode();

  auto show_only_current_sketch = [&]()
  {
    for (Sketch_ptr& s : m_sketches)
    {
      s->set_show_faces(s == m_cur_sketch);
      s->set_show_edges(s == m_cur_sketch);
    }
  };

  auto show_sketches = [&](const bool show)
  {
    for (Sketch_ptr& s : m_sketches)
    {
      s->set_show_edges(show);
      s->set_show_faces(show);
    }
  };

  if (is_sketch_mode(get_mode()))
  {
    for (auto shp : m_shps)
      shp->set_visible(false);

    switch (get_mode())
    {
    case Mode::Sketch_operation_axis:
      show_only_current_sketch();
      break;

    default:
      show_sketches(true);
      break;
    }
  }
  else
  {
    switch (get_mode())
    {
    case Mode::Shape_polar_duplicate:
      show_only_current_sketch();
      break;

    default:
      show_sketches(false);
      break;
    }

    switch (get_mode())
    {
      // clang-format off
      case Mode::Sketch_from_planar_face: set_shp_selection_mode(TopAbs_FACE);      break;
      case Mode::Shape_chamfer:           on_chamfer_mode();                        break; // Will update selection mode
      case Mode::Shape_fillet:            on_fillet_mode();                         break; // Will update selection mode
      case Mode::Move:                    set_shp_selection_mode(TopAbs_COMPOUND);  break;
      case Mode::Rotate:                  set_shp_selection_mode(TopAbs_COMPOUND);  break;
      case Mode::Scale:                   set_shp_selection_mode(TopAbs_COMPOUND);  break;
      default:
        if(m_modes_selection_mode_map.count(get_mode()))
          set_shp_selection_mode(m_modes_selection_mode_map.at(get_mode()));
        
        break;
      // clang-format on
    }

    for (auto shp : m_shps)
      shp->set_visible(true);
  }

  apply_sketch_dimensions_visibility();
  apply_camera_projection();
}

void Occt_view::on_chamfer_mode()
{
  EZY_ASSERT(get_mode() == Mode::Shape_chamfer);
  switch (gui().get_chamfer_mode())
  {
    // clang-format off
    case Chamfer_mode::Edge:  set_shp_selection_mode(TopAbs_EDGE);  break;
    case Chamfer_mode::Wire:  set_shp_selection_mode(TopAbs_WIRE);  break;
    case Chamfer_mode::Face:  set_shp_selection_mode(TopAbs_FACE);  break;
    case Chamfer_mode::Shape: set_shp_selection_mode(TopAbs_SHAPE); break;
     // clang-format off
      default:
        EZY_ASSERT(false);
  }
}

void Occt_view::on_fillet_mode()
{
  EZY_ASSERT(get_mode() == Mode::Shape_fillet);
  switch (gui().get_fillet_mode())
  {
      // clang-format off
    case Fillet_mode::Edge:  set_shp_selection_mode(TopAbs_EDGE);  break;
    case Fillet_mode::Wire:  set_shp_selection_mode(TopAbs_WIRE);  break;
    case Fillet_mode::Face:  set_shp_selection_mode(TopAbs_FACE);  break;
    case Fillet_mode::Shape: set_shp_selection_mode(TopAbs_SHAPE); break;
     // clang-format off
      default:
        EZY_ASSERT(false);
  }
}

Aspect_VKeyMouse Occt_view::mouse_button_from_glfw_(int theButton)
{
  // clang-format off
  switch (theButton)
  {
    case GLFW_MOUSE_BUTTON_LEFT:   return Aspect_VKeyMouse_LeftButton;
    case GLFW_MOUSE_BUTTON_RIGHT:  return Aspect_VKeyMouse_RightButton;
    case GLFW_MOUSE_BUTTON_MIDDLE: return Aspect_VKeyMouse_MiddleButton;
  }
  // clang-format on
  return Aspect_VKeyMouse_NONE;
}

Aspect_VKeyFlags Occt_view::key_flags_from_glfw_(int theFlags)
{
  // clang-format off
  Aspect_VKeyFlags flags = Aspect_VKeyFlags_NONE;
  if ((theFlags & GLFW_MOD_SHIFT)   != 0) flags |= Aspect_VKeyFlags_SHIFT;
  if ((theFlags & GLFW_MOD_CONTROL) != 0) flags |= Aspect_VKeyFlags_CTRL;
  if ((theFlags & GLFW_MOD_ALT)     != 0) flags |= Aspect_VKeyFlags_ALT;
  if ((theFlags & GLFW_MOD_SUPER)   != 0) flags |= Aspect_VKeyFlags_META;
  // clang-format on
  return flags;
}

Aspect_VKeyFlags Occt_view::key_flags_from_glfw_window_() const
{
  if (m_occt_window.IsNull() || m_occt_window->getGlfwWindow() == nullptr)
    return Aspect_VKeyFlags_NONE;

  GLFWwindow* const window = m_occt_window->getGlfwWindow();
  int               mods   = 0;
  if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
    mods |= GLFW_MOD_SHIFT;

  if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)
    mods |= GLFW_MOD_CONTROL;

  if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS)
    mods |= GLFW_MOD_ALT;

  if (glfwGetKey(window, GLFW_KEY_LEFT_SUPER) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SUPER) == GLFW_PRESS)
    mods |= GLFW_MOD_SUPER;

  return key_flags_from_glfw_(mods);
}

Occt_view::Sketch_list& Occt_view::get_sketches() { return m_sketches; }

size_t Occt_view::allocate_sketch_id() { return m_next_sketch_id++; }

void Occt_view::adopt_sketch_id(const size_t id)
{
  if (id >= m_next_sketch_id)
    m_next_sketch_id = id + 1;
}

void Occt_view::remove_sketch(const Sketch_ptr& sketch)
{
  if (m_sketch_list_hover == sketch)
    set_sketch_list_hover(nullptr);

  push_undo_snapshot();
  m_sketches.remove(sketch);
  if (m_cur_sketch == sketch)
  {
    if (m_sketches.empty())
    {
      m_cur_sketch = nullptr;
      create_default_sketch_();
    }
    else
      m_cur_sketch = m_sketches.front();
  }
}

Sketch& Occt_view::curr_sketch()
{
  ensure_current_sketch_();
  return *m_cur_sketch;
}

Sketch* Occt_view::current_sketch_if_any() const { return m_cur_sketch.get(); }

Occt_view::Sketch_ptr Occt_view::curr_sketch_shared() const
{
  if (!m_cur_sketch)
    const_cast<Occt_view*>(this)->ensure_current_sketch_();

  return m_cur_sketch;
}

void Occt_view::set_curr_sketch(const Sketch_ptr& to_set)
{
  for (Sketch_ptr& sketch : m_sketches)
    if (sketch.get() == to_set.get())
    {
      m_cur_sketch = sketch;
      m_cur_sketch->set_current();
      refresh_viewer_grid_();

      // If hide all shapes is enabled, hide all shapes except the current sketch
      if (m_gui.get_hide_all_shapes())
        // Hide all shapes
        for (const Shp_ptr& shape : m_shps)
          shape->set_visible(false);

      return;
    }

  EZY_ASSERT(false); // Sketch does not belong to this view.
}

std::list<Shp_ptr>& Occt_view::get_shapes() { return m_shps; }

// clang-format off
Shp_move&      Occt_view::shp_move()      { return m_shp_move;       }
Shp_rotate&    Occt_view::shp_rotate()    { return m_shp_rotate;     }
Shp_scale&     Occt_view::shp_scale()     { return m_shp_scale;      }
Shp_chamfer&   Occt_view::shp_chamfer()   { return m_shp_chamfer;    }
Shp_fillet&    Occt_view::shp_fillet()    { return m_shp_fillet;     }
Shp_cut&       Occt_view::shp_cut()       { return m_shp_cut;        }
Shp_fuse&      Occt_view::shp_fuse()      { return m_shp_fuse;       }
Shp_common&    Occt_view::shp_common()    { return m_shp_common;     }
Shp_polar_dup& Occt_view::shp_polar_dup() { return m_shp_polar_dup;  }
Shp_extrude&   Occt_view::shp_extrude()   { return m_shp_extrude;    }
// clang-format on

// ---------------------------------------------------------------------------
// Undo / redo: sketch edits use element deltas; other operations use full JSON snapshots.
void Occt_view::push_undo_snapshot()
{
  if (m_restoring)
    return;

  m_redo_stack.clear();
  Undo_entry entry;
  entry.json = to_json();
  entry.mode = m_gui.get_mode();
  m_undo_stack.push_back(std::move(entry));
  if (m_undo_stack.size() > k_max_undo)
    m_undo_stack.erase(m_undo_stack.begin());
}

void Occt_view::push_undo_delta(std::unique_ptr<Delta> delta)
{
  if (m_restoring || !delta)
    return;

  m_redo_stack.clear();
  Undo_entry entry;
  entry.delta = std::move(delta);
  entry.mode  = m_gui.get_mode();
  m_undo_stack.push_back(std::move(entry));
  if (m_undo_stack.size() > k_max_undo)
    m_undo_stack.erase(m_undo_stack.begin());
}

void Occt_view::pop_undo_snapshot()
{
  if (m_restoring || m_undo_stack.empty())
    return;

  m_undo_stack.pop_back();
}

bool Occt_view::undo()
{
  if (!can_undo())
    return false;

  m_restoring      = true;
  Undo_entry state = std::move(m_undo_stack.back());
  m_undo_stack.pop_back();

  Undo_entry redo_entry;
  redo_entry.mode = m_gui.get_mode();

  if (state.delta)
  {
    redo_entry.delta = state.delta->clone();
    state.delta->apply_reverse(*this);
  }
  else
  {
    redo_entry.json = to_json();
    load(state.json, false); // Keep current view so undo/redo keeps a single perspective
  }

  m_redo_stack.push_back(std::move(redo_entry));
  m_gui.set_mode(state.mode);
  if (state.mode == Mode::Sketch_inspection_mode)
    m_gui.set_show_sketch_list(true);

  m_restoring = false;
  return true;
}

bool Occt_view::redo()
{
  if (!can_redo())
    return false;

  m_restoring      = true;
  Undo_entry state = std::move(m_redo_stack.back());
  m_redo_stack.pop_back();

  Undo_entry undo_entry;
  undo_entry.mode = m_gui.get_mode();

  if (state.delta)
  {
    undo_entry.delta = state.delta->clone();
    state.delta->apply_forward(*this);
  }
  else
  {
    undo_entry.json = to_json();
    load(state.json, false); // Keep current view so undo/redo keeps a single perspective
  }

  m_undo_stack.push_back(std::move(undo_entry));
  m_gui.set_mode(state.mode);
  if (state.mode == Mode::Sketch_inspection_mode)
    m_gui.set_show_sketch_list(true);

  m_restoring = false;
  return true;
}

bool Occt_view::can_undo() const { return !m_undo_stack.empty(); }
bool Occt_view::can_redo() const { return !m_redo_stack.empty(); }

size_t Occt_view::undo_stack_size() const { return m_undo_stack.size(); }
size_t Occt_view::redo_stack_size() const { return m_redo_stack.size(); }

// ---------------------------------------------------------------------------
// Document format: 1 = legacy sketch edges could carry a 4th "dim" flag; 2 = length_dimensions array + 3-tuple edges.
namespace
{
constexpr int k_ezy_file_format_version = 3;
}

// ---------------------------------------------------------------------------
std::string Occt_view::to_json() const
{
  using namespace nlohmann;
  json j;
  j["ezyFormat"] = k_ezy_file_format_version;
  json& sketches = j["sketches"] = json::array();
  json& shps = j["shapes"] = json::array();

  const auto pnt_to_json = [](double x, double y, double z)
  {
    return ::to_json(gp_Pnt(x, y, z));
  };

  // ---------------------------------------------------------------------------
  // Sketches / shapes
  for (const Sketch_ptr& s : m_sketches)
    sketches.push_back(Sketch_json::to_json(*s, m_assets));

  for (const Shp_ptr& s : m_shps)
  {
    const TopoDS_Shape& shape = s->Shape();
    std::ostringstream  oss;
    BRepTools::Write(shape, oss, false, false, TopTools_FormatVersion_CURRENT); // Write BREP data to the stream
    json shp_json;
    shp_json["name"]     = s->get_name();
    shp_json["material"] = s->Material();
    shp_json["geom"]     = oss.str();
    shps.push_back(shp_json);
  }

  // ---------------------------------------------------------------------------
  // View / camera state
  if (!m_view.IsNull())
  {
    json& view_json = j["view"];

    // Eye and target (At) positions
    double eye_x, eye_y, eye_z;
    double at_x, at_y, at_z;
    m_view->Eye(eye_x, eye_y, eye_z);
    m_view->At(at_x, at_y, at_z);

    view_json["eye"] = pnt_to_json(eye_x, eye_y, eye_z);
    view_json["at"]  = pnt_to_json(at_x, at_y, at_z);

    // Up and projection directions
    double up_x, up_y, up_z;
    double proj_x, proj_y, proj_z;
    m_view->Up(up_x, up_y, up_z);
    m_view->Proj(proj_x, proj_y, proj_z);

    view_json["up"]   = ::to_json(gp_Dir(up_x, up_y, up_z));
    view_json["proj"] = ::to_json(gp_Dir(proj_x, proj_y, proj_z));

    // View scale (zoom level)
    view_json["scale"] = m_view->Scale();
  }

  return j.dump(2);
}

void Occt_view::load(const std::string& json_str, bool restore_view)
{
  using namespace nlohmann;
  for (AIS_Shape_ptr& s : m_shps)
    m_ctx->Remove(s, false);

  clear_all(m_sketches, m_cur_sketch, m_shps);

  if (!m_restoring)
  {
    m_undo_stack.clear();
    m_redo_stack.clear();
  }

  const json j = json::parse(json_str);
  (void)j.value("ezyFormat", 1); // Reserved for future migrations; sketch JSON migrates per-edge dim flags in Sketch_json.
  EZY_ASSERT(j.contains("sketches") && j["sketches"].is_array());
  for (const auto& s : j["sketches"])
  {
    m_sketches.push_back(Sketch_json::from_json(*this, s));
    if (s["isCurrent"])
    {
      // if(m_cur_sketch)
      // MSG("Multiple sketches marked as current in JSON; using the last one.");

      m_cur_sketch = m_sketches.back();
    }
  }

  ensure_current_sketch_();

  for (const json& s : j["shapes"])
  {
    TopoDS_Shape       shape;
    std::istringstream iss;
    iss.str(s["geom"]);
    BRepTools::Read(shape, iss, BRep_Builder());
    Shp_ptr shp = new Shp(*m_ctx, shape);
    shp->set_name(s["name"]);
    int mat_idx = static_cast<int>(m_default_material.Name());
    if (s.contains("material") && s["material"].is_number_integer())
      mat_idx = s["material"].get<int>();

    const int nmat = Graphic3d_MaterialAspect::NumberOfMaterials();
    if (mat_idx < 0 || mat_idx >= nmat)
      mat_idx = static_cast<int>(m_default_material.Name());

    shp->SetMaterial(Graphic3d_MaterialAspect(static_cast<Graphic3d_NameOfMaterial>(mat_idx)));
    refresh_shape_shading_(shp);
    m_shps.push_back(shp);
    m_ctx->Display(shp, shp->get_disp_mode(), 0, true);
    m_ctx->Redisplay(shp, true);
  }

  // ---------------------------------------------------------------------------
  // Restore view / camera state if requested (e.g. File > Open; not for undo/redo)
  if (restore_view && !m_view.IsNull() && j.contains("view") && j["view"].is_object())
  {
    const json& view_json = j["view"];
    try
    {
      if (view_json.contains("eye") && view_json["eye"].is_object())
      {
        gp_Pnt eye = from_json_pnt(view_json["eye"]);
        m_view->SetEye(eye.X(), eye.Y(), eye.Z());
      }

      if (view_json.contains("at") && view_json["at"].is_object())
      {
        gp_Pnt at = from_json_pnt(view_json["at"]);
        m_view->SetAt(at.X(), at.Y(), at.Z());
      }

      if (view_json.contains("up") && view_json["up"].is_object())
      {
        gp_Dir up = from_json_dir(view_json["up"]);
        m_view->SetUp(up.X(), up.Y(), up.Z());
      }

      if (view_json.contains("proj") && view_json["proj"].is_object())
      {
        gp_Dir dir = from_json_dir(view_json["proj"]);
        m_view->SetProj(dir.X(), dir.Y(), dir.Z());
      }

      if (view_json.contains("scale") && view_json["scale"].is_number())
      {
        const double scale = view_json["scale"].get<double>();
        if (scale > Precision::Confusion())
          m_view->SetScale(scale);
      }

      m_view->Redraw();
      m_ctx->UpdateCurrentViewer();
    }
    catch (const std::exception&)
    {
      // Ignore view restoration errors; project geometry has already loaded.
    }
  }

  if (m_cur_sketch)
  {
    m_cur_sketch->set_current();
    refresh_viewer_grid_();
  }
}

TopoDS_Shape Occt_view::shape_with_local_transform_(const AIS_Shape_ptr& ais) const
{
  if (ais.IsNull())
    return {};

  const TopoDS_Shape& s = ais->Shape();
  if (s.IsNull())
    return {};

  const gp_Trsf&           tr = ais->LocalTransformation();
  BRepBuilderAPI_Transform transformer(s, tr, true);
  return transformer.Shape();
}

Status Occt_view::build_export_shape_(TopoDS_Shape& out_shape) const
{
  std::vector<TopoDS_Shape>        parts;
  const std::vector<AIS_Shape_ptr> selected = get_selected();
  if (!selected.empty())
    for (const AIS_Shape_ptr& ais : selected)
    {
      TopoDS_Shape t = shape_with_local_transform_(ais);
      if (!t.IsNull())
        parts.push_back(t);
    }

  else
    for (const Shp_ptr& shp : m_shps)
    {
      TopoDS_Shape t = shape_with_local_transform_(shp);
      if (!t.IsNull())
        parts.push_back(t);
    }

  if (parts.empty())
    return Status::user_error("Nothing to export (no shapes).");

  if (parts.size() == 1)
    out_shape = parts.front();
  else
  {
    TopoDS_Compound comp;
    BRep_Builder    bb;
    bb.MakeCompound(comp);
    for (const TopoDS_Shape& p : parts)
      bb.Add(comp, p);

    out_shape = comp;
  }
  return Status::ok();
}

Status Occt_view::export_document(Export_format fmt, const std::string& file_path)
{
  TopoDS_Shape shape;
  CHK_RET(build_export_shape_(shape));

  switch (fmt)
  {
  case Export_format::Step:
  {
    STEPControl_Writer    writer;
    IFSelect_ReturnStatus tr = writer.Transfer(shape, STEPControl_AsIs);
    if (tr != IFSelect_RetDone)
      return Status::user_error("STEP transfer failed.");

    tr = writer.Write(file_path.c_str());
    if (tr != IFSelect_RetDone)
      return Status::user_error("STEP write failed.");

    return Status::ok();
  }
  case Export_format::Iges:
  {
    IGESControl_Writer writer;
    if (!writer.AddShape(shape))
      return Status::user_error("IGES does not support this shape.");

    if (!writer.Write(file_path.c_str()))
      return Status::user_error("IGES write failed.");

    return Status::ok();
  }
  case Export_format::Stl:
  {
    // Tessellate for mesh export (linear deflection in model units).
    constexpr double               k_lin_deflection = 0.1;
    const BRepMesh_IncrementalMesh mesher(shape, k_lin_deflection);
    (void)mesher;
    StlAPI_Writer stl_writer;
    stl_writer.ASCIIMode() = false;
    if (!stl_writer.Write(shape, file_path.c_str()))
      return Status::user_error("STL write failed.");

    return Status::ok();
  }
  case Export_format::Ply:
  {
    constexpr double               k_lin_deflection = 0.1;
    const BRepMesh_IncrementalMesh mesher(shape, k_lin_deflection);
    (void)mesher;
    return export_ply_binary_file(shape, file_path);
  }
  }
  return Status::user_error("Unknown export format.");
}

Status Occt_view::import_step(const std::string& step_data)
{
  STEPControl_Reader reader;
  std::istringstream stream(step_data);

  IFSelect_ReturnStatus read_st = reader.ReadStream("", stream);
  if (read_st != IFSelect_RetDone)
    return Status::user_error("STEP: could not read file (invalid or corrupt STEP data).");

  if (reader.TransferRoots() == 0)
    return Status::user_error("STEP: no geometry was transferred from the file.");

  const int                 num_shps = reader.NbShapes();
  std::vector<TopoDS_Shape> to_add;
  to_add.reserve(static_cast<size_t>(num_shps));
  for (int i = 1; i <= num_shps; ++i)
  {
    TopoDS_Shape shape = reader.Shape(i);
    if (!shape.IsNull())
      to_add.push_back(shape);
  }

  if (to_add.empty())
    return Status::user_error("STEP: no valid shapes in file.");

  push_undo_snapshot();
  for (const TopoDS_Shape& shape : to_add)
  {
    Shp_ptr shp = new Shp(*m_ctx, shape);
    add_shp_(shp);
  }

  return Status::ok();
}

bool Occt_view::import_ply(const std::string& ply_bytes)
{
  push_undo_snapshot();
  TopoDS_Shape shape;
  if (Status st = import_ply_shape(ply_bytes, shape); !st.is_ok())
  {
    m_gui.log_message("PLY import failed: " + st.message());
    return false;
  }
  if (shape.IsNull())
    return false;

  Shp_ptr shp = new Shp(*m_ctx, shape);
  add_shp_(shp);
  return true;
}

GUI& Occt_view::gui() { return m_gui; }

AIS_InteractiveContext& Occt_view::ctx() { return *m_ctx; }

void Occt_view::new_file()
{
  m_undo_stack.clear();
  m_redo_stack.clear();
  remove(m_shps);
  clear_all(m_shps, m_sketches, m_cur_sketch);
  m_assets.clear();
  m_next_sketch_id = 1;

  create_default_sketch_();
  refresh_viewer_grid_();
  m_gui.set_mode(Mode::Normal);
}