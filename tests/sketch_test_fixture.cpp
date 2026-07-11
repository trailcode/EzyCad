#include "sketch_test_fixture.h"

#include <utility>

#include "sketch_edge.h"

GUI Sketch_test::s_gui;

void Sketch_test::SetUp()
{
  // TODO it is creating a view each time, create only one view and have a reset method.
  std::unique_ptr<Occt_view> view = std::make_unique<Occt_view>(s_gui);
  view->init_viewer();
  view->init_default();
  GUI_access::set_view(s_gui, view);

  // Force midpoint creation on for tests (they were written assuming auto mids on linear edges).
  // Production default is off (see user's request); specific tests can override with set_... (false).
  Sketch::set_add_mid_pt_edges(true);
}

void Sketch_test::TearDown()
{
  // view.reset();
}

Occt_view& Sketch_test::view() { return GUI_access::get_view(s_gui); }

GUI& Sketch_test::gui() { return s_gui; }

void GUI_access::set_view(GUI& gui, std::unique_ptr<Occt_view>& view) { gui.m_view = std::move(view); }

Occt_view& GUI_access::get_view(GUI& gui) { return *gui.m_view; }

std::string GUI_access::get_message(const GUI& gui) { return gui.m_message; }

void GUI_access::sketch_left_click(GUI& gui, const ScreenCoords& screen_coords) { gui.sketch_left_click(screen_coords); }

void GUI_access::mirror_selected_edges(GUI& gui) { gui.mirror_selected_sketch_edges(); }

void Sketch_access::add_edge_(Sketch& sketch, const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b) { sketch.add_edge_(pt_a, pt_b); }

void Sketch_access::add_edge_(Sketch& sketch, const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, Sketch_op_recorder& rec)
{
  sketch.add_edge_(pt_a, pt_b, rec);
}

void Sketch_access::add_edge_raw_(Sketch& sketch, const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b)
{
  sketch.add_edge_raw_(pt_a, pt_b);
}

std::vector<ezy_geom::polygon_2d> Sketch_access::update_faces_(Sketch& sketch)
{
  sketch.update_faces_();

  std::vector<ezy_geom::polygon_2d> polys;
  const auto&                      faces = Sketch_access::get_faces(sketch);
  polys.reserve(faces.size());
  for (const auto& face : faces)
    polys.push_back(to_boost(face->Shape(), sketch.get_plane()));

  return polys;
}

std::vector<ezy_geom::linestring_2d> Sketch_access::dbg_edge_linestrings(const Sketch& sketch)
{
  std::vector<ezy_geom::linestring_2d> out;
  for (const auto& e : Sketch_access::get_edges(sketch))
  {
    if (e.shp.IsNull())
      continue;

    out.push_back(to_boost_ls(e.shp->Shape(), sketch.get_plane()));
  }

  return out;
}

void Sketch_access::add_arc_circle_(Sketch& sketch, const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, const gp_Pnt2d& pt_c)
{
  sketch.add_arc_circle_(pt_a, pt_b, pt_c);
}

void Sketch_access::add_arc_circle_(Sketch& sketch, const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, const gp_Pnt2d& pt_c,
                                    Sketch_op_recorder& rec)
{
  sketch.add_arc_circle_(pt_a, pt_b, pt_c, rec);
}

void Sketch_access::get_originating_face_snp_pts_3d_(Sketch& sketch, std::vector<gp_Pnt>& out)
{
  sketch.get_originating_face_snp_pts_3d_(out);
}

const std::vector<Sketch_face_shp_ptr>& Sketch_access::get_faces(const Sketch& sketch) { return sketch.m_topo.faces(); }

const std::list<Sketch::Edge>& Sketch_access::get_edges(const Sketch& sketch) { return sketch.m_edges.edges(); }

size_t Sketch_access::get_edge_count(const Sketch& sketch)
{
  size_t count = 0;
  for (const auto& edge : Sketch_access::get_edges(sketch))
    if (edge.node_idx_b.has_value())
      ++count;

  return count;
}

size_t Sketch_access::get_linear_edge_count(const Sketch& sketch)
{
  size_t count = 0;
  for (const auto& edge : Sketch_access::get_edges(sketch))
    if (sketch_edge_is_linear(edge))
      ++count;

  return count;
}

size_t Sketch_access::get_arc_internal_edge_count(const Sketch& sketch)
{
  size_t count = 0;
  for (const auto& edge : Sketch_access::get_edges(sketch))
    if (sketch_edge_is_arc(edge))
      ++count;

  return count;
}

size_t Sketch_access::count_permanent_nodes(Sketch& sketch)
{
  size_t n = 0;
  for (const auto& node : sketch.get_nodes())
    if (node.permanent && !node.deleted)
      ++n;

  return n;
}

size_t Sketch_access::length_dimension_count(const Sketch& sketch) { return sketch.m_dims.dimensions().size(); }

size_t Sketch_access::length_dimension_node_lo(const Sketch& sketch, size_t index)
{
  return sketch.m_dims.dimensions()[index].node_idx_lo;
}

size_t Sketch_access::length_dimension_node_hi(const Sketch& sketch, size_t index)
{
  return sketch.m_dims.dimensions()[index].node_idx_hi;
}

void Sketch_access::set_entered_edge_len(Sketch& sketch, const gp_Dir2d& dir, double len)
{
  sketch.m_dims.entered_edge_len() = Sketch_dims::Edge_len{dir, len};
}

void Sketch_access::sketch_json_add_linear_edge_(Sketch& sketch, size_t idx_a, size_t idx_b, std::optional<size_t> idx_mid)
{
  sketch.sketch_json_add_linear_edge_(idx_a, idx_b, idx_mid);
}

void Sketch_access::set_operation_axis_(Sketch& sketch, const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b)
{
  sketch.sketch_json_set_operation_axis_(pt_a, pt_b);
}

void Sketch_access::remove_permanent_node_mark_(Sketch& sketch, size_t node_idx)
{
  Sketch_AIS_node_mark mark(sketch, node_idx, TopoDS_Shape());
  sketch.remove_permanent_node_mark(mark);
}

void View_access::set_view_plane(Occt_view& view, const gp_Pln& pln) { view.shp_extrude().set_curr_view_pln(pln); }

void View_access::set_headless(Occt_view& view, bool headless) { view.m_headless_view = headless; }
