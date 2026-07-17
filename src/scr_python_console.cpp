#include "scr_python_console.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "gui.h"
#include "imgui.h"
#include "mode.h"
#include "gui_occt_view.h"
#include "shp.h"
#include "shp_common.h"
#include "shp_cut.h"
#include "shp_fuse.h"
#include "skt.h"
#include "utl_geom.h"

#ifdef EZYCAD_HAVE_PYTHON
#ifdef _MSC_VER
// pybind11 pulls in Python.h; avoid python3xx_d.lib #pragma in Debug (we link release DLL).
#ifdef _DEBUG
#pragma push_macro("_DEBUG")
#undef _DEBUG
#define EZYCAD_POP_DEBUG_AFTER_PYBIND
#endif
#endif
#include <pybind11/embed.h>
#include <pybind11/eval.h>
#ifdef EZYCAD_POP_DEBUG_AFTER_PYBIND
#pragma pop_macro("_DEBUG")
#undef EZYCAD_POP_DEBUG_AFTER_PYBIND
#endif

namespace py = pybind11;

namespace
{
const TextEditor::LanguageDefinition& python_language_definition()
{
  static TextEditor::LanguageDefinition lang_def;
  static bool                           initialized = false;
  if (!initialized)
  {
    static const char* const keywords[] = {
        "False",  "None",     "True", "and",    "as",      "assert", "async",  "await",  "break", "class",  "continue", "def",
        "del",    "elif",     "else", "except", "finally", "for",    "from",   "global", "if",    "import", "in",       "is",
        "lambda", "nonlocal", "not",  "or",     "pass",    "raise",  "return", "try",    "while", "with",   "yield",
    };
    for (const char* kw : keywords)
      lang_def.mKeywords.insert(kw);

    lang_def.mTokenRegexStrings.push_back(
        std::make_pair<std::string, TextEditor::PaletteIndex>("L?\\\"(\\\\.|[^\\\"])*\\\"", TextEditor::PaletteIndex::String));
    lang_def.mTokenRegexStrings.push_back(
        std::make_pair<std::string, TextEditor::PaletteIndex>("\\\'[^\\\']*\\\'", TextEditor::PaletteIndex::String));
    lang_def.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>(
        "[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?", TextEditor::PaletteIndex::Number));
    lang_def.mTokenRegexStrings.push_back(
        std::make_pair<std::string, TextEditor::PaletteIndex>("[a-zA-Z_][a-zA-Z0-9_]*", TextEditor::PaletteIndex::Identifier));
    lang_def.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>(
        "[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", TextEditor::PaletteIndex::Punctuation));
    lang_def.mCommentStart      = "\"\"\"";
    lang_def.mCommentEnd        = "\"\"\"";
    lang_def.mSingleLineComment = "#";
    lang_def.mCaseSensitive     = true;
    lang_def.mAutoIndentation   = true;
    lang_def.mName              = "Python";
    initialized                 = true;
  }
  return lang_def;
}
} // namespace

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
// Returns a single multi-line error string (no "[err] " prefix). Clears PyErr.
std::string format_and_clear_python_exception()
{
  PyObject *type = nullptr, *value = nullptr, *traceback = nullptr;
  PyErr_Fetch(&type, &value, &traceback);
  PyErr_NormalizeException(&type, &value, &traceback);
  PyObject*   lines = nullptr;
  std::string text;
  if (value)
  {
    PyObject* tb_mod = PyImport_ImportModule("traceback");
    if (tb_mod)
    {
      PyObject* fmt = PyObject_GetAttrString(tb_mod, "format_exception");
      if (fmt && PyCallable_Check(fmt))
      {
        PyObject* t    = type ? type : Py_None;
        PyObject* v    = value ? value : Py_None;
        PyObject* tb   = traceback ? traceback : Py_None;
        PyObject* args = PyTuple_Pack(3, t, v, tb);
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
      PyObject*   item = PyList_GET_ITEM(lines, i);
      const char* u    = PyUnicode_AsUTF8(item);
      if (u)
        text += u;
    }
  }
  else
  {
    PyObject*   s   = value ? PyObject_Str(value) : nullptr;
    const char* msg = s ? PyUnicode_AsUTF8(s) : "Python error";
    text            = msg ? msg : "?";
    Py_XDECREF(s);
  }

  Py_XDECREF(lines);
  Py_XDECREF(type);
  Py_XDECREF(value);
  Py_XDECREF(traceback);
  PyErr_Clear();
  while (!text.empty() && (text.back() == '\n' || text.back() == '\r'))
    text.pop_back();

  return text;
}

void append_python_exception_to_history(std::vector<std::string>& history, bool* scroll, uint64_t* log_display_version)
{
  const std::string  text = format_and_clear_python_exception();
  std::istringstream iss(text);
  std::string        line;
  bool               any = false;
  while (std::getline(iss, line))
  {
    while (!line.empty() && line.back() == '\r')
      line.pop_back();

    history.push_back("[err] " + line);
    any = true;
  }
  if (!any)
    history.push_back("[err] Python error");

  if (scroll)
    *scroll = true;

  if (log_display_version)
    ++*log_display_version;
}

static Sketch_ref_plane parse_sketch_ref_plane(const std::string& plane)
{
  if (plane == "XZ" || plane == "xz")
    return Sketch_ref_plane::XZ;

  if (plane == "YZ" || plane == "yz")
    return Sketch_ref_plane::YZ;

  return Sketch_ref_plane::XY;
}

static const char* default_sketch_base_name(Sketch_ref_plane plane)
{
  switch (plane)
  {
  case Sketch_ref_plane::XZ:
    return "Sketch_xz";

  case Sketch_ref_plane::YZ:
    return "Sketch_yz";

  default:
    return "Sketch_xy";
  }
}

static const char* k_bootstrap = R"PY(
def _ezycad_bootstrap():
    import ezycad_native as _n

    class Sketch:
        def name(self):
            return _n.view_curr_sketch_name()
        def curr_name(self):
            return self.name()
        def node_count(self):
            return _n.view_curr_sketch_node_count()
        def node(self, i):
            return _n.view_curr_sketch_node(int(i))  # returns (x, y) tuple
        def dim_count(self):
            return _n.view_curr_sketch_dim_count()
        def dim(self, i):
            return _n.view_curr_sketch_dim(int(i))  # (lo, hi, visible, offset, name, distance)
        def add(self, plane="XY", offset=0.0, base_name=None):
            return _n.view_add_sketch(plane, offset, base_name)
        def add_edge(self, x1, y1, x2, y2):
            return _n.view_add_edge(x1, y1, x2, y2)
        def finish_edges(self):
            return _n.view_finish_sketch_edges()

    class View:
        def __init__(self, sketch):
            self.curr_sketch = sketch
        def sketch_count(self):
            return _n.view_sketch_count()
        def shape_count(self):
            return _n.view_shape_count()
        def add_box(self, ox, oy, oz, w, l, h):
            return _n.view_add_box(ox, oy, oz, w, l, h)
        def add_sphere(self, ox, oy, oz, r):
            return _n.view_add_sphere(ox, oy, oz, r)
        def fuse(self, *shapes):
            return _n.view_fuse(*shapes)
        def cut(self, *shapes):
            return _n.view_cut(*shapes)
        def common(self, *shapes):
            return _n.view_common(*shapes)
        def delete(self, *shapes):
            return _n.view_delete(*shapes)
        def get_shape(self, i):
            return _n.view_get_shape(int(i))
        def get_selected(self):
            return _n.view_get_selected()
        def get_selected_indices(self):
            return _n.view_get_selected_indices()
        def get_camera(self):
            return _n.view_get_camera()
        def set_camera(self, ex, ey, ez, cx, cy, cz, ux, uy, uz):
            return _n.view_set_camera(ex, ey, ez, cx, cy, cz, ux, uy, uz)
        # Compatibility aliases -> ezy.sketch / view.curr_sketch
        def add_sketch(self, plane="XY", offset=0.0, base_name=None):
            return self.curr_sketch.add(plane, offset, base_name)
        def add_edge(self, x1, y1, x2, y2):
            return self.curr_sketch.add_edge(x1, y1, x2, y2)
        def finish_sketch_edges(self):
            return self.curr_sketch.finish_edges()

    class Ezy:
        def __init__(self):
            self.sketch = Sketch()
            self.view = View(self.sketch)
            self.Shp = _n.Shp
        def log(self, msg):
            return _n.ezy_log(msg)
        def msg(self, text):
            return _n.ezy_msg(text)
        def get_mode(self):
            return _n.ezy_get_mode()
        def set_mode(self, name):
            return _n.ezy_set_mode(name)
        def save_occt_view_settings(self):
            return _n.ezy_save_occt_view_settings()
        def occt_view_settings_json(self):
            return _n.ezy_occt_view_settings_json()
        def help(self):
            return _n.ezy_help()

    import __main__
    __main__.ezy = Ezy()
    __main__.view = __main__.ezy.view
    __main__.Shp = __main__.ezy.Shp
    __main__.help = lambda: __main__.ezy.help()
    import builtins
    builtins.print = lambda *a: __main__.ezy.log(" ".join(str(x) for x in a))
_ezycad_bootstrap()
del _ezycad_bootstrap
)PY";

} // namespace

PYBIND11_EMBEDDED_MODULE(ezycad_native, m)
{
  py::class_<Ezy_shp>(m, "Shp")
      .def("name", [](const Ezy_shp& s) { return s.shp->get_name(); })
      .def("set_name", [](Ezy_shp& s, const std::string& n) { s.shp->set_name(n); })
      .def("visible", [](const Ezy_shp& s) { return s.shp->get_visible(); })
      .def("set_visible", [](Ezy_shp& s, bool v) { s.shp->set_visible(v); })
      .def("__repr__", [](const Ezy_shp& s) { return "<ezycad.Shp name='" + s.shp->get_name() + "'>"; });

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

  m.def("ezy_get_mode",
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

  m.def("ezy_help",
        []
        {
          if (!g_py_console)
            return;

          const char* help_text = "ezy (public scripting API):\n"
                                  "  ezy.log(msg) / ezy.msg(text) / ezy.help()\n"
                                  "  ezy.get_mode() / ezy.set_mode(name)\n"
                                  "  ezy.save_occt_view_settings() / ezy.occt_view_settings_json()\n"
                                  "ezy.view:\n"
                                  "  sketch_count() / shape_count()\n"
                                  "  add_box(ox,oy,oz,w,l,h) / add_sphere(ox,oy,oz,r)  - returns Shp\n"
                                  "  fuse(s1, s2, ...)  - boolean union of two or more Shp; returns Shp\n"
                                  "  cut(body, tool, ...)  - subtract tools from body; returns Shp\n"
                                  "  common(s1, s2, ...)  - boolean intersection; returns Shp\n"
                                  "  delete(s1, ...)  - remove one or more Shp from the document\n"
                                  "  get_shape(i)  - shape by 0-based index (raises IndexError if out of range)\n"
                                  "  get_selected()  - list of selected document Shp (empty if none)\n"
                                  "  get_selected_indices()  - 0-based indices of selected document shapes\n"
                                  "  get_camera() / set_camera(ex,ey,ez,cx,cy,cz,ux,uy,uz)\n"
                                  "  curr_sketch.name() / node_count() / node(i) / dim_count() / dim(i)\n"
                                  "ezy.sketch: (same object as ezy.view.curr_sketch)\n"
                                  "  name() / node_count() / node(i) / dim_count() / dim(i)\n"
                                  "  add(plane, offset, base_name) / add_edge(x1,y1,x2,y2) / finish_edges()\n"
                                  "ezy.Shp: name / set_name / visible / set_visible\n"
                                  "aliases: view == ezy.view; Shp == ezy.Shp; help() == ezy.help()\n"
                                  "  view.add_sketch / add_edge / finish_sketch_edges -> view.curr_sketch.*";
          g_py_console->append_line_from_python(help_text);
        });

  m.def("ezy_save_occt_view_settings",
        []
        {
          if (g_py_gui)
            g_py_gui->save_occt_view_settings();
        });

  m.def("ezy_occt_view_settings_json",
        []
        {
          if (!g_py_gui)
            return std::string("{}");

          return g_py_gui->occt_view_settings_json();
        });

  m.def("view_sketch_count",
        []
        {
          Occt_view* view = g_py_gui && g_py_gui->get_view() ? g_py_gui->get_view() : nullptr;
          return view ? static_cast<std::size_t>(view->get_sketches().size()) : std::size_t{0};
        });

  m.def("view_shape_count",
        []
        {
          Occt_view* view = g_py_gui && g_py_gui->get_view() ? g_py_gui->get_view() : nullptr;
          return view ? static_cast<std::size_t>(view->get_shapes().size()) : std::size_t{0};
        });

  m.def("view_curr_sketch_name",
        []
        {
          Occt_view* view = g_py_gui && g_py_gui->get_view() ? g_py_gui->get_view() : nullptr;
          return view ? view->curr_sketch().get_name() : std::string{};
        });

  m.def("view_curr_sketch_node_count",
        []
        {
          Occt_view* view = g_py_gui && g_py_gui->get_view() ? g_py_gui->get_view() : nullptr;
          if (!view)
            return std::size_t{0};

          return view->curr_sketch().get_nodes().size();
        });

  m.def(
      "view_curr_sketch_node",
      [](std::ptrdiff_t idx) -> py::tuple
      {
        Occt_view* view = g_py_gui && g_py_gui->get_view() ? g_py_gui->get_view() : nullptr;
        if (!view)
          throw std::runtime_error("no 3D view available");

        if (idx < 0)
          throw py::index_error("node index must be >= 0");

        Sketch_nodes& nodes = view->curr_sketch().get_nodes();
        if (static_cast<std::size_t>(idx) >= nodes.size())
          throw py::index_error("node index out of range");

        const Sketch_nodes::Node& n = nodes[static_cast<std::size_t>(idx)];
        return py::make_tuple(n.X(), n.Y());
      },
      py::arg("i"));

  m.def("view_curr_sketch_dim_count",
        []
        {
          Occt_view* view = g_py_gui && g_py_gui->get_view() ? g_py_gui->get_view() : nullptr;
          if (!view)
            return std::size_t{0};

          return view->curr_sketch().length_dimension_count();
        });

  m.def(
      "view_curr_sketch_dim",
      [](std::ptrdiff_t idx) -> py::tuple
      {
        Occt_view* view = g_py_gui && g_py_gui->get_view() ? g_py_gui->get_view() : nullptr;
        if (!view)
          throw std::runtime_error("no 3D view available");

        if (idx < 0)
          throw py::index_error("dimension index must be >= 0");

        Sketch& sketch = view->curr_sketch();
        if (static_cast<std::size_t>(idx) >= sketch.length_dimension_count())
          throw py::index_error("dimension index out of range");

        const std::size_t   i     = static_cast<std::size_t>(idx);
        const std::size_t   lo    = sketch.dimension_node_lo(i);
        const std::size_t   hi    = sketch.dimension_node_hi(i);
        const Sketch_nodes& nodes = sketch.get_nodes();
        const double        dist  = nodes[lo].Distance(nodes[hi]) / view->get_dimension_scale();
        return py::make_tuple(lo, hi, sketch.dimension_visible(i), sketch.dimension_offset(i), sketch.dimension_name(i), dist);
      },
      py::arg("i"));

  m.def(
      "view_add_sketch",
      [](const std::string& plane, double offset, const py::object& base_name)
      {
        Occt_view* view = g_py_gui && g_py_gui->get_view() ? g_py_gui->get_view() : nullptr;
        if (!view)
          throw std::runtime_error("no 3D view available");

        const Sketch_ref_plane ref = parse_sketch_ref_plane(plane);
        const std::string      base =
            base_name.is_none() ? std::string(default_sketch_base_name(ref)) : base_name.cast<std::string>();
        view->add_sketch_on_ref_plane(ref, offset, base);
      },
      py::arg("plane") = "XY", py::arg("offset") = 0.0, py::arg("base_name") = py::none());

  m.def(
      "view_add_edge",
      [](double x1, double y1, double x2, double y2)
      {
        Occt_view* view = g_py_gui && g_py_gui->get_view() ? g_py_gui->get_view() : nullptr;
        if (!view)
          throw std::runtime_error("no 3D view available");

        view->curr_sketch_add_edge(x1, y1, x2, y2);
      },
      py::arg("x1"), py::arg("y1"), py::arg("x2"), py::arg("y2"));

  m.def("view_finish_sketch_edges",
        []
        {
          Occt_view* view = g_py_gui && g_py_gui->get_view() ? g_py_gui->get_view() : nullptr;
          if (!view)
            throw std::runtime_error("no 3D view available");

          view->curr_sketch_rebuild_faces();
        });

  m.def(
      "view_add_box",
      [](double ox, double oy, double oz, double w, double len, double h) -> Ezy_shp
      {
        Occt_view* view = g_py_gui && g_py_gui->get_view() ? g_py_gui->get_view() : nullptr;
        if (!view)
          throw std::runtime_error("no 3D view available");

        view->add_box(ox, oy, oz, w, len, h);
        std::list<Shp_ptr>& shapes = view->get_shapes();
        if (shapes.empty())
          throw std::runtime_error("add_box failed to create a shape");

        return Ezy_shp{shapes.back()};
      },
      py::arg("ox"), py::arg("oy"), py::arg("oz"), py::arg("w"), py::arg("l"), py::arg("h"));

  m.def(
      "view_add_sphere",
      [](double ox, double oy, double oz, double r) -> Ezy_shp
      {
        Occt_view* view = g_py_gui && g_py_gui->get_view() ? g_py_gui->get_view() : nullptr;
        if (!view)
          throw std::runtime_error("no 3D view available");

        view->add_sphere(ox, oy, oz, r);
        std::list<Shp_ptr>& shapes = view->get_shapes();
        if (shapes.empty())
          throw std::runtime_error("add_sphere failed to create a shape");

        return Ezy_shp{shapes.back()};
      },
      py::arg("ox"), py::arg("oy"), py::arg("oz"), py::arg("r"));

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

        return Ezy_shp{*it};
      },
      py::arg("i"));

  m.def("view_get_selected",
        []() -> py::list
        {
          Occt_view* view = g_py_gui && g_py_gui->get_view() ? g_py_gui->get_view() : nullptr;
          py::list   out;
          if (!view)
            return out;

          for (const Shp_ptr& shp : view->get_selected_shps())
            out.append(Ezy_shp{shp});

          return out;
        });

  m.def("view_get_selected_indices",
        []() -> py::list
        {
          Occt_view* view = g_py_gui && g_py_gui->get_view() ? g_py_gui->get_view() : nullptr;
          py::list   out;
          if (!view)
            return out;

          const std::vector<Shp_ptr> selected = view->get_selected_shps();
          std::list<Shp_ptr>&        shapes   = view->get_shapes();
          for (const Shp_ptr& sel : selected)
          {
            std::ptrdiff_t i = 0;
            for (const Shp_ptr& shp : shapes)
            {
              if (shp == sel)
              {
                out.append(i);
                break;
              }
              ++i;
            }
          }
          return out;
        });

  m.def("view_fuse",
        [](py::args args) -> Ezy_shp
        {
          Occt_view* view = g_py_gui && g_py_gui->get_view() ? g_py_gui->get_view() : nullptr;
          if (!view)
            throw std::runtime_error("no 3D view available");

          if (args.size() < 2)
            throw std::runtime_error("fuse requires two or more shapes");

          std::vector<Shp_ptr> shps;
          shps.reserve(static_cast<std::size_t>(args.size()));
          for (py::handle item : args)
          {
            Ezy_shp es = item.cast<Ezy_shp>();
            if (es.shp.IsNull())
              throw std::runtime_error("fuse: null shape");

            shps.push_back(es.shp);
          }
          Shp_rslt r = view->shp_fuse().fuse(std::move(shps));
          if (!r.is_ok())
            throw std::runtime_error(r.message().empty() ? "fuse failed" : r.message());

          return Ezy_shp{r.value()};
        });

  m.def("view_cut",
        [](py::args args) -> Ezy_shp
        {
          Occt_view* view = g_py_gui && g_py_gui->get_view() ? g_py_gui->get_view() : nullptr;
          if (!view)
            throw std::runtime_error("no 3D view available");

          if (args.size() < 2)
            throw std::runtime_error("cut requires two or more shapes");

          std::vector<Shp_ptr> shps;
          shps.reserve(static_cast<std::size_t>(args.size()));
          for (py::handle item : args)
          {
            Ezy_shp es = item.cast<Ezy_shp>();
            if (es.shp.IsNull())
              throw std::runtime_error("cut: null shape");

            shps.push_back(es.shp);
          }
          Shp_rslt r = view->shp_cut().cut(std::move(shps));
          if (!r.is_ok())
            throw std::runtime_error(r.message().empty() ? "cut failed" : r.message());

          return Ezy_shp{r.value()};
        });

  m.def("view_common",
        [](py::args args) -> Ezy_shp
        {
          Occt_view* view = g_py_gui && g_py_gui->get_view() ? g_py_gui->get_view() : nullptr;
          if (!view)
            throw std::runtime_error("no 3D view available");

          if (args.size() < 2)
            throw std::runtime_error("common requires two or more shapes");

          std::vector<Shp_ptr> shps;
          shps.reserve(static_cast<std::size_t>(args.size()));
          for (py::handle item : args)
          {
            Ezy_shp es = item.cast<Ezy_shp>();
            if (es.shp.IsNull())
              throw std::runtime_error("common: null shape");

            shps.push_back(es.shp);
          }
          Shp_rslt r = view->shp_common().common(std::move(shps));
          if (!r.is_ok())
            throw std::runtime_error(r.message().empty() ? "common failed" : r.message());

          return Ezy_shp{r.value()};
        });

  m.def("view_delete",
        [](py::args args)
        {
          Occt_view* view = g_py_gui && g_py_gui->get_view() ? g_py_gui->get_view() : nullptr;
          if (!view)
            throw std::runtime_error("no 3D view available");

          if (args.size() < 1)
            throw std::runtime_error("delete requires one or more shapes");

          std::vector<AIS_Shape_ptr> to_delete;
          to_delete.reserve(static_cast<std::size_t>(args.size()));
          for (py::handle item : args)
          {
            Ezy_shp es = item.cast<Ezy_shp>();
            if (es.shp.IsNull())
              throw std::runtime_error("delete: null shape");

            to_delete.push_back(es.shp);
          }
          view->delete_shapes(std::move(to_delete));
        });

  m.def("view_get_camera",
        []() -> py::dict
        {
          Occt_view* view = g_py_gui && g_py_gui->get_view() ? g_py_gui->get_view() : nullptr;
          if (!view)
            throw std::runtime_error("no 3D view available");

          gp_Pnt eye;
          gp_Pnt center;
          gp_Dir up;
          if (!view->get_camera(eye, center, up))
            throw std::runtime_error("camera is not available");

          py::dict out;
          out["eye"]    = py::make_tuple(eye.X(), eye.Y(), eye.Z());
          out["center"] = py::make_tuple(center.X(), center.Y(), center.Z());
          out["up"]     = py::make_tuple(up.X(), up.Y(), up.Z());
          return out;
        });

  m.def(
      "view_set_camera",
      [](double ex, double ey, double ez, double cx, double cy, double cz, double ux, double uy, double uz)
      {
        Occt_view* view = g_py_gui && g_py_gui->get_view() ? g_py_gui->get_view() : nullptr;
        if (!view)
          throw std::runtime_error("no 3D view available");

        view->set_camera(gp_Pnt(ex, ey, ez), gp_Pnt(cx, cy, cz), gp_Dir(ux, uy, uz));
      },
      py::arg("ex"), py::arg("ey"), py::arg("ez"), py::arg("cx"), py::arg("cy"), py::arg("cz"), py::arg("ux"), py::arg("uy"),
      py::arg("uz"));
}

struct Python_console::Ezycad_python_runtime
{
  py::scoped_interpreter guard;
};

#endif // EZYCAD_HAVE_PYTHON

void Python_console::append_line_from_python(const std::string& line)
{
  append_line(line, false);
  if (m_capturing)
  {
    if (!m_capture_buf.empty())
      m_capture_buf.push_back('\n');

    m_capture_buf += line;
  }
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
    append_line(
        "Python console ready (pybind11). Try: ezy.log('hello'), ezy.view.sketch_count(), view.curr_sketch.node_count()");
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

    std::string   content;
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
    entry.editor.SetLanguageDefinition(python_language_definition());
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

Python_exec_result Python_console::execute_captured(const std::string& code)
{
  Python_exec_result r;
  if (!m_python_ok)
  {
    r.ok    = false;
    r.error = "Python interpreter is not ready";
    return r;
  }

  if (code.empty())
  {
    r.ok    = false;
    r.error = "empty code";
    return r;
  }

  if (m_cmd_history.empty() || m_cmd_history.back() != code)
    m_cmd_history.push_back(code);

  m_cmd_history_pos = -1;

  append_line("> " + code);

  m_capturing = true;
  m_capture_buf.clear();

  try
  {
    py::object result;
    // Prefer expression eval so remote clients get a result repr (e.g. view.sketch_count()).
    // Fall back to single-statement (console REPL) when the source is not an expression.
    try
    {
      result = py::eval(py::str(code), py::globals(), py::globals());
    }
    catch (py::error_already_set& e)
    {
      if (!e.matches(PyExc_SyntaxError))
        throw;

      e.restore();
      PyErr_Clear();
      result = py::eval<py::eval_single_statement>(py::str(code), py::globals(), py::globals());
    }
    if (result.ptr() != nullptr && !result.is_none())
    {
      r.result = py::repr(result).cast<std::string>();
      append_line(r.result);
    }
  }
  catch (py::error_already_set& e)
  {
    e.restore();
    r.ok    = false;
    r.error = format_and_clear_python_exception();
    {
      std::istringstream iss(r.error);
      std::string        line;
      bool               any = false;
      while (std::getline(iss, line))
      {
        while (!line.empty() && line.back() == '\r')
          line.pop_back();

        m_history.push_back("[err] " + line);
        any = true;
      }
      if (!any)
        m_history.push_back("[err] Python error");

      m_scroll_to_bottom = true;
      ++m_log_display_version;
    }
  }

  m_capturing = false;
  r.output    = m_capture_buf;
  m_capture_buf.clear();
  return r;
}

void Python_console::execute(const std::string& code) { (void)execute_captured(code); }
#else
void Python_console::load_scripts() {}

void Python_console::execute(const std::string& code) { (void)code; }

Python_exec_result Python_console::execute_captured(const std::string& code)
{
  (void)code;
  Python_exec_result r;
  r.ok    = false;
  r.error = "Python console is not available in this build";
  return r;
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
      ImGui::InputTextMultiline("##python_log", m_log_display_buf.data(), m_log_display_buf.size(), ImVec2(-1.f, -1.f),
                                ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_CallbackResize |
                                    ImGuiInputTextFlags_NoUndoRedo,
                                &Python_console::log_display_resize_callback, this);
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
    bool run = ImGui::InputTextWithHint(
        "##python_input", "Enter Python code (e.g. ezy.log('hi'))", m_input_buf, k_input_buf_size,
        ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory, &Python_console::text_edit_callback, this);
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
      std::ofstream     of(script.path);
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
