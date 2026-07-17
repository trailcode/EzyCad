#pragma once

#include "gui.h"
#include "gui_occt_view.h"
#include "skt.h"
#include "skt_op_recorder.h"
#include "utl_geom.h"

#include <gtest/gtest.h>

#include <list>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <gp_Dir2d.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>

class GUI_access
{
public:
  static void        set_view(GUI& gui, std::unique_ptr<Occt_view>& view);
  static Occt_view&  get_view(GUI& gui);
  static std::string get_message(const GUI& gui);
  static void        sketch_left_click(GUI& gui, const ScreenCoords& screen_coords);
  static void        mirror_selected_edges(GUI& gui);
};

class Sketch_access
{
public:
  static void add_edge_(Sketch& sketch, const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b);
  static void add_edge_(Sketch& sketch, const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, Sketch_op_recorder& rec);
  static void add_edge_raw_(Sketch& sketch, const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b);
  static std::vector<ezy_geom::polygon_2d>    update_faces_(Sketch& sketch);
  static std::vector<ezy_geom::linestring_2d> dbg_edge_linestrings(const Sketch& sketch);
  static void add_arc_circle_(Sketch& sketch, const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, const gp_Pnt2d& pt_c);
  static void add_arc_circle_(Sketch& sketch, const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, const gp_Pnt2d& pt_c,
                              Sketch_op_recorder& rec);
  static void get_originating_face_snp_pts_3d_(Sketch& sketch, std::vector<gp_Pnt>& out);

  static const std::vector<Sketch_face_shp_ptr>& get_faces(const Sketch& sketch);
  static const std::list<Sketch::Edge>&          get_edges(const Sketch& sketch);
  static size_t                                  get_edge_count(const Sketch& sketch);
  static size_t                                  get_linear_edge_count(const Sketch& sketch);
  static size_t                                  get_arc_internal_edge_count(const Sketch& sketch);
  static size_t                                  count_permanent_nodes(Sketch& sketch);
  static size_t                                  length_dimension_count(const Sketch& sketch);
  static size_t                                  length_dimension_node_lo(const Sketch& sketch, size_t index);
  static size_t                                  length_dimension_node_hi(const Sketch& sketch, size_t index);
  static void                                    set_entered_edge_len(Sketch& sketch, const gp_Dir2d& dir, double len);

  /// Exact replay of a saved linear edge (with its pre-existing midpoint node index).
  /// Used for regression tests of specific .ezy cases (e.g. bridge + hole topologies).
  static void sketch_json_add_linear_edge_(Sketch& sketch, size_t idx_a, size_t idx_b,
                                           std::optional<size_t> idx_mid = std::nullopt);
  static void set_operation_axis_(Sketch& sketch, const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b);
  static void remove_permanent_node_mark_(Sketch& sketch, size_t node_idx);
};

class View_access
{
public:
  static void set_view_plane(Occt_view& view, const gp_Pln& pln);
  static void set_headless(Occt_view& view, bool headless);
};

class Shp_extrude_access
{
public:
  static void set_curr_view_pln(Shp_extrude& extrude, const gp_Pln& pln);
  /// Start an extrude preview from a known face (bypasses AIS screen picking).
  static void begin_face_extrude(Shp_extrude& extrude, const AIS_Shape_ptr& face, double extrude_dist);
};

struct Headless_guard
{
  Occt_view& m_v;

  explicit Headless_guard(Occt_view& v)
      : m_v(v)
  {
    View_access::set_headless(m_v, true);
  }

  ~Headless_guard() { View_access::set_headless(m_v, false); }
};

// Test fixture for Sketch tests
class Sketch_test : public ::testing::Test
{
protected:
  void SetUp() override;
  void TearDown() override;

  Occt_view& view();
  GUI&       gui();

  // Helper to add all permutations of edges (with both orientations) to a sketch and call a lambda
  template <typename Callback>
  void for_all_edge_permutations_(const std::vector<gp_Pnt2d>& points, const std::vector<std::pair<int, int>>& edge_indices,
                                  const gp_Pln& plane, Callback&& callback);

  // Helper to add edges to a sketch from a vector of points and edge index pairs
  template <typename Callback>
  void add_edges_from_indices_(const std::vector<gp_Pnt2d>& points, const std::vector<std::pair<int, int>>& edge_indices,
                               const gp_Pln& plane, Callback&& callback);

  static GUI s_gui;
};

#include "skt_test_fixture.inl"
