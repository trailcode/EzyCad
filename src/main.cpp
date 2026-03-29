// Dear ImGui + EzyCad GUI + OCCT 3D view + chained GLFW input.
// On wasm, sizing is handled by imgui_impl_glfw (OnCanvasSizeChange: CSS * DPR + canvas); do not
// second-guess with extra glfwSetWindowSize/io overrides here — they fight that path and break input.

#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <functional>

#include "gui.h"
#include "imgui.h"
#include "ui_font.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h>  // Will drag system OpenGL headers

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

// This example can also compile and run with Emscripten! See 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include "emscripten/emscripten_mainloop_stub.h"

static GUI* s_gui_for_unload = nullptr;

extern "C" void emscripten_save_settings_on_unload()
{
  if (s_gui_for_unload)
    s_gui_for_unload->save_occt_view_settings();
}
#endif

static void glfw_error_callback(int error, const char* description)
{
  fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

static std::function<void(GLFWwindow* window, int key, int scancode, int action, int mods)> keyCallback;
static std::function<void(GLFWwindow* window, double xpos, double ypos)>                    cursorPosCallback;
static std::function<void(GLFWwindow* window, int button, int action, int mods)>            mouseButtonCallback;
static std::function<void(GLFWwindow* window, int width, int height)>                       windowSizeCallback;
static std::function<void(GLFWwindow* window, double xoffset, double yoffset)>              scroll_callback;

void key_callback_wrapper(GLFWwindow* window,
                          int         key,
                          int         scancode,
                          int         action,
                          int         mods)
{
  if (keyCallback)
    keyCallback(window, key, scancode, action, mods);
}

void cursor_pos_callback_wrapper(GLFWwindow* window, double xpos, double ypos)
{
  if (cursorPosCallback)
    cursorPosCallback(window, xpos, ypos);
}

void mouse_button_callback_wrapper(GLFWwindow* window,
                                   int         button,
                                   int         action,
                                   int         mods)
{
  if (mouseButtonCallback)
    mouseButtonCallback(window, button, action, mods);
}

void window_size_callback_wrapper(GLFWwindow* window, int width, int height)
{
  if (windowSizeCallback)
    windowSizeCallback(window, width, height);
}

void scroll_callback_wrapper(GLFWwindow* window, double xoffset, double yoffset)
{
  if (scroll_callback)
    scroll_callback(window, xoffset, yoffset);
}

// Main code
int main(int, char**)
{
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit())
    return 1;

  // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
  // GL ES 2.0 + GLSL 100 (WebGL 1.0)
  const char* glsl_version = "#version 100";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
  // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
  const char* glsl_version = "#version 300 es";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
  // GL 3.2 + GLSL 150
  const char* glsl_version = "#version 150";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
  // GL 3.0 + GLSL 130
  const char* glsl_version = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
  // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

  // Create window with graphics context
  float       main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());  // Valid on GLFW 3.3+ only
  GLFWwindow* window     = glfwCreateWindow((int) (1280 * main_scale), (int) (800 * main_scale), "EzyCad", nullptr, nullptr);
  if (window == nullptr)
    return 1;
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);  // Enable vsync
#ifndef __EMSCRIPTEN__
  glfwMaximizeWindow(window);
#endif

#ifdef __EMSCRIPTEN__
  // GLFW often reports content scale 1.0 on wasm while the browser uses devicePixelRatio > 1.
  // Windows native uses monitor content scale for main_scale — align wasm so font/style size matches.
  {
    int ww = 0, wh = 0, fbw = 0, fbh = 0;
    glfwGetWindowSize(window, &ww, &wh);
    glfwGetFramebufferSize(window, &fbw, &fbh);
    float fb_scale = 1.0f;
    if (ww > 0 && wh > 0)
      fb_scale = ((float)fbw / (float)ww + (float)fbh / (float)wh) * 0.5f;
    const float dpr = emscripten_get_device_pixel_ratio();
    if (main_scale <= 1.0f)
      main_scale = (fb_scale > 1.01f) ? fb_scale : dpr;
  }
#endif

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void) io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  // ImGui::StyleColorsLight();

  // Setup scaling
  ImGuiStyle& style = ImGui::GetStyle();
  style.ScaleAllSizes(main_scale);  // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
  style.FontScaleDpi = main_scale;  // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
#ifdef __EMSCRIPTEN__
  ImGui_ImplGlfw_InstallEmscriptenCallbacks(window, "#canvas");
#endif
  ImGui_ImplOpenGL3_Init(glsl_version);

  // DroidSans at logical px; do not multiply by main_scale — FontScaleDpi applies HiDPI.
  {
#ifdef __EMSCRIPTEN__
    ImFont* font = io.Fonts->AddFontFromFileTTF("/DroidSans.ttf", k_imgui_base_font_size_px);
#else
    ImFont* font = io.Fonts->AddFontFromFileTTF("DroidSans.ttf", k_imgui_base_font_size_px);
#endif
    IM_ASSERT(font != nullptr);
  }

  // Monospace for script console (bundled ImGui font: Cousine).
  ImFont* console_font = nullptr;
#ifdef __EMSCRIPTEN__
  console_font = io.Fonts->AddFontFromFileTTF("/Cousine-Regular.ttf", k_imgui_base_font_size_px);
#else
  console_font = io.Fonts->AddFontFromFileTTF("Cousine-Regular.ttf", k_imgui_base_font_size_px);
  if (!console_font)
    console_font = io.Fonts->AddFontFromFileTTF("third_party/imgui/fonts/Cousine-Regular.ttf", k_imgui_base_font_size_px);
#endif
  IM_ASSERT(console_font != nullptr);

  io.IniFilename = nullptr;  // Layout persisted via ezycad_settings.json (see GUI)

  GUI gui;
  gui.set_console_font(console_font);
  gui.init(window);

#ifdef __EMSCRIPTEN__
  s_gui_for_unload = &gui;
  EM_ASM(
      {
        window.addEventListener('beforeunload', function() { Module.ccall('emscripten_save_settings_on_unload', null, [], []); });
      });
#endif

  keyCallback = [&](GLFWwindow* window, int key, int scancode, int action, int mods)
  {
    if (key == GLFW_KEY_TAB)
    {
      if (action == GLFW_PRESS)
        gui.on_key(key, scancode, action, mods);
      return;
    }

    // Script console toggle: always reach GUI (ImGui text fields would otherwise capture F12 / Ctrl+Shift+L).
#ifndef __EMSCRIPTEN__
    if (action == GLFW_PRESS && key == GLFW_KEY_F12)
    {
      gui.on_key(key, scancode, action, mods);
      return;
    }
#else
    if (action == GLFW_PRESS && (mods & GLFW_MOD_CONTROL) && (mods & GLFW_MOD_SHIFT) != 0 && key == GLFW_KEY_L)
    {
      gui.on_key(key, scancode, action, mods);
      return;
    }
#endif

    if (action == GLFW_PRESS && (mods & GLFW_MOD_CONTROL) &&
        (key == GLFW_KEY_Z || key == GLFW_KEY_Y))
    {
      gui.on_key(key, scancode, action, mods);
      return;
    }

    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
    if (!io.WantCaptureKeyboard)
      gui.on_key(key, scancode, action, mods);
  };

  cursorPosCallback = [&](GLFWwindow* window, double xpos, double ypos)
  {
    ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
    if (!io.WantCaptureMouse)
      gui.on_mouse_pos(ScreenCoords(glm::dvec2(xpos, ypos)));
  };

  mouseButtonCallback = [&](GLFWwindow* window, int button, int action, int mods)
  {
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
    if (!io.WantCaptureMouse)
      gui.on_mouse_button(button, action, mods);
  };

  windowSizeCallback = [&](GLFWwindow* window, int width, int height)
  {
    gui.on_resize(width, height);
  };

  scroll_callback = [&](GLFWwindow* window, double xoffset, double yoffset)
  {
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
    if (!io.WantCaptureMouse)
      gui.on_mouse_scroll(xoffset, yoffset);
  };

  glfwSetKeyCallback(window, key_callback_wrapper);
  glfwSetCursorPosCallback(window, cursor_pos_callback_wrapper);
  glfwSetMouseButtonCallback(window, mouse_button_callback_wrapper);
  glfwSetWindowSizeCallback(window, window_size_callback_wrapper);
  glfwSetScrollCallback(window, scroll_callback_wrapper);

  // Main loop
#ifdef __EMSCRIPTEN__
  EMSCRIPTEN_MAINLOOP_BEGIN
#else
  while (!glfwWindowShouldClose(window))
#endif
  {
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    glfwPollEvents();
    if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
    {
      ImGui_ImplGlfw_Sleep(10);
      continue;
    }

    // Start the Dear ImGui frame (platform must set DisplaySize before renderer NewFrame)
    ImGui_ImplGlfw_NewFrame();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    gui.render_gui();

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    ImVec4 clear_color = gui.get_clear_color();
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);

    gui.render_occt();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);

    if (io.WantSaveIniSettings)
    {
      gui.save_occt_view_settings();
      io.WantSaveIniSettings = false;
    }
  }
#ifdef __EMSCRIPTEN__
  EMSCRIPTEN_MAINLOOP_END;
#endif

  // Cleanup
  gui.save_occt_view_settings();
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
