#include "occt_view.h"

#include <AIS_ViewCube.hxx>
#include <Aspect_Grid.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepTools.hxx>
#include <BRep_Tool.hxx>
#include <GeomAPI_IntCS.hxx>
#include <Geom_Line.hxx>
#include <Geom_Plane.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <Prs3d_DatumAspect.hxx>
#include <PrsDim_LengthDimension.hxx>
#include <STEPControl_Reader.hxx>
#include <StdSelect_BRepOwner.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <WNT_WClass.hxx>
#include <WNT_Window.hxx>

#include "dbg.h"
#include "geom.h"
#include "gui.h"
#include "sketch.h"
#include "sketch_json.h"
#include "utl.h"

#ifdef __EMSCRIPTEN__
#include <Wasm_Window.hxx>
#endif

#include <GLFW/glfw3.h>

#include <iostream>
#include <sstream>

Occt_view::Occt_view(GUI& gui)
    : m_gui(gui), m_shp_move(*this), m_shp_rotate(*this), m_shp_chamfer(*this), m_shp_cut(*this), m_shp_fuse(*this), m_shp_common(*this), m_shp_polar_dup(*this)
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
  double myDevicePixelRatio = 1.0;  // TODO
#ifndef __EMSCRIPTEN__
  if (m_occt_window.IsNull() || m_occt_window->getGlfwWindow() == nullptr)
  {
    // create graphic driver
    Handle(Aspect_DisplayConnection) aDisp =
        new Aspect_DisplayConnection();
    Handle(OpenGl_GraphicDriver) aDriver =
        new OpenGl_GraphicDriver(aDisp, true);
    aDriver->ChangeOptions().swapInterval = 0;  // no window, no swap
    Handle(V3d_Viewer) myViewer           = new V3d_Viewer(aDriver);

    // create offscreen window
    const TCollection_AsciiString aWinName("OCCT offscreen window");
    Graphic3d_Vec2i               aWinSize(512, 512);
#if defined(_WIN32)
    const TCollection_AsciiString aClassName("OffscreenClass");
    // empty callback!
    Handle(WNT_WClass) aWinClass =
        new WNT_WClass(aClassName.ToCString(), nullptr, 0);
    Handle(WNT_Window) aWindow =
        new WNT_Window(aWinName.ToCString(), aWinClass, 0x80000000,  // WS_POPUP
                       64, 64, aWinSize.x(), aWinSize.y(),
                       Quantity_NOC_BLACK);
#elif defined(__APPLE__)
    Handle(Cocoa_Window) aWindow =
        new Cocoa_Window(aWinName.ToCString(),
                         64, 64, aWinSize.x(), aWinSize.y());
#else
    Handle(Xw_Window) aWindow = new Xw_Window(aDisp, aWinName.ToCString(),
                                              64, 64, aWinSize.x(), aWinSize.y());
#endif
    aWindow->SetVirtual(true);
    m_view          = new V3d_View(myViewer);
    m_ctx           = new AIS_InteractiveContext(myViewer);
    m_headless_view = true;
    return;
  }

  Handle(OpenGl_GraphicDriver) aGraphicDriver =
      new OpenGl_GraphicDriver(m_occt_window->GetDisplay(), false);

  aGraphicDriver->ChangeOptions().buffersNoSwap      = true;
  aGraphicDriver->ChangeOptions().buffersOpaqueAlpha = true;
  aGraphicDriver->ChangeOptions().sRGBDisable =
      true;  // Otherwise colors are wrong when native.

  Handle(V3d_Viewer) aViewer = new V3d_Viewer(aGraphicDriver);

  aViewer->SetDefaultLights();
  aViewer->SetLightOn();
  aViewer->SetDefaultTypeOfView(V3d_PERSPECTIVE);
  m_view = aViewer->CreateView();
  m_view->SetWindow(m_occt_window, m_occt_window->NativeGlContext());

  // Enable anti-aliasing through the viewer
  aViewer->SetDefaultComputedMode(Standard_True);
  aViewer->SetDefaultShadingModel(Graphic3d_TypeOfShadingModel_Phong);

  m_ctx = new AIS_InteractiveContext(aViewer);
#else
  Handle(Aspect_DisplayConnection) aDisp;
  Handle(OpenGl_GraphicDriver) aDriver   = new OpenGl_GraphicDriver(aDisp, false);
  aDriver->ChangeOptions().buffersNoSwap = true;  // swap has no effect in WebGL
  aDriver->ChangeOptions().buffersOpaqueAlpha =
      true;  // avoid unexpected blending of canvas with page background
  if (!aDriver->InitContext())
  {
    Message::DefaultMessenger()->Send(
        TCollection_AsciiString("Error: EGL initialization failed"),
        Message_Fail);
    return;
  }

  Handle(V3d_Viewer) aViewer = new V3d_Viewer(aDriver);
  aViewer->SetDefaultComputedMode(Standard_True);  // Enable better quality rendering
  aViewer->SetDefaultShadingModel(Graphic3d_TypeOfShadingModel_Phong);
  aViewer->SetDefaultLights();
  aViewer->SetLightOn();
  for (V3d_ListOfLight::Iterator aLightIter(aViewer->ActiveLights());
       aLightIter.More();
       aLightIter.Next())
  {
    const Handle(V3d_Light)& aLight = aLightIter.Value();
    if (aLight->Type() == Graphic3d_TypeOfLightSource_Directional)
      aLight->SetCastShadows(true);
  }

  Handle(Wasm_Window) aWindow = new Wasm_Window("#canvas");

  m_view = new V3d_View(aViewer);
  m_view->Camera()->SetProjectionType(Graphic3d_Camera::Projection_Perspective);

  m_view->SetWindow(aWindow);
  m_ctx = new AIS_InteractiveContext(aViewer);
#endif

  m_view->SetImmediateUpdate(false);
  m_view->ChangeRenderingParams().ToShowStats           = true;
  m_view->ChangeRenderingParams().NbMsaaSamples         = 8;                              // Set MSAA samples
  m_view->ChangeRenderingParams().RenderResolutionScale = 2.0;                            // Increase resolution scale
  m_view->ChangeRenderingParams().IsShadowEnabled       = true;                           // Enable shadows
  m_view->ChangeRenderingParams().ShadowMapResolution   = 1024;                           // Shadow map resolution
  m_view->ChangeRenderingParams().TransparencyMethod    = Graphic3d_RTM_BLEND_UNORDERED;  // Better transparency
  m_view->ChangeRenderingParams().OitDepthFactor        = 0.0;                            // Depth peeling for better transparency
  m_view->ChangeRenderingParams().Resolution            = (unsigned int) (96.0 * myDevicePixelRatio + 0.5);

  // m_view->SetScale(1000.0); // Treat coordinates as millimeters
  // m_view->SetScale(39.3701);

  aViewer->ActivateGrid(Aspect_GT_Rectangular, Aspect_GDM_Lines);
  aViewer->SetGridEcho(false);
  // aViewer->SetGridEcho(true);
  Quantity_Color cc(0.1, 0.1, 0.1, Quantity_TypeOfColor::Quantity_TOC_RGB);
  Quantity_Color cd(0.1, 0.1, 0.3, Quantity_TypeOfColor::Quantity_TOC_RGB);
  aViewer->Grid()->SetColors(cc, cd);
  // aViewer->Grid()->SetDrawMode(Aspect_GridDrawMode::Aspect_GDM_Points);

  // aViewer->SetComputedMode(false);
  //   Enable anti-aliasing through the viewer
  aViewer->SetDefaultComputedMode(Standard_True);
  aViewer->SetDefaultShadingModel(Graphic3d_TypeOfShadingModel_Phong);
  aViewer->SetDefaultLights();
  aViewer->SetLightOn();

  m_ctx->SetAutoActivateSelection(Standard_True);  // Enable automatic selection

  auto highlight_style                           = m_ctx->HighlightStyle();
  // highlight_style->SetDisplayMode(1);
  Handle(Graphic3d_AspectFillArea3d) fill_aspect = new Graphic3d_AspectFillArea3d();
  fill_aspect->SetAlphaMode(Graphic3d_AlphaMode::Graphic3d_AlphaMode_Blend);
  fill_aspect->SetColor(Quantity_Color(0.1, 0.1, 0.1, Quantity_TOC_RGB));
  highlight_style->SetColor(Quantity_NOC_YELLOW);
  highlight_style->SetBasicFillAreaAspect(fill_aspect);
  highlight_style->SetTransparency(0.8f);  // Seems to not do anything

  /*
  Handle(Prs3d_LineAspect) line_aspect = new Prs3d_LineAspect(
      Quantity_NOC_GREEN,  // Color
      Aspect_TOL_SOLID,     // Line type
      5.0                   // Width
  );
  //highlight_style->SetLineAspect(line_aspect);
  highlight_style->SetWireAspect(line_aspect);
  */

  m_ctx->SetHighlightStyle(highlight_style);
  // m_ctx->SetAutomaticHilight(false);
  //  m_ctx->SetSelectionStyle(highlight_style);
  m_ctx->SetPixelTolerance(10);  // Picking?

  m_ctx->UpdateCurrentViewer();

  Handle(AIS_ViewCube) myViewCube = new AIS_ViewCube();
  myViewCube->SetTransformPersistence(
      new Graphic3d_TransformPers(Graphic3d_TMF_TriedronPers,
                                  Aspect_TOTP_RIGHT_LOWER,
                                  Graphic3d_Vec2i(100, 100)));
  myViewCube->Attributes()->SetDatumAspect(new Prs3d_DatumAspect());
  // myViewCube->Attributes()->DatumAspect()->SetTextAspect(myTextStyle);
  //  animation parameters
  myViewCube->SetViewAnimation(myViewAnimation);
  myViewCube->SetFixedAnimationLoop(false);
  myViewCube->SetAutoStartAnimation(true);
  m_ctx->Display(myViewCube, false);

  // opencascade::handle<Graphic3d_TextureEnv> env_map = new Graphic3d_TextureEnv("c:/src/pngwing.com.png");
  // m_view->SetTextureEnv(env_map);

  m_default_material = Graphic3d_MaterialAspect(Graphic3d_NOM_CHROME);
}

void Occt_view::init_default()
{
  create_default_sketch_();

  if (m_ctx.IsNull())
    return;

  m_view->TriedronDisplay(Aspect_TOTP_LEFT_LOWER,
                          Quantity_NOC_GOLD,
                          // 0.08,
                          0.01 / 5,
                          // 0.5,
                          V3d_WIREFRAME);

  TCollection_AsciiString aGlInfo;
  {
    TColStd_IndexedDataMapOfStringString aRendInfo;
    m_view->DiagnosticInformation(aRendInfo, Graphic3d_DiagnosticInfo_Basic);
    for (TColStd_IndexedDataMapOfStringString::Iterator aValueIter(aRendInfo);
         aValueIter.More();
         aValueIter.Next())
    {
      if (!aGlInfo.IsEmpty())
      {
        aGlInfo += "\n";
      }
      aGlInfo += TCollection_AsciiString("  ") + aValueIter.Key() + ": " +
                 aValueIter.Value();
    }
  }
  Message::DefaultMessenger()->Send(
      TCollection_AsciiString("OpenGL info:\n") + aGlInfo,
      Message_Info);

  SetStickToRayOnRotation(true);
  SetStickToRayOnZoom(true);
  SetRotationMode(AIS_RotationMode_PickLast);

  m_view->SetProj(0, 0, 1);  // Look up the Z-axis (top view)
  m_view->SetUp(0, 1, 0);    // Up direction along Y-axis

  // Optional: Adjust zoom or fit content
  m_view->SetZoom(2.0);
  //

  // load();
  //  m_view->FitAll();
}

// Geometry related
ScreenCoords Occt_view::get_screen_coords(const gp_Pnt& point)
{
  Standard_Integer screen_x;
  Standard_Integer screen_y;
  m_view->Convert(point.X(), point.Y(), point.Z(), screen_x, screen_y);
  return ScreenCoords(glm::dvec2(screen_x, screen_y));
}

std::optional<gp_Pnt> Occt_view::pt3d_on_plane(const ScreenCoords& screen_coords, const gp_Pln& plane) const
{
  // TODO there must be a way to do this using the MVP matrixes.
  // Convert 2D screen coordinates to 3D world coordinates near the camera
  Standard_Real x_near, y_near, z_near;
  m_view->Convert((Standard_Integer) screen_coords.unsafe_get_x(),
                  (Standard_Integer) screen_coords.unsafe_get_y(),
                  x_near,
                  y_near,
                  z_near);
  gp_Pnt near_point(x_near, y_near, z_near);

  const Graphic3d_Camera_ptr& camera = m_view->Camera();

  auto get_intersection = [&](Geom_Line_ptr& ray_line) -> std::optional<gp_Pnt>
  {
    // Intersect the ray with the plane
    Geom_Plane_ptr geom_plane = new Geom_Plane(plane);
    GeomAPI_IntCS  intersection(ray_line, geom_plane);

    if (intersection.NbPoints() > 0)
      return intersection.Point(1);  // First intersection point

    // No intersection with plane.
    return std::nullopt;
  };

  switch (camera->ProjectionType())
  {
    case Graphic3d_Camera::Projection::Projection_Orthographic:
    {
      gp_Dir view_dir = camera->Direction().Reversed();  // Reverse to point into the scene

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
  BRepBuilderAPI_Transform transformer(original_shape,
                                       current_transform,
                                       true);

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
  gp_Pnt               eye    = camera->Eye();     // Camera position
  gp_Pnt               at     = camera->Center();  // Focal point
  gp_Vec               view_vector(eye, at);       // Direction from eye to focal point
  view_vector.Normalize();
  gp_Dir normal(view_vector);  // Plane normal

  return gp_Pln(point_on_plane, normal);
}

void Occt_view::on_enter(const ScreenCoords& screen_coords)
{
  switch (get_mode())
  {
    case Mode::Sketch_face_extrude:
      sketch_face_extrude(screen_coords);  // Update in case dimension was entered
      finalize_sketch_extrude_();
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
  RevolvedShp_rslt revolved = curr_sketch().revolve_selected(angle);
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
void Occt_view::create_sketch_from_face_(const ScreenCoords& screen_coords)
{
  if (auto face = get_face_(screen_coords); face)
    if (auto pln = plane_from_face(*face); pln)
    {
      // Get the outer wire of the face
      TopoDS_Wire outer_wire = BRepTools::OuterWire(*face);
      EZY_ASSERT(!outer_wire.IsNull());
      m_cur_sketch = std::make_shared<Sketch>("Sketch from face", *this, *pln, outer_wire);
      m_sketches.push_back(m_cur_sketch);
      m_cur_sketch->set_current();
      // fit_face_in_view(*face);
      m_gui.set_mode(Mode::Sketch_inspection_mode);
    }
}

void Occt_view::finalize_sketch_extrude_()
{
  DBG_MSG("");
  EZY_ASSERT(m_extruded);
  add_shp_(m_extruded);
  m_ctx->Remove(m_tmp_dim, false);
  clear_all(m_to_extrude_pt, m_to_extrude, m_extruded,
            m_tmp_dim, m_show_dim_input, m_entered_dim);
  m_ctx->ClearSelected(true);

  // Switch to normal mode to prevent picking up a second mouse click and extruding again.
  gui().set_mode(Mode::Normal);
}

bool Occt_view::cancel_sketch_extrude_()
{
  bool did_cancel = false;
  m_ctx->Remove(m_extruded, true);
  m_ctx->Remove(m_tmp_dim, false);
  did_cancel = m_to_extrude_pt.has_value();
  clear_all(m_to_extrude_pt, m_to_extrude, m_extruded,
            m_tmp_dim, m_show_dim_input, m_entered_dim);

  m_ctx->ClearSelected(true);

  return did_cancel;
}

void Occt_view::create_default_sketch_()
{
  EZY_ASSERT(m_sketches.empty());
  EZY_ASSERT(!m_cur_sketch);
  m_cur_sketch = std::make_shared<Sketch>("Sketch", *this, xy_plane());
  m_sketches.push_back(m_cur_sketch);
  m_cur_sketch->set_current();
}

// Query related
AIS_Shape_ptr Occt_view::get_shape(const ScreenCoords& screen_coords)
{
  // Move the selection to the clicked point
  m_ctx->MoveTo(int(screen_coords.unsafe_get_x()), int(screen_coords.unsafe_get_y()), m_view, Standard_True);

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

const TopoDS_Shape* Occt_view::get_(const ScreenCoords& screen_coords) const
{
  AIS_StatusOfDetection detection_status = m_ctx->MoveTo(
      Standard_Integer(screen_coords.unsafe_get_x()),
      Standard_Integer(screen_coords.unsafe_get_y()), m_view, true);

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
  Standard_Real        x_near, y_near, z_near;
  m_view->Convert((Standard_Integer) screen_coords.unsafe_get_x(),
                  (Standard_Integer) screen_coords.unsafe_get_y(),
                  x_near,
                  y_near,
                  z_near);
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

void Occt_view::add_shp_(ShapeBase_ptr& shp)
{
  shp->SetMaterial(m_default_material);
  shp->set_selection_mode(m_shp_selection_mode);  // Adds to m_ctx
  m_shps.push_back(shp);
}

bool Occt_view::fit_face_in_view(const TopoDS_Face& face)
{
  EZY_ASSERT(!face.IsNull());
  auto pln = plane_from_face(face);
  if (!pln)
    return false;

  // Set camera direction to look down the normal
  gp_Dir normal = pln->Axis().Direction();
  gp_Pnt center = pln->Location();  // Use plane origin as initial center

  // Compute the bounding box of the face to refine the center and size
  Bnd_Box bbox;
  BRepBndLib::Add(face, bbox);
  if (bbox.IsVoid())
  {
    std::cout << "Error: Bounding box is void" << std::endl;
    return false;
  }

  // Get the center of the bounding box
  Standard_Real x_min, y_min, z_min, x_max, y_max, z_max;
  bbox.Get(x_min, y_min, z_min, x_max, y_max, z_max);
  center.SetCoord((x_min + x_max) / 2.0, (y_min + y_max) / 2.0, (z_min + z_max) / 2.0);

  // Set the view direction (looking "down" means opposite to normal)
  gp_Dir view_dir = normal.Reversed();  // Look down at the face
  m_view->SetAt(center.X(), center.Y(), center.Z());
  gp_Pnt eye = center.Translated(gp_Vec(view_dir).Multiplied(100.0));  // Target point (center of face)
  m_view->SetEye(eye.X(), eye.Y(), eye.Z());                           // Eye 100 units above
  m_view->SetUp(0, 0, 1);
  // Fit the face in view
  m_view->FitAll(bbox, 0.5);  // 0.5 is margin factor (50% padding)
  m_view->Redraw();           // Update the view
  return true;
}

// Dimension related
void Occt_view::dimension_input(const ScreenCoords& screen_coords)
{
  switch (get_mode())
  {
    case Mode::Sketch_face_extrude:
      m_show_dim_input = true;
      sketch_face_extrude(screen_coords);
      break;
    default:
      curr_sketch().dimension_input(screen_coords);
      break;
  }
}

double Occt_view::get_dimension_scale() const
{
  return m_dimension_scale;
}

void Occt_view::sketch_face_extrude(const ScreenCoords& screen_coords)
{
  if (!m_to_extrude_pt)
  {
    // DBG_MSG("!m_to_extrude_pt");
    //  Find face to extrude
    for (auto& shp : get_selected())
      if (auto face = dynamic_cast<Sketch_face_shp*>(shp.get()); face)
      {
        m_to_extrude_pln = face->owner_sketch.get_plane();
        m_to_extrude_pt  = closest_to_camera(m_view, face->verts_3d);
        m_curr_view_pln  = get_view_plane(*m_to_extrude_pt);
        m_to_extrude     = shp;
      }
  }
  else
  {
    // DBG_MSG("m_to_extrude_pt");
    //  Extrude the face
    std::optional<gp_Pnt> p = pt3d_on_plane(screen_coords, m_curr_view_pln);
    if (p)  // TODO report error!
    {
      auto   extrude_vec  = gp_Vec(m_to_extrude_pln.Axis().Direction());
      double extrude_dist = m_to_extrude_pln.Distance(*p);
      if (m_entered_dim)
        extrude_dist = *m_entered_dim;

      if (extrude_dist <= Precision::Confusion())
        return;  // TODO report error!

      double scaled_dist = extrude_dist / get_dimension_scale();

      if (m_show_dim_input)
      {
        auto l = [&](float new_dist, bool _)
        {
          m_entered_dim = new_dist * get_dimension_scale();
        };

        gui().set_dist_edit(float(scaled_dist), std::move(std::function<void(float, bool)>(l)), screen_coords);
      }
      if (side_of_plane(m_to_extrude_pln, *p) == Plane_side::Front)
        extrude_vec *= extrude_dist;
      else
        extrude_vec *= -extrude_dist;

      m_ctx->Remove(m_tmp_dim, false);
      m_tmp_dim = create_distance_annotation(gp_Pnt(m_to_extrude_pt->XYZ() + extrude_vec.XYZ()),
                                             *m_to_extrude_pt,
                                             m_curr_view_pln);
      m_tmp_dim->SetCustomValue(scaled_dist);

      m_ctx->Display(m_tmp_dim, false);

      const TopoDS_Face& face = TopoDS::Face(m_to_extrude->Shape());
      TopoDS_Shape       body = BRepPrimAPI_MakePrism(face, extrude_vec);
      if (!m_extruded)
      {
        m_extruded = new ExtrudedShp(*m_ctx, body);
        m_extruded->SetMaterial(m_default_material);
        m_ctx->Display(m_extruded, AIS_Shaded, -1, Standard_True);
      }
      else
      {
        m_extruded->Set(body);
        m_ctx->Redisplay(m_extruded, true);
      }
    }
  }
}

void Occt_view::delete_selected()
{
  auto selected = get_selected();
  delete_(selected);
  cancel(Set_parent_mode::No);  // In case we are in the middle of a operation.
}

void Occt_view::delete_(std::vector<AIS_Shape_ptr>& to_delete)
{
  for (AIS_Shape_ptr& shp : to_delete)
    if (auto wire = dynamic_cast<Sketch_AIS_edge*>(shp.get()); wire)
      wire->owner_sketch.remove_edge(*wire);

  for (auto itr = m_shps.begin(); itr != m_shps.end();)
    if (std::find(to_delete.begin(), to_delete.end(), *itr) != to_delete.end())
      itr = m_shps.erase(itr);
    else
      ++itr;

  remove(to_delete);
}

void Occt_view::do_frame()
{
  if (!m_view.IsNull())
  {
    FlushViewEvents(m_ctx, m_view, true);
    m_view->Redraw();
  }
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

void Occt_view::on_mouse_scroll(double theOffsetX, double theOffsetY)
{
  if (!m_view.IsNull())
    UpdateZoom(Aspect_ScrollDelta(m_occt_window->CursorPosition(), int(theOffsetY * 4.0)));
}

void Occt_view::on_mouse_button(int theButton, int theAction, int theMods)
{
  if (m_view.IsNull())
    return;

  const Graphic3d_Vec2i pos = m_occt_window->CursorPosition();
  if (theAction == GLFW_PRESS)
  {
    PressMouseButton(pos,
                     mouse_button_from_glfw_(theButton),
                     key_flags_from_glfw_(theMods),
                     false);

    if (m_extruded)
    {
      finalize_sketch_extrude_();
      return;
    }

    switch (get_mode())
    {
      case Mode::Sketch_from_face:
        return create_sketch_from_face_(ScreenCoords(glm::dvec2(pos.x(), pos.y())));
      case Mode::Sketch_toggle_edge_dim:
        return curr_sketch().toggle_edge_dim(ScreenCoords(glm::dvec2(pos.x(), pos.y())));
      default:
        break;
    }
  }
  else
    ReleaseMouseButton(pos,
                       mouse_button_from_glfw_(theButton),
                       key_flags_from_glfw_(theMods),
                       false);
}

void Occt_view::on_mouse_move(const ScreenCoords& screen_coords)
{
  EZY_ASSERT(!m_view.IsNull());
  UpdateMousePosition(Graphic3d_Vec2i(int(screen_coords.unsafe_get_x()), int(screen_coords.unsafe_get_y())),
                      PressedMouseButtons(),
                      LastMouseFlags(),
                      false);
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

    m_ctx->NextSelected();  // Move to the next selected object
  }
  return shapes;
}

TopAbs_ShapeEnum Occt_view::get_shp_selection_mode() const
{
  return m_shp_selection_mode;
}

void Occt_view::set_shp_selection_mode(const TopAbs_ShapeEnum mode)
{
  m_shp_selection_mode = mode;
  for (auto& shp : m_shps)
    shp->set_selection_mode(mode);
}

// Material related
const Graphic3d_MaterialAspect& Occt_view::get_default_material() const
{
  return m_default_material;
}
void Occt_view::set_default_material(const Graphic3d_MaterialAspect& mat)
{
  m_default_material = mat;
}

bool Occt_view::is_headless() const
{
  return m_headless_view;
}

// Mode related
Mode Occt_view::get_mode() const
{
  return m_gui.get_mode();
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

  bool ortho = false;
  if (is_sketch_mode(get_mode()))
  {
    ortho = true;
    for (auto shp : m_shps)
    {
      // m_ctx->Remove(shp, false);
      // m_ctx->Erase(shp, false);
    }

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
      case Mode::Sketch_from_face: set_shp_selection_mode(TopAbs_FACE); break;
      case Mode::Shape_chamfer:    on_chamfer_mode();                   break; // Will update selection mode
      default: break;
        // clang-format on
    }

    for (auto shp : m_shps)
    {
      // m_ctx->Redisplay(shp, false);
    }
  }

  if (!is_headless())
  {
    Graphic3d_Camera_ptr camera = m_view->Camera();
    if (ortho)
      camera->SetProjectionType(Graphic3d_Camera::Projection_Orthographic);
    else
      camera->SetProjectionType(Graphic3d_Camera::Projection_Perspective);

    m_view->Redraw();
    m_ctx->UpdateCurrentViewer();
  }
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
  if ((theFlags & GLFW_MOD_SHIFT) != 0)   flags |= Aspect_VKeyFlags_SHIFT;
  if ((theFlags & GLFW_MOD_CONTROL) != 0) flags |= Aspect_VKeyFlags_CTRL;
  if ((theFlags & GLFW_MOD_ALT) != 0)     flags |= Aspect_VKeyFlags_ALT;
  if ((theFlags & GLFW_MOD_SUPER) != 0)   flags |= Aspect_VKeyFlags_META;
  // clang-format on
  return flags;
}

Occt_view::Sketch_list& Occt_view::get_sketches()
{
  return m_sketches;
}

void Occt_view::remove_sketch(const Sketch_ptr& sketch)
{
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
  EZY_ASSERT(m_cur_sketch);
  return *m_cur_sketch;
}

void Occt_view::set_curr_sketch(const Sketch_ptr& to_set)
{
  for (Sketch_ptr& sketch : m_sketches)
    if (sketch.get() == to_set.get())
    {
      m_cur_sketch = sketch;
      m_cur_sketch->set_current();

      // If hide all shapes is enabled, hide all shapes except the current sketch
      if (m_gui.m_hide_all_shapes)
      {
        // Hide all shapes
        for (const ShapeBase_ptr& shape : m_shps)
        {
          shape->set_visible(false);
        }
      }

      return;
    }

  EZY_ASSERT(false);  // Sketch does not belong to this view.
}

std::list<ShapeBase_ptr>& Occt_view::get_shapes()
{
  return m_shps;
}

// clang-format off
Shp_move&      Occt_view::shp_move()      { return m_shp_move;      }
Shp_rotate&    Occt_view::shp_rotate()    { return m_shp_rotate;    }
Shp_chamfer&   Occt_view::shp_chamfer()   { return m_shp_chamfer;   }
Shp_cut&       Occt_view::shp_cut()       { return m_shp_cut;       }
Shp_fuse&      Occt_view::shp_fuse()      { return m_shp_fuse;      }
Shp_common&    Occt_view::shp_common()    { return m_shp_common;    }
Shp_polar_dup& Occt_view::shp_polar_dup() { return m_shp_polar_dup; }
// clang-format on

std::string Occt_view::to_json() const
{
  using namespace nlohmann;
  json  j;
  json& sketches = j["sketches"] = json::array();
  json& shps = j["shapes"] = json::array();
  for (const Sketch_ptr& s : m_sketches)
    sketches.push_back(Sketch_json::to_json(*s));

  for (const ShapeBase_ptr& s : m_shps)
  {
    const TopoDS_Shape& shape = s->Shape();
    std::ostringstream  oss;
    BRepTools::Write(shape, oss, false, false, TopTools_FormatVersion_CURRENT);  // Write BREP data to the stream
    json shp_json;
    shp_json["name"]     = s->get_name();
    shp_json["type"]     = s->get_type_str();
    shp_json["material"] = s->Material();
    shp_json["geom"]     = oss.str();
    shps.push_back(shp_json);
  }

  return j.dump(2);
}

void Occt_view::load(const std::string& json_str)
{
  using namespace nlohmann;
  for (AIS_Shape_ptr& s : m_shps)
    m_ctx->Remove(s, false);

  clear_all(m_sketches, m_cur_sketch, m_shps);
  const json j = json::parse(json_str);
  EZY_ASSERT(j.contains("sketches") && j["sketches"].is_array());
  for (const auto& s : j["sketches"])
  {
    m_sketches.push_back(Sketch_json::from_json(*this, s));
    if (s["isCurrent"])
    {
      EZY_ASSERT(!m_cur_sketch);
      m_cur_sketch = m_sketches.back();
    }
  }
  // We need at lease one sketch. TODO what if the user deletes them all?
  EZY_ASSERT(m_cur_sketch);

  for (const json& s : j["shapes"])
  {
    TopoDS_Shape       shape;
    std::istringstream iss;
    iss.str(s["geom"]);
    BRepTools::Read(shape, iss, BRep_Builder());
    ShapeBase_ptr shp;
    switch (Shape_base::get_type(s["type"]))
    {
      case Shape_type::Extruded:
        shp = new ExtrudedShp(*m_ctx, shape);
        break;
      case Shape_type::Revolved:
        shp = new RevolvedShp(*m_ctx, shape);
        break;
      case Shape_type::Chamfer:
        shp = new ChamferShp(*m_ctx, shape);
        break;
      default:
        break;
    }

    shp->set_name(s["name"]);
    // TODO improve
    shp->SetMaterial(Graphic3d_MaterialAspect(Graphic3d_NameOfMaterial(s["material"])));
    m_shps.push_back(shp);
    m_ctx->Display(shp, AIS_Shaded, 0, true);
  }

  // Ensure correct state
  gui().set_mode(Mode::Normal);
}

bool Occt_view::import_step(const std::string& step_data)
{
  // Initialize the STEP reader
  STEPControl_Reader reader;

  std::istringstream stream(step_data);

  // Read the STEP file
  IFSelect_ReturnStatus status = reader.ReadStream("", stream);
  if (status != IFSelect_RetDone)
  {
    std::cerr << "Error: Cannot read the STEP file." << std::endl;
    return 1;
  }

  // Transfer all root entities
  Standard_Integer num_roots = reader.TransferRoots();
  if (num_roots == 0)
  {
    std::cerr << "Error: No shapes were transferred." << std::endl;
    return 1;
  }

  // Get the number of shapes
  Standard_Integer num_shps = reader.NbShapes();
  std::cout << "Number of shapes loaded: " << num_shps << std::endl;

  // Iterate over each shape
  for (Standard_Integer i = 1; i <= num_shps; ++i)
  {
    TopoDS_Shape shape = reader.Shape(i);
    if (shape.IsNull())
    {
      std::cerr << "Warning: Shape " << i << " is null." << std::endl;
      continue;
    }

    ExtrudedShp_ptr shp = new ExtrudedShp(*m_ctx, shape);
    add_shp_(shp);
  }

  return true;
}

GUI& Occt_view::gui()
{
  return m_gui;
}
AIS_InteractiveContext& Occt_view::ctx()
{
  return *m_ctx;
}

void Occt_view::new_file()
{
  remove(m_shps);
  clear_all(m_shps, m_sketches, m_cur_sketch);

  create_default_sketch_();
  m_gui.set_mode(Mode::Normal);
}