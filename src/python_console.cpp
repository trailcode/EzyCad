#include "python_console.h"

#include "gui.h"
#include "imgui.h"
#include "modes.h"
#include "occt_view.h"
#include "shp.h"
#include "sketch.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

#ifdef EZYCAD_HAVE_PYTHON
#  ifdef _MSC_VER
// pybind11 pulls in Python.h; avoid python3xx_d.lib #pragma in Debug (we link release DLL).
#    ifdef _DEBUG
#      pragma push_macro("_DEBUG")
#      undef _DEBUG
#      define EZYCAD_POP_DEBUG_AFTER_PYBIND
#    endif
#  endif
#  include <pybind11/embed.h>
#  include <pybind11/eval.h>
#  ifdef EZYCAD_POP_DEBUG_AFTER_PYBIND
#    pragma pop_macro("_DEBUG")
#    undef EZYCAD_POP_DEBUG_AFTER_PYBIND
#  endif

namespace py = pybind11;

// Registered with py::class_; must be visible to PYBIND11_EMBEDDED_MODULE at file scope.
struct Ezy_shp
{
  Shp_ptr shp;
};

namespace
{
GUI*            g_py_gui     = nullptr;
Python_console* g_py_console = nullptr;

// Requires PyErr to be set. After `catch (py::error_already_set& e)`, call `e.restore()` first -
// pybind11 stores the error in the exception object and clears PyErr until restored.
void append_python_exception_to_history(std::vector<std::string>& history, bool* scroll, uint64_t* log_display_version)
{
  PyObject *type = nullptr, *value = nullptr, *traceback = nullptr;
  PyErr_Fetch(&type, &value, &traceback);
  PyErr_NormalizeException(&type, &value, &traceback);
  PyObject* lines = nullptr;
  if (value)
  {
    PyObject* tb_mod = PyImport_ImportModule("traceback");
    if (tb_mod)
    {
      PyObject* fmt = PyObject_GetAttrString(tb_mod, "format_exception");
      if (fmt && PyCallable_Check(fmt))
      {
        PyObject* t     = type ? type : Py_None;
        PyObject* v     = value ? value : Py_None;
        PyObject* tb    = traceback ? traceback : Py_None;
        PyObject* args  = PyTuple_Pack(3, t, v, tb);
        if (args)
        {
          lines = PyObject_CallObject(fmt, args);
          Py_DECREF(args);
        }
      }
      Py_XDECREF(fmt);
      Py_DECREF(tb_mod);
    }
  }
  if (lines && PyList_Check(lines))
  {
    Py_ssize_t n = PyList_GET_SIZE(lines);
    for (Py_ssize_t i = 0; i < n; ++i)
    {
      PyObject* item = PyList_GET_ITEM(lines, i);
      const char* u  = PyUnicode_AsUTF8(item);
      if (u)
      {
        std::string line(u);
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
          line.pop_back();
        history.push_back("[err] " + line);
      }
    }
    *scroll = true;
  }
  else
  {
    PyObject* s      = value ? PyObject_Str(value) : nullptr;
    const char* msg  = s ? PyUnicode_AsUTF8(s) : "Python error";
    history.push_back(std::string("[err] ") + (msg ? msg : "?"));
    *scroll = true;
    Py_XDECREF(s);
  }
  Py_XDECREF(lines);
  Py_XDECREF(type);
  Py_XDECREF(value);
  Py_XDECREF(traceback);
  PyErr_Clear();
  if (log_display_version)
    ++*log_display_version;
}

static const char* k_bootstrap = R"PY(
def _ezycad_bootstrap():
    import ezycad_native as _n
    class View:
        def sketch_count(self):
            return _n.view_sketch_count()
        def shape_count(self):
            return _n.view_shape_count()
        def curr_sketch_name(self):
            return _n.view_curr_sketch_name()
        def add_box(self, ox, oy, oz, w, l, h):
            return _n.view_add_box(ox, oy, oz, w, l, h)
        def add_sphere(self, ox, oy, oz, r):
            return _n.view_add_sphere(ox, oy, oz, r)
        def get_shape(self, i):
            return _n.view_get_shape(int(i))
    class Ezy:
        def log(self, msg):
            return _n.ezy_log(msg)
        def msg(self, text):
            return _n.ezy_msg(text)
        def get_mode(self):
            return _n.ezy_get_mode()
        def set_mode(self, name):
            return _n.ezy_set_mode(name)
        def help(self):
            return _n.ezy_help()
    import __main__
    __main__.ezy = Ezy()
    __main__.view = View()
    __main__.Shp = _n.Shp
    __main__.help = lambda: __main__.ezy.help()
    import builtins
    builtins.print = lambda *a: __main__.ezy.log(" ".join(str(x) for x in a))
_ezycad_bootstrap()
del _ezycad_bootstrap
)PY";

}  // namespace

PYBIND11_EMBEDDED_MODULE(ezycad_native, m)
{
  py::class_<Ezy_shp>(m, "Shp")
      .def("name", [](const Ezy_shp& s) { return s.shp->get_name(); })
      .def("set_name", [](Ezy_shp& s, const std::string& n) { s.shp->set_name(n); })
      .def("visible", [](const Ezy_shp& s) { return s.shp->get_visible(); })
      .def("set_visible", [](Ezy_shp& s, bool v) { s.shp->set_visible(v); })
      .def(
          "__repr__",
          [](const Ezy_shp& s)
          {
            return "<ezycad.Shp name='" + s.shp->get_name() + "'>";
          });

  m.def(
      "ezy_log",
      [](const py::object& msg)
      {
        std::string line = py::str(msg).cast<std::string>();
        if (g_py_gui)
          g_py_gui->log_message(line);
        if (g_py_console)
          g_py_console->append_line_from_python(line);
      },
      py::arg("msg"));

  m.def(
      "ezy_msg",
      [](const std::string& text)
      {
        if (g_py_gui)
          g_py_gui->show_message(text);
      },
      py::arg("text"));

  m.def(
      "ezy_get_mode",
      []
      {
        if (!g_py_gui)
          return std::string("Normal");
        Mode mmode = g_py_gui->get_mode();
        return std::string(c_mode_strs[static_cast<int>(mmode)]);
      });

  m.def(
      "ezy_set_mode",
      [](const std::string& name)
      {
        if (g_py_gui)
          g_py_gui->set_mode(mode_from_string(name));
      },
      py::arg("name"));

  m.def(
      "ezy_help",
      []
      {
        if (!g_py_console)
          return;
        const char* help_text =
            "ezy:\n"
            "  ezy.log(msg)          - append message to console and log window\n"
            "  ezy.msg(text)         - show status message\n"
            "  ezy.get_mode()        - return current mode name\n"
            "  ezy.set_mode(name)    - set mode by name\n"
            "  ezy.help()            - print this help\n"
            "view:\n"
            "  view.sketch_count()          - number of sketches\n"
            "  view.shape_count()           - number of shapes\n"
            "  view.curr_sketch_name()      - current sketch name\n"
            "  view.add_box(ox,oy,oz,w,l,h) - add box\n"
            "  view.add_sphere(ox,oy,oz,r)  - add sphere\n"
            "  view.get_shape(i)            - get shape by 0-based index (raises IndexError if out of range)\n"
            "Shp:\n"
            "  s.name()              - get shape name\n"
            "  s.set_name(s)         - set shape name\n"
            "  s.visible()           - get visibility\n"
            "  s.set_visible(b)      - set visibility";
        g_py_console->append_line_from_python(help_text);
      });

  m.def(
      "view_sketch_count",
      []
      {
        Occt_view* view = g_py_gui && g_py_gui->get_view() ? g_py_gui->get_view() : nullptr;
        return view ? static_cast<std::size_t>(view->get_sketches().size()) : std::size_t {0};
      });

  m.def(
      "view_shape_count",
      []
      {
        Occt_view* view = g_py_gui && g_py_gui->get_view() ? g_py_gui->get_view() : nullptr;
        return view ? static_cast<std::size_t>(view->get_shapes().size()) : std::size_t {0};
      });

  m.def(
      "view_curr_sketch_name",
      []
      {
        Occt_view* view = g_py_gui && g_py_gui->get_view() ? g_py_gui->get_view() : nullptr;
        return view ? view->curr_sketch().get_name() : std::string {};
      });

  m.def(
      "view_add_box",
      [](double ox, double oy, double oz, double w, double len, double h)
      {
        Occt_view* view = g_py_gui && g_py_gui->get_view() ? g_py_gui->get_view() : nullptr;
        if (view)
          view->add_box(ox, oy, oz, w, len, h);
      },
      py::arg("ox"),
      py::arg("oy"),
      py::arg("oz"),
      py::arg("w"),
      py::arg("l"),
      py::arg("h"));

  m.def(
      "view_add_sphere",
      [](double ox, double oy, double oz, double r)
      {
        Occt_view* view = g_py_gui && g_py_gui->get_view() ? g_py_gui->get_view() : nullptr;
        if (view)
          view->add_sphere(ox, oy, oz, r);
      },
      py::arg("ox"),
      py::arg("oy"),
      py::arg("oz"),
      py::arg("r"));

  m.def(
      "view_get_shape",
      [](std::ptrdiff_t idx) -> Ezy_shp
      {
        Occt_view* view = g_py_gui && g_py_gui->get_view() ? g_py_gui->get_view() : nullptr;
        if (!view)
          throw std::runtime_error("no 3D view available");
        if (idx < 0)
          throw py::index_error("shape index must be >= 0");
        std::list<Shp_ptr>& shapes = view->get_shapes();
        auto                it     = shapes.begin();
        for (std::ptrdiff_t i = 0; i < idx && it != shapes.end(); ++i, ++it)
          ;
        if (it == shapes.end())
          throw py::index_error("shape index out of range");
        return Ezy_shp {*it};
      },
      py::arg("i"));
}

struct Python_console::Ezycad_python_runtime
{
  py::scoped_interpreter guard;
};

#endif  // EZYCAD_HAVE_PYTHON

void Python_console::append_line_from_python(const std::string& line)
{
  append_line(line, false);
}

void Python_console::append_line(const std::string& line, bool is_error)
{
  m_history.push_back(is_error ? "[err] " + line : line);
  m_scroll_to_bottom = true;
  ++m_log_display_version;
}

Python_console::Python_console(GUI* gui)
    : m_gui(gui)
{
#ifdef EZYCAD_HAVE_PYTHON
  m_python_ok = init_python_();
  if (m_python_ok)
    append_line("Python console ready (pybind11). Try: ezy.log('hello'), view.sketch_count()");
  else
    append_line("Python failed to initialize. Is PYTHONHOME set and the runtime on PATH?", true);
  load_scripts();
#else
  append_line("Python console is not available in this build (configure with Python development libraries).");
#endif
}

Python_console::~Python_console()
{
#ifdef EZYCAD_HAVE_PYTHON
  if (m_python_ok)
  {
    g_py_gui     = nullptr;
    g_py_console = nullptr;
    m_py_runtime.reset();
  }
#endif
}

#ifdef EZYCAD_HAVE_PYTHON
bool Python_console::init_python_()
{
  g_py_gui     = m_gui;
  g_py_console = this;
  try
  {
    m_py_runtime = std::make_unique<Ezycad_python_runtime>();
  }
  catch (const std::exception& e)
  {
    append_line(std::string("[err] Python interpreter: ") + e.what(), true);
    g_py_gui     = nullptr;
    g_py_console = nullptr;
    m_py_runtime.reset();
    return false;
  }
  catch (...)
  {
    append_line("[err] Python interpreter: unknown error", true);
    g_py_gui     = nullptr;
    g_py_console = nullptr;
    m_py_runtime.reset();
    return false;
  }

  try
  {
    py::exec(py::str(k_bootstrap), py::globals(), py::globals());
  }
  catch (py::error_already_set& e)
  {
    e.restore();
    append_python_exception_to_history(m_history, &m_scroll_to_bottom, &m_log_display_version);
    g_py_gui     = nullptr;
    g_py_console = nullptr;
    m_py_runtime.reset();
    return false;
  }
  return true;
}

void Python_console::load_scripts()
{
  if (!m_python_ok)
    return;
#ifdef __EMSCRIPTEN__
  const std::filesystem::path scripts_dir("/res/scripts/python");
#else
  const std::filesystem::path scripts_dir("res/scripts/python");
#endif
  if (!std::filesystem::is_directory(scripts_dir))
    return;

  std::vector<std::filesystem::path> py_files;
  for (const auto& entry : std::filesystem::directory_iterator(scripts_dir))
  {
    if (!entry.is_regular_file() || entry.path().extension() != ".py")
      continue;
    py_files.push_back(entry.path());
  }
  std::sort(py_files.begin(), py_files.end());

  for (const auto& path : py_files)
  {
    std::string path_str = path.string();
    std::string filename = path.filename().string();

    std::string content;
    std::ifstream f(path_str);
    if (f)
    {
      content.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
      f.close();
    }

    Python_script_editor entry;
    entry.path     = path_str;
    entry.filename = filename;
    entry.editor.SetTabSize(4);
    entry.editor.SetLanguage(TextEditor::Language::Python());
    entry.editor.SetPalette(TextEditor::GetDarkPalette());
    entry.editor.SetText(content);

    if (!content.empty())
    {
      try
      {
        py::exec(py::str(content), py::globals(), py::globals());
      }
      catch (py::error_already_set& e)
      {
        append_line("script " + filename + ":", true);
        e.restore();
        append_python_exception_to_history(m_history, &m_scroll_to_bottom, &m_log_display_version);
      }
    }

    m_script_editors.push_back(std::move(entry));
  }
}

void Python_console::execute(const std::string& code)
{
  if (!m_python_ok || code.empty())
    return;

  if (m_cmd_history.empty() || m_cmd_history.back() != code)
    m_cmd_history.push_back(code);
  m_cmd_history_pos = -1;

  append_line("> " + code);

  try
  {
    py::object result =
        py::eval<py::eval_single_statement>(py::str(code), py::globals(), py::globals());
    if (result.ptr() != nullptr && !result.is_none())
      append_line(py::repr(result).cast<std::string>());
  }
  catch (py::error_already_set& e)
  {
    e.restore();
    append_python_exception_to_history(m_history, &m_scroll_to_bottom, &m_log_display_version);
  }
}
#else
void Python_console::load_scripts() {}

void Python_console::execute(const std::string& code)
{
  (void) code;
}
#endif

int Python_console::text_edit_callback(ImGuiInputTextCallbackData* data)
{
  auto* console = static_cast<Python_console*>(data->UserData);
  if (!console)
    return 0;

  if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory && !console->m_cmd_history.empty())
  {
    const int history_size = static_cast<int>(console->m_cmd_history.size());

    if (data->EventKey == ImGuiKey_UpArrow)
    {
      if (console->m_cmd_history_pos == -1)
        console->m_cmd_history_pos = history_size - 1;
      else if (console->m_cmd_history_pos > 0)
        --console->m_cmd_history_pos;
    }
    else if (data->EventKey == ImGuiKey_DownArrow)
    {
      if (console->m_cmd_history_pos != -1)
      {
        if (console->m_cmd_history_pos + 1 < history_size)
          ++console->m_cmd_history_pos;
        else
          console->m_cmd_history_pos = -1;
      }
    }

    const char* new_buf = "";
    if (console->m_cmd_history_pos != -1)
      new_buf = console->m_cmd_history[console->m_cmd_history_pos].c_str();

    data->DeleteChars(0, data->BufTextLen);
    data->InsertChars(0, new_buf);
    data->SelectionStart = 0;
    data->SelectionEnd   = data->BufTextLen;
  }

  return 0;
}

int Python_console::log_display_resize_callback(ImGuiInputTextCallbackData* data)
{
  if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
  {
    auto* self = static_cast<Python_console*>(data->UserData);
    self->m_log_display_buf.resize(static_cast<size_t>(data->BufTextLen) + 1);
    data->Buf = self->m_log_display_buf.data();
  }
  return 0;
}

void Python_console::render(bool* p_open)
{
  ImFont* console_font = m_gui ? m_gui->console_font() : nullptr;

  if (!ImGui::Begin("Python Console", p_open, ImGuiWindowFlags_None))
  {
    ImGui::End();
    return;
  }

  if (!ImGui::BeginTabBar("PythonConsoleTabs", ImGuiTabBarFlags_None))
  {
    ImGui::End();
    return;
  }

  if (ImGui::BeginTabItem("Console"))
  {
    if (console_font)
      ImGui::PushFont(console_font);

    float height = ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing() - 4.f;
    if (height > 80.f)
    {
      if (m_log_display_version != m_log_display_built_version)
      {
        constexpr size_t k_max_lines = 5000;
        constexpr size_t k_max_chars = 512 * 1024;
        m_log_display_buf.clear();
        m_log_display_buf.reserve(m_history.size() * 32);
        size_t start_idx = m_history.size() > k_max_lines ? m_history.size() - k_max_lines : 0;
        for (size_t i = start_idx; i < m_history.size() && m_log_display_buf.size() < k_max_chars; ++i)
        {
          m_log_display_buf += m_history[i];
          m_log_display_buf += '\n';
        }
        m_log_display_buf.push_back('\0');
        m_log_display_built_version = m_log_display_version;
      }
      if (m_log_display_buf.empty())
        m_log_display_buf.push_back('\0');

      ImGui::BeginChild("Python_consoleScroll", ImVec2(0, height), true, ImGuiWindowFlags_HorizontalScrollbar);
      ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.f, 0.f, 0.f, 0.f));
      ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.f, 0.f, 0.f, 0.f));
      ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.f, 0.f, 0.f, 0.f));
      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 0.f));
      ImGui::InputTextMultiline(
          "##python_log",
          m_log_display_buf.data(),
          m_log_display_buf.size(),
          ImVec2(-1.f, -1.f),
          ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_NoUndoRedo,
          &Python_console::log_display_resize_callback,
          this);
      ImGui::PopStyleVar();
      ImGui::PopStyleColor(3);
      if (m_scroll_to_bottom)
      {
        ImGui::SetScrollHereY(1.0f);
        m_scroll_to_bottom = false;
      }
      ImGui::EndChild();
    }

    ImGui::SetNextItemWidth(-1);
#ifdef EZYCAD_HAVE_PYTHON
    const bool can_run = m_python_ok;
#else
    const bool can_run = false;
#endif
    ImGui::BeginDisabled(!can_run);
    bool run = ImGui::InputTextWithHint("##python_input",
                                        "Enter Python code (e.g. ezy.log('hi'))",
                                        m_input_buf,
                                        k_input_buf_size,
                                        ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory,
                                        &Python_console::text_edit_callback,
                                        this);
    ImGui::EndDisabled();

    if (run && can_run)
    {
      execute(m_input_buf);
      m_input_buf[0] = '\0';
      ImGui::SetKeyboardFocusHere(-1);
    }

    if (console_font)
      ImGui::PopFont();
    ImGui::EndTabItem();
  }

  for (size_t i = 0; i < m_script_editors.size(); ++i)
  {
    Python_script_editor& script = m_script_editors[i];
    if (!ImGui::BeginTabItem(script.filename.c_str()))
      continue;

    ImVec2 editor_size(-1.f, ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing() - 4.f);
    if (editor_size.y > 60.f)
    {
      const std::string editor_id = "##py_script_" + script.filename;
      script.editor.Render(editor_id.c_str(), editor_size, false);
    }

#ifdef EZYCAD_HAVE_PYTHON
    const bool can_save = m_python_ok;
#else
    const bool can_save = false;
#endif
    ImGui::BeginDisabled(!can_save);
    if (ImGui::Button("Save"))
    {
      const std::string to_save = script.editor.GetText();
      std::ofstream of(script.path);
      if (of)
      {
        if (!to_save.empty())
          of.write(to_save.data(), static_cast<std::streamsize>(to_save.size()));
        if (of.good())
          append_line("Saved " + script.filename);
        else
          append_line("Save failed: " + script.filename, true);
      }
      else
        append_line("Could not open for write: " + script.path, true);
    }
    ImGui::EndDisabled();

    ImGui::EndTabItem();
  }

  ImGui::EndTabBar();
  ImGui::End();
}

