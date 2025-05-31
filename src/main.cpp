#define WIN32_LEAN_AND_MEAN
#include "gui.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "occt_view.h"
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h>  // Will drag system OpenGL headers

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && \
    !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

#ifdef __EMSCRIPTEN__
#include "third_party/imgui/emscripten/emscripten_mainloop_stub.h"
#endif

static void glfw_error_callback(int error, const char* description)
{
  fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// Global lambda storage
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
  // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+
  // only glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // 3.0+ only
#endif

  // Create window with graphics context
  GLFWwindow* window = glfwCreateWindow(1,
                                        1,
                                        "EzyCad",
                                        nullptr,
                                        nullptr);
  if (window == nullptr)
    return 1;

  // Add this line to maximize the window
  glfwMaximizeWindow(window);

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);  // Enable vsync

  //glfwSetWindowIcon()

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void) io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

  // Setup Dear ImGui style
  //ImGui::StyleColorsDark();
  ImGui::StyleColorsLight();

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
#ifdef __EMSCRIPTEN__
  io.IniFilename = nullptr;  // Disable automatic saving/loading
  ImGui::LoadIniSettingsFromDisk("/imgui.ini");
  ImGui_ImplGlfw_InstallEmscriptenCallbacks(window, "#canvas");
#endif
  ImGui_ImplOpenGL3_Init(glsl_version);
  io.Fonts->AddFontFromFileTTF("DroidSans.ttf", 18.0f);

  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

  GUI gui;
  gui.init(window);

  keyCallback = [&](GLFWwindow* window, int key, int scancode, int action, int mods)
  {
    /*
    if (!io.WantCaptureKeyboard)
      gui.on_key(key, scancode, action, mods);
    else
      ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
      */
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
    gui.on_key(key, scancode, action, mods);
  };

  cursorPosCallback = [&](GLFWwindow* window, double xpos, double ypos)
  {
    if (!io.WantCaptureMouse)
      gui.on_mouse_pos(ScreenCoords(glm::dvec2(xpos, ypos)));

    ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
  };

  mouseButtonCallback = [&](GLFWwindow* window, int button, int action, int mods)
  {
    if (!io.WantCaptureMouse)
      gui.on_mouse_button(button, action, mods);

    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
  };

  windowSizeCallback = [&](GLFWwindow* window, int width, int height)
  {
    // Update the view size
    gui.on_resize(width, height);
  };

  scroll_callback = [&](GLFWwindow* window, double xoffset, double yoffset)
  {
    if (!io.WantCaptureMouse)
      gui.on_mouse_scroll(xoffset, yoffset);

    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
  };

  glfwSetKeyCallback(window, key_callback_wrapper);
  glfwSetCursorPosCallback(window, cursor_pos_callback_wrapper);
  glfwSetMouseButtonCallback(window, mouse_button_callback_wrapper);
  glfwSetWindowSizeCallback(window, window_size_callback_wrapper);
  glfwSetScrollCallback(window, scroll_callback_wrapper);

  // Main loop
#ifdef __EMSCRIPTEN__
  // For an Emscripten build we are disabling file-system access, so let's not
  // attempt to do a fopen() of the imgui.ini file. You may manually call
  // LoadIniSettingsFromMemory() to load settings from your own storage.
  io.IniFilename = nullptr;
  EMSCRIPTEN_MAINLOOP_BEGIN
#else
  while (!glfwWindowShouldClose(window))
#endif
  {
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to
    // tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to
    // your main application, or clear/overwrite your copy of the mouse data.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input
    // data to your main application, or clear/overwrite your copy of the
    // keyboard data. Generally you may always pass all inputs to dear imgui,
    // and hide them from your application based on those two flags.
    glfwPollEvents();
    if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
    {
      ImGui_ImplGlfw_Sleep(10);
      continue;
    }

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    gui.render_gui();

    // Rendering

    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    static int last_display_w = 0;
    static int last_display_h = 0;
    if (last_display_w != display_w || last_display_h != display_h)
    {
      // TODO use glfw callback;
      last_display_w = display_w;
      last_display_h = display_h;
      // view.onResize(display_w, display_h);
    }

    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x * clear_color.w,
                 clear_color.y * clear_color.w,
                 clear_color.z * clear_color.w,
                 clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);

    gui.render_occt();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
  }
#ifdef __EMSCRIPTEN__
  EMSCRIPTEN_MAINLOOP_END;
#endif

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
