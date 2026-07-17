#include "skt_nodes.h"

#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <Precision.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Wire.hxx>
#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

#include "utl_dbg.h"
#include "utl_geom.h"
#include "imgui.h"
#include "gui_occt_view.h"

namespace
{
double                        s_snap_dist_pixels = 35.0;
Sketch_nodes::Snap_guide_mode s_snap_guide_mode  = Sketch_nodes::Snap_guide_mode::Both;
glm::vec3                     s_snap_guide_color_node{0.823295f, 0.549411f, 0.953390f};
glm::vec3                     s_snap_guide_color_axis{0.957627f, 0.064924f, 0.541537f};
float                         s_snap_guide_line_width      = 1.0f;
bool                          s_annotate_all_coaxial_nodes = true;

static Quantity_Color snap_guide_qc(const glm::vec3& c) { return Quantity_Color(c.x, c.y, c.z, Quantity_TOC_RGB); }

static void prepare_snap_ais_(AIS_InteractiveContext& ctx, const AIS_Shape_ptr& ais)
{
  if (ais.IsNull())
    return;

  ctx.Unhilight(ais, false);
  ctx.Deactivate(ais);
}

static void update_snap_ais_shape_(AIS_InteractiveContext& ctx, AIS_Shape_ptr& ais, const TopoDS_Shape& shape,
                                   const glm::vec3& color)
{
  if (shape.IsNull())
  {
    if (!ais.IsNull())
    {
      ctx.Remove(ais, false);
      ais.Nullify();
    }

    return;
  }

  const Quantity_Color qc = snap_guide_qc(color);
  if (ais.IsNull())
  {
    ais = new AIS_Shape(shape);
    ais->SetWidth(s_snap_guide_line_width);
    ais->SetColor(qc);
    ctx.Display(ais, false);
    prepare_snap_ais_(ctx, ais);
  }
  else
  {
    prepare_snap_ais_(ctx, ais);
    ais->Set(shape);
    ais->SetWidth(s_snap_guide_line_width);
    ais->SetColor(qc);
    ctx.Redisplay(ais, false);
    prepare_snap_ais_(ctx, ais);
  }
}

static void clear_snap_ais_(AIS_InteractiveContext& ctx, AIS_Shape_ptr& ais)
{
  if (!ais.IsNull())
  {
    ctx.Remove(ais, false);
    ais.Nullify();
  }
}
} // namespace

class Sketch_nodes::Impl
{
public:
  Impl(Sketch_nodes* owner, Occt_view& view, const gp_Pln& pln)
      : m_owner(owner)
      , m_view(view)
      , m_ctx(view.ctx())
      , m_pln(pln)
  {
  }

  std::optional<gp_Pnt2d> snap(const ScreenCoords& screen_coords);
  size_t                  get_node_exact(const gp_Pnt2d& pt, bool permanent_for_new);
  std::optional<size_t>   get_node(const ScreenCoords& screen_coords);
  std::optional<size_t>   try_pick_existing_node(const ScreenCoords& screen_coords);
  std::optional<size_t>   try_get_node_idx_snap(gp_Pnt2d& pt, const std::vector<size_t>& to_exclude = {});
  void                    get_snap_pts_3d(std::vector<gp_Pnt>& out);
  void                    hide_snap_annos();
  size_t                  add_new_node(const gp_Pnt2d& pt, bool is_edge_mid_point = false, bool is_permanent = false);

  std::vector<Node>::iterator       begin();
  std::vector<Node>::iterator       end();
  std::vector<Node>::const_iterator begin() const;
  std::vector<Node>::const_iterator end() const;
  std::vector<Node>::const_iterator cbegin() const;
  std::vector<Node>::const_iterator cend() const;

  Node&       node_at(size_t idx);
  const Node& node_at(size_t idx) const;
  bool        empty() const;
  size_t      size() const;

  void resize(size_t count);
  void set_node(size_t idx, const gp_Pnt2d& pt, bool deleted, bool midpoint, bool permanent, bool origin,
                const std::string& name);
  void restore_node_at(size_t idx, const gp_Pnt2d& pt, bool deleted, bool midpoint, bool permanent, bool origin,
                       const std::string& name);

  void finalize();
  void cancel();

  void clear_outside_snap_pnts();
  void add_outside_snap_pnt(const gp_Pnt& pt3d);

  void               set_origin_snap_enabled(bool enabled) { m_origin_snap_enabled = enabled; }
  [[nodiscard]] bool origin_snap_enabled() const { return m_origin_snap_enabled; }

private:
  Sketch_nodes*           m_owner;
  std::vector<Node>       m_nodes;
  std::set<gp_Pnt2d>      m_outside_snap_pts; // Projected snap points from other sketches.
  AIS_Shape_ptr           m_snap_anno_axis[2];
  AIS_Shape_ptr           m_snap_anno_axis_fs[2];
  std::optional<gp_Pnt2d> m_last_snap_pt; // Used for snap annotation
  AIS_Shape_ptr           m_snap_anno;
  AIS_Shape_ptr           m_snap_anno_fs[2];
  AIS_Shape_ptr           m_global_coax_fs_v;
  AIS_Shape_ptr           m_global_coax_fs_h;
  AIS_Shape_ptr           m_global_coax_markers;
  size_t                  m_prev_num_nodes{0}; // Used when an operation is canceled.

  // Owner related
  Occt_view&              m_view;
  AIS_InteractiveContext& m_ctx;
  const gp_Pln            m_pln;
  bool                    m_origin_snap_enabled{true};

  [[nodiscard]] bool node_snap_eligible_(const Node& n) const
  {
    if (n.deleted)
      return false;
    if (n.origin && !m_origin_snap_enabled)
      return false;
    return true;
  }

  /// World-space snap radius at `pt` (same convention as `try_get_node_idx_snap` / `try_pick_existing_node`).
  double snap_radius_world_(const gp_Pnt2d& pt) const;
  bool   view_bounds_2d_(double& min_u, double& min_v, double& max_u, double& max_v) const;
  void   update_node_snap_anno_(const gp_Pnt2d& pt, const double snap_dist);
  void   update_axis_snap_anno_(int axis_index, const std::vector<gp_Pnt2d>& axis_pts, double snap_dist);
  void   update_global_coaxial_annotations_(double snap_dist);
};

// === Impl helpers ==========================================================

double Sketch_nodes::Impl::snap_radius_world_(const gp_Pnt2d& pt) const
{
  if (m_view.is_headless())
    return s_snap_dist_pixels;

  gp_Pnt       pt3d_on_plane       = to_3d(m_pln, pt);
  ScreenCoords screen_coords_at_pt = m_view.get_screen_coords(pt3d_on_plane);

  ScreenCoords screen_coords_offset = screen_coords_at_pt;
  screen_coords_offset.unsafe_get().x += s_snap_dist_pixels;

  std::optional<gp_Pnt2d> pt_offset_on_plane_2d;
  if (std::optional<gp_Pnt> pt_offset_on_plane_3d = m_view.pt3d_on_plane(screen_coords_offset, m_pln))
    pt_offset_on_plane_2d = to_2d(m_pln, *pt_offset_on_plane_3d);

  if (pt_offset_on_plane_2d)
    return pt.Distance(*pt_offset_on_plane_2d);

  return 5.0;
}

bool Sketch_nodes::Impl::view_bounds_2d_(double& min_u, double& min_v, double& max_u, double& max_v) const
{
  if (m_view.is_headless())
    return false;

  const ImGuiIO& io = ImGui::GetIO();
  return m_view.sketch_plane_view_aabb_2d(m_pln, static_cast<double>(io.DisplaySize.x), static_cast<double>(io.DisplaySize.y),
                                          min_u, min_v, max_u, max_v);
}

std::optional<size_t> Sketch_nodes::Impl::try_pick_existing_node(const ScreenCoords& screen_coords)
{
  if (m_view.sketch_snap_suppressed())
  {
    m_owner->hide_snap_annos();
    return std::nullopt;
  }

  std::optional<gp_Pnt2d> pt_opt = m_view.pt_on_plane(screen_coords, m_pln);
  if (!pt_opt)
  {
    m_owner->hide_snap_annos();
    return std::nullopt;
  }

  const gp_Pnt2d pt        = *pt_opt;
  const double   snap_dist = snap_radius_world_(pt);
  size_t         best_idx  = static_cast<size_t>(-1);
  double         best_sq   = std::numeric_limits<double>::max();
  for (size_t idx = 0, num = m_nodes.size(); idx < num; ++idx)
  {
    if (!node_snap_eligible_(m_nodes[idx]))
      continue;

    const double sq = m_nodes[idx].SquareDistance(pt);
    if (sq < best_sq)
    {
      best_sq  = sq;
      best_idx = idx;
    }
  }

  if (best_idx == static_cast<size_t>(-1))
  {
    m_owner->hide_snap_annos();
    return std::nullopt;
  }

  if (best_sq <= snap_dist * 0.25 * snap_dist)
  {
    // `try_get_node_idx_snap` can modify the input `pt`, we call this function to display snapping annotations.
    gp_Pnt2d pt_snapped = m_nodes[best_idx];
    m_owner->try_get_node_idx_snap(pt_snapped, {});
    return best_idx;
  }

  m_owner->hide_snap_annos();
  return std::nullopt;
}

std::optional<gp_Pnt2d> Sketch_nodes::Impl::snap(const ScreenCoords& screen_coords)
{
  std::optional<gp_Pnt2d> pt = m_view.pt_on_plane(screen_coords, m_pln);
  if (pt && !m_view.sketch_snap_suppressed())
    m_owner->try_get_node_idx_snap(*pt);

  return pt;
}

size_t Sketch_nodes::Impl::get_node_exact(const gp_Pnt2d& pt, bool permanent_for_new)
{
  std::optional<size_t> deleted_match;
  for (size_t idx = 0, num = m_nodes.size(); idx < num; ++idx)
    if (equal(pt, gp_Pnt2d(m_nodes[idx])))
    {
      Node& n = m_nodes[idx];
      // Never bind to tombstoned nodes while searching for an exact live match.
      if (n.deleted)
      {
        if (!deleted_match.has_value())
          deleted_match = idx;

        continue;
      }
      // If caller requests permanence (e.g. add-node tool), preserve/promote it.
      if (permanent_for_new)
        n.permanent = true;

      return idx;
    }

  // If only a deleted exact match exists, revive it instead of appending a duplicate index.
  if (deleted_match.has_value())
  {
    Node& n   = m_nodes[*deleted_match];
    n.deleted = false;
    if (permanent_for_new)
      n.permanent = true;

    return *deleted_match;
  }

  Node n(pt);
  n.permanent      = permanent_for_new;
  const size_t ret = m_nodes.size();
  m_nodes.push_back(n);
  return ret;
}

std::optional<size_t> Sketch_nodes::Impl::get_node(const ScreenCoords& screen_coords)
{
  std::optional<gp_Pnt2d> pt = m_view.pt_on_plane(screen_coords, m_pln);
  if (!pt)
    // View plane and sketch plane must be perpendicular.
    return std::nullopt;

  std::optional<size_t> idx = m_owner->try_get_node_idx_snap(*pt);
  if (idx.has_value())
    return *idx;

  return m_owner->add_new_node(*pt);
}

void Sketch_nodes::Impl::get_snap_pts_3d(std::vector<gp_Pnt>& out)
{
  for (const Node& n : m_nodes)
    if (node_snap_eligible_(n))
      out.push_back(to_3d(m_pln, n));
}

void Sketch_nodes::Impl::hide_snap_annos()
{
  clear_snap_ais_(m_ctx, m_snap_anno);

  for (AIS_Shape_ptr& anno : m_snap_anno_fs)
    clear_snap_ais_(m_ctx, anno);

  for (AIS_Shape_ptr& anno : m_snap_anno_axis)
    clear_snap_ais_(m_ctx, anno);

  for (AIS_Shape_ptr& anno : m_snap_anno_axis_fs)
    clear_snap_ais_(m_ctx, anno);

  clear_snap_ais_(m_ctx, m_global_coax_fs_v);
  clear_snap_ais_(m_ctx, m_global_coax_fs_h);
  clear_snap_ais_(m_ctx, m_global_coax_markers);

  m_ctx.UpdateCurrentViewer();
  m_last_snap_pt = std::nullopt;
}

size_t Sketch_nodes::Impl::add_new_node(const gp_Pnt2d& pt, bool is_edge_mid_point, bool is_permanent)
{
  size_t ret = m_nodes.size();
  Node   n(pt);
  n.midpoint  = is_edge_mid_point;
  n.permanent = is_permanent;
  m_nodes.emplace_back(n);
  return ret;
}

void Sketch_nodes::Impl::update_node_snap_anno_(const gp_Pnt2d& pt, const double snap_dist)
{
  if (m_last_snap_pt && equal(*m_last_snap_pt, pt))
    return;

  m_last_snap_pt = pt;

  const auto mode = s_snap_guide_mode;
  const bool show_traditional =
      mode == Sketch_nodes::Snap_guide_mode::Traditional || mode == Sketch_nodes::Snap_guide_mode::Both;
  const bool show_fullscreen = mode == Sketch_nodes::Snap_guide_mode::Fullscreen || mode == Sketch_nodes::Snap_guide_mode::Both;

  TopoDS_Shape fs_h;
  TopoDS_Shape fs_v;
  if (show_fullscreen)
  {
    double min_u{}, min_v{}, max_u{}, max_v{};
    if (view_bounds_2d_(min_u, min_v, max_u, max_v))
    {
      const gp_Pnt p_h0 = to_3d(m_pln, gp_Pnt2d(min_u, pt.Y()));
      const gp_Pnt p_h1 = to_3d(m_pln, gp_Pnt2d(max_u, pt.Y()));
      const gp_Pnt p_v0 = to_3d(m_pln, gp_Pnt2d(pt.X(), min_v));
      const gp_Pnt p_v1 = to_3d(m_pln, gp_Pnt2d(pt.X(), max_v));
      fs_h              = BRepBuilderAPI_MakeEdge(p_h0, p_h1).Edge();
      fs_v              = BRepBuilderAPI_MakeEdge(p_v0, p_v1).Edge();
    }
  }

  const TopoDS_Shape traditional_shape = create_wire_box(m_pln, to_3d(m_pln, pt), snap_dist, snap_dist);
  update_snap_ais_shape_(m_ctx, m_snap_anno, show_traditional ? traditional_shape : TopoDS_Shape(), s_snap_guide_color_node);
  update_snap_ais_shape_(m_ctx, m_snap_anno_fs[0], fs_v, s_snap_guide_color_node);
  update_snap_ais_shape_(m_ctx, m_snap_anno_fs[1], fs_h, s_snap_guide_color_node);
}

void Sketch_nodes::Impl::update_axis_snap_anno_(int axis_index, const std::vector<gp_Pnt2d>& axis_pts, double snap_dist)
{
  if (axis_pts.empty())
    return;

  const auto mode = s_snap_guide_mode;
  const bool show_traditional =
      mode == Sketch_nodes::Snap_guide_mode::Traditional || mode == Sketch_nodes::Snap_guide_mode::Both;
  const bool show_fullscreen = mode == Sketch_nodes::Snap_guide_mode::Fullscreen || mode == Sketch_nodes::Snap_guide_mode::Both;

  // Use the first point as representative for the fullscreen guide line (all should share the axis coord)
  const gp_Pnt2d& rep_pt = axis_pts.front();

  TopoDS_Shape fullscreen_shape;
  if (show_fullscreen)
  {
    double min_u{}, min_v{}, max_u{}, max_v{};
    if (view_bounds_2d_(min_u, min_v, max_u, max_v))
    {
      if (axis_index == 0)
      {
        const gp_Pnt p0  = to_3d(m_pln, gp_Pnt2d(rep_pt.X(), min_v));
        const gp_Pnt p1  = to_3d(m_pln, gp_Pnt2d(rep_pt.X(), max_v));
        fullscreen_shape = BRepBuilderAPI_MakeEdge(p0, p1).Edge();
      }
      else
      {
        const gp_Pnt p0  = to_3d(m_pln, gp_Pnt2d(min_u, rep_pt.Y()));
        const gp_Pnt p1  = to_3d(m_pln, gp_Pnt2d(max_u, rep_pt.Y()));
        fullscreen_shape = BRepBuilderAPI_MakeEdge(p0, p1).Edge();
      }
    }
  }

  TopoDS_Shape marker_shape;
  if (show_traditional)
  {
    BRep_Builder    builder;
    TopoDS_Compound comp;
    builder.MakeCompound(comp);
    for (const gp_Pnt2d& p : axis_pts)
    {
      const TopoDS_Shape small = create_wire_box(m_pln, to_3d(m_pln, p), snap_dist, snap_dist);
      builder.Add(comp, small);
    }
    marker_shape = comp;
  }

  update_snap_ais_shape_(m_ctx, m_snap_anno_axis_fs[axis_index], fullscreen_shape, s_snap_guide_color_axis);
  update_snap_ais_shape_(m_ctx, m_snap_anno_axis[axis_index], marker_shape, s_snap_guide_color_axis);
}

void Sketch_nodes::Impl::update_global_coaxial_annotations_(double snap_dist)
{
  // Collect all current nodes + outside points (from other visible sketches)
  std::vector<gp_Pnt2d> all_pts;
  for (const auto& n : m_nodes)
    if (!n.deleted)
      all_pts.push_back(n);

  for (const auto& p : m_outside_snap_pts)
    all_pts.push_back(p);

  if (all_pts.empty())
    return;

  // Collect all X and Y values
  std::vector<double> all_xs, all_ys;
  for (const auto& p : all_pts)
  {
    all_xs.push_back(p.X());
    all_ys.push_back(p.Y());
  }

  // Canonicalize unique values within Precision::Confusion() (as per requirement for co-axial alignments)
  auto canonicalize = [](std::vector<double>& vals)
  {
    if (vals.empty())
      return;

    std::sort(vals.begin(), vals.end());
    std::vector<double> unique;
    double              tol = Precision::Confusion();
    for (double v : vals)
      if (unique.empty() || std::fabs(v - unique.back()) > tol)
        unique.push_back(v);

    vals = std::move(unique);
  };

  canonicalize(all_xs);
  canonicalize(all_ys);

  BRep_Builder    builder_v;
  TopoDS_Compound comp_v;
  builder_v.MakeCompound(comp_v);

  BRep_Builder    builder_h;
  TopoDS_Compound comp_h;
  builder_h.MakeCompound(comp_h);

  BRep_Builder    builder_m;
  TopoDS_Compound comp_m;
  builder_m.MakeCompound(comp_m);

  // Add vertical lines for every unique (canonicalized) X
  double min_u{}, min_v{}, max_u{}, max_v{};
  bool   have_bounds = view_bounds_2d_(min_u, min_v, max_u, max_v);

  for (double x : all_xs)
  {
    TopoDS_Shape line;
    if (have_bounds)
    {
      gp_Pnt p0 = to_3d(m_pln, gp_Pnt2d(x, min_v));
      gp_Pnt p1 = to_3d(m_pln, gp_Pnt2d(x, max_v));
      line      = BRepBuilderAPI_MakeEdge(p0, p1).Edge();
    }
    else
    {
      gp_Pnt p0 = to_3d(m_pln, gp_Pnt2d(x, all_pts[0].Y() - 100));
      gp_Pnt p1 = to_3d(m_pln, gp_Pnt2d(x, all_pts[0].Y() + 100));
      line      = BRepBuilderAPI_MakeEdge(p0, p1).Edge();
    }
    builder_v.Add(comp_v, line);
  }

  // Add horizontal lines for every unique (canonicalized) Y
  for (double y : all_ys)
  {
    TopoDS_Shape line;
    if (have_bounds)
    {
      gp_Pnt p0 = to_3d(m_pln, gp_Pnt2d(min_u, y));
      gp_Pnt p1 = to_3d(m_pln, gp_Pnt2d(max_u, y));
      line      = BRepBuilderAPI_MakeEdge(p0, p1).Edge();
    }
    else
    {
      gp_Pnt p0 = to_3d(m_pln, gp_Pnt2d(all_pts[0].X() - 100, y));
      gp_Pnt p1 = to_3d(m_pln, gp_Pnt2d(all_pts[0].X() + 100, y));
      line      = BRepBuilderAPI_MakeEdge(p0, p1).Edge();
    }
    builder_h.Add(comp_h, line);
  }

  // Add small boxes at every node position (the "annotate" part)
  for (const auto& p : all_pts)
  {
    const TopoDS_Shape small = create_wire_box(m_pln, to_3d(m_pln, p), snap_dist, snap_dist);
    builder_m.Add(comp_m, small);
  }

  update_snap_ais_shape_(m_ctx, m_global_coax_fs_v, comp_v, s_snap_guide_color_axis);
  update_snap_ais_shape_(m_ctx, m_global_coax_fs_h, comp_h, s_snap_guide_color_axis);
  update_snap_ais_shape_(m_ctx, m_global_coax_markers, comp_m, s_snap_guide_color_axis);
}

std::optional<size_t> Sketch_nodes::Impl::try_get_node_idx_snap(
    gp_Pnt2d&                  pt, // `pt` could be snapped to a node, an axis of another node, or an outside snap point.
    const std::vector<size_t>& to_exclude)
{
  if (m_view.sketch_snap_suppressed())
  {
    m_owner->hide_snap_annos();
    return std::nullopt;
  }

  const double snap_dist = snap_radius_world_(pt);

  if (!s_annotate_all_coaxial_nodes &&
      (!m_global_coax_fs_v.IsNull() || !m_global_coax_fs_h.IsNull() || !m_global_coax_markers.IsNull()))
  {
    clear_snap_ais_(m_ctx, m_global_coax_fs_v);
    clear_snap_ais_(m_ctx, m_global_coax_fs_h);
    clear_snap_ais_(m_ctx, m_global_coax_markers);
  }

  m_owner->hide_snap_annos();

  gp_Pnt2d              pt_original = pt;
  std::optional<size_t> snap_node_idx[2];
  for (int axis_idx = 0; axis_idx < 2; ++axis_idx)
  {
    std::optional<gp_Pnt2d> snap_axis_point;
    double                  best_dist = std::numeric_limits<double>::max();

    // For "all coaxial" annotation we collect every node (current sketch + outside)
    // whose coordinate on this axis is within tolerance. The *closest* one is still
    // used for the actual snap of `pt` and for the main guide line.
    std::vector<gp_Pnt2d> axis_anno_points;

    auto try_nd_pt = [&](const gp_Pnt2d& nd_pt) -> bool
    {
      double dist                      = pt_original.SquareDistance(nd_pt);
      double axis_snap_threshold_world = sqrt(snap_dist) * 0.5;
      double axis_dist                 = std::fabs(pt_original.XY().Coord(axis_idx + 1) - nd_pt.XY().Coord(axis_idx + 1));

      const bool axis_matches = axis_dist <= axis_snap_threshold_world;

      if (axis_matches)
        axis_anno_points.push_back(nd_pt);

      if (dist < best_dist && axis_matches)
      {
        best_dist       = dist;
        snap_axis_point = nd_pt;
        if (!axis_idx)
          pt.SetX(nd_pt.X());
        else
          pt.SetY(nd_pt.Y());

        return true;
      }

      return false;
    };

    for (size_t nd_idx = 0, num = m_nodes.size(); nd_idx < num; ++nd_idx)
    {
      if (!node_snap_eligible_(m_nodes[nd_idx]))
        continue;

      if (std::find(to_exclude.begin(), to_exclude.end(), nd_idx) != to_exclude.end())
        continue;

      if (try_nd_pt(m_nodes[nd_idx]))
        snap_node_idx[axis_idx] = nd_idx;
    }

    for (const gp_Pnt2d& nd_pt : m_outside_snap_pts)
      try_nd_pt(nd_pt);

    // If the "annotate all coaxial" option is on we keep the full list (dedup later if wanted).
    // Otherwise we only annotate the single closest one that drove the snap.
    if (!s_annotate_all_coaxial_nodes && snap_axis_point)
      axis_anno_points = {*snap_axis_point};

    if (!axis_anno_points.empty())
    {
      // Dedup points that are essentially identical on the axis (floating point tolerance)
      std::vector<gp_Pnt2d> unique_pts;
      for (const auto& p : axis_anno_points)
      {
        bool dup = false;
        for (const auto& u : unique_pts)
          if (std::fabs(p.XY().Coord(axis_idx + 1) - u.XY().Coord(axis_idx + 1)) < 1e-9)
          {
            dup = true;
            break;
          }

        if (!dup)
          unique_pts.push_back(p);
      }

      update_axis_snap_anno_(axis_idx, unique_pts, sqrt(snap_dist));
    }
    else
    {
      clear_snap_ais_(m_ctx, m_snap_anno_axis[axis_idx]);
      clear_snap_ais_(m_ctx, m_snap_anno_axis_fs[axis_idx]);
    }
  }

  if (s_annotate_all_coaxial_nodes)
  {
    // For the active axes (those for which we have a guide from the current snap),
    // collect ALL nodes (current sketch + other visible sketches) whose coordinate
    // on the axis matches the final guide value exactly or within Precision::Confusion().
    // Then draw the small annotation markers at all of them.
    for (int axis_idx = 0; axis_idx < 2; ++axis_idx)
    {
      double guide_val = (axis_idx == 0 ? pt.X() : pt.Y());

      std::vector<gp_Pnt2d> matches;
      for (const auto& n : m_nodes)
      {
        if (!node_snap_eligible_(n))
          continue;

        double axis_diff = std::fabs(guide_val - n.XY().Coord(axis_idx + 1));
        if (axis_diff <= Precision::Confusion())
          matches.push_back(n);
      }

      for (const auto& p : m_outside_snap_pts)
      {
        double axis_diff = std::fabs(guide_val - p.XY().Coord(axis_idx + 1));
        if (axis_diff <= Precision::Confusion())
          matches.push_back(p);
      }

      if (!matches.empty())
      {
        // dedup by position
        std::vector<gp_Pnt2d> unique;
        for (const auto& p : matches)
        {
          bool seen = false;
          for (const auto& u : unique)
            if (p.Distance(u) < Precision::Confusion())
            {
              seen = true;
              break;
            }

          if (!seen)
            unique.push_back(p);
        }

        update_axis_snap_anno_(axis_idx, unique, sqrt(snap_dist));
      }
    }
  }

  if (snap_node_idx[0].has_value() && snap_node_idx[0] == snap_node_idx[1])
  {
    for (int i = 0; i < 2; ++i)
    {
      clear_snap_ais_(m_ctx, m_snap_anno_axis[i]);
      clear_snap_ais_(m_ctx, m_snap_anno_axis_fs[i]);
    }

    update_node_snap_anno_(pt, sqrt(snap_dist));
    m_ctx.UpdateCurrentViewer();
    return snap_node_idx[0];
  }

  m_ctx.UpdateCurrentViewer();
  return {};
}

// === Impl data access ======================================================

// clang-format off
std::vector<Sketch_nodes::Node>::iterator Sketch_nodes::Impl::begin()               { return m_nodes.begin(); }
std::vector<Sketch_nodes::Node>::iterator Sketch_nodes::Impl::end()                 { return m_nodes.end(); }
std::vector<Sketch_nodes::Node>::const_iterator Sketch_nodes::Impl::begin()   const { return m_nodes.begin(); }
std::vector<Sketch_nodes::Node>::const_iterator Sketch_nodes::Impl::end()     const { return m_nodes.end(); }
std::vector<Sketch_nodes::Node>::const_iterator Sketch_nodes::Impl::cbegin()  const { return m_nodes.cbegin(); }
std::vector<Sketch_nodes::Node>::const_iterator Sketch_nodes::Impl::cend()    const { return m_nodes.cend(); }
// clang-format on

Sketch_nodes::Node& Sketch_nodes::Impl::node_at(size_t idx)
{
  EZY_ASSERT(idx < size());
  return m_nodes[idx];
}

const Sketch_nodes::Node& Sketch_nodes::Impl::node_at(size_t idx) const { return m_nodes[idx]; }

bool Sketch_nodes::Impl::empty() const { return m_nodes.empty(); }

size_t Sketch_nodes::Impl::size() const { return m_nodes.size(); }

void Sketch_nodes::Impl::resize(size_t count) { m_nodes.assign(count, Node{}); }

void Sketch_nodes::Impl::set_node(size_t idx, const gp_Pnt2d& pt, bool deleted, bool midpoint, bool permanent, bool origin,
                                  const std::string& name)
{
  EZY_ASSERT(idx < m_nodes.size());
  Node& n = m_nodes[idx];
  n.SetX(pt.X());
  n.SetY(pt.Y());
  n.deleted   = deleted;
  n.midpoint  = midpoint;
  n.permanent = permanent;
  n.origin    = origin;
  n.name      = name;
}

void Sketch_nodes::Impl::restore_node_at(size_t idx, const gp_Pnt2d& pt, bool deleted, bool midpoint, bool permanent,
                                         bool origin, const std::string& name)
{
  if (idx >= size())
    resize(idx + 1);

  set_node(idx, pt, deleted, midpoint, permanent, origin, name);
}

void Sketch_nodes::Impl::finalize() { m_prev_num_nodes = m_nodes.size(); }

void Sketch_nodes::Impl::cancel() { m_nodes.resize(m_prev_num_nodes); }

void Sketch_nodes::Impl::clear_outside_snap_pnts() { m_outside_snap_pts.clear(); }

void Sketch_nodes::Impl::add_outside_snap_pnt(const gp_Pnt& pt3d) { m_outside_snap_pts.insert(to_2d(m_pln, pt3d)); }

Sketch_nodes::Sketch_nodes(Occt_view& view, const gp_Pln& pln)
    : m_impl(std::make_unique<Impl>(this, view, pln))
{
}

Sketch_nodes::~Sketch_nodes()
{
  hide_snap_annos(); // Deletes them from context
}

// === Iteration =============================================================

// clang-format off
std::vector<Sketch_nodes::Node>::iterator       Sketch_nodes::begin()         { return m_impl->begin();   }
std::vector<Sketch_nodes::Node>::iterator       Sketch_nodes::end()           { return m_impl->end();     }
std::vector<Sketch_nodes::Node>::const_iterator Sketch_nodes::begin()   const { return m_impl->begin();   }
std::vector<Sketch_nodes::Node>::const_iterator Sketch_nodes::end()     const { return m_impl->end();     }
std::vector<Sketch_nodes::Node>::const_iterator Sketch_nodes::cbegin()  const { return m_impl->cbegin();  }
std::vector<Sketch_nodes::Node>::const_iterator Sketch_nodes::cend()    const { return m_impl->cend();    }
// clang-format on

std::optional<gp_Pnt2d> Sketch_nodes::snap(const ScreenCoords& screen_coords) { return m_impl->snap(screen_coords); }

size_t Sketch_nodes::get_node_exact(const gp_Pnt2d& pt, bool permanent_for_new)
{
  return m_impl->get_node_exact(pt, permanent_for_new);
}

std::optional<size_t> Sketch_nodes::get_node(const ScreenCoords& screen_coords) { return m_impl->get_node(screen_coords); }

std::optional<size_t> Sketch_nodes::try_pick_existing_node(const ScreenCoords& screen_coords)
{
  return m_impl->try_pick_existing_node(screen_coords);
}

std::optional<size_t> Sketch_nodes::try_get_node_idx_snap(
    gp_Pnt2d&                  pt, // `pt` could be snapped to a node, an axis of another node, or an outside snap point.
    const std::vector<size_t>& to_exclude)
{
  return m_impl->try_get_node_idx_snap(pt, to_exclude);
}

void Sketch_nodes::hide_snap_annos() { m_impl->hide_snap_annos(); }

size_t Sketch_nodes::add_new_node(const gp_Pnt2d& pt, bool is_edge_mid_point, bool is_permanent)
{
  return m_impl->add_new_node(pt, is_edge_mid_point, is_permanent);
}

void Sketch_nodes::get_snap_pts_3d(std::vector<gp_Pnt>& out) { m_impl->get_snap_pts_3d(out); }

Sketch_nodes::Node& Sketch_nodes::operator[](size_t idx) { return m_impl->node_at(idx); }

const Sketch_nodes::Node& Sketch_nodes::operator[](size_t idx) const { return m_impl->node_at(idx); }

Sketch_nodes::Node& Sketch_nodes::operator[](const std::optional<size_t> idx)
{
  EZY_ASSERT(idx.has_value());
  return m_impl->node_at(idx.value());
}

const Sketch_nodes::Node& Sketch_nodes::operator[](const std::optional<size_t> idx) const
{
  EZY_ASSERT(idx.has_value());
  return m_impl->node_at(idx.value());
}

bool Sketch_nodes::empty() const { return m_impl->empty(); }

size_t Sketch_nodes::size() const { return m_impl->size(); }

void Sketch_nodes::resize(size_t count) { m_impl->resize(count); }

void Sketch_nodes::set_node(size_t idx, const gp_Pnt2d& pt, bool deleted, bool midpoint, bool permanent, bool origin,
                            const std::string& name)
{
  m_impl->set_node(idx, pt, deleted, midpoint, permanent, origin, name);
}

void Sketch_nodes::finalize() { m_impl->finalize(); }

void Sketch_nodes::cancel() { m_impl->cancel(); }

void Sketch_nodes::restore_node_at(size_t idx, const gp_Pnt2d& pt, bool deleted, bool midpoint, bool permanent, bool origin,
                                   const std::string& name)
{
  m_impl->restore_node_at(idx, pt, deleted, midpoint, permanent, origin, name);
}

void Sketch_nodes::clear_outside_snap_pnts() { m_impl->clear_outside_snap_pnts(); }

void Sketch_nodes::add_outside_snap_pnt(const gp_Pnt& pt3d) { m_impl->add_outside_snap_pnt(pt3d); }

// === Snap settings =========================================================

void Sketch_nodes::set_snap_dist(double snap_dist_pixels) { s_snap_dist_pixels = snap_dist_pixels; }

double Sketch_nodes::get_snap_dist() { return s_snap_dist_pixels; }

void Sketch_nodes::set_snap_guide_mode(Snap_guide_mode mode) { s_snap_guide_mode = mode; }

Sketch_nodes::Snap_guide_mode Sketch_nodes::get_snap_guide_mode() { return s_snap_guide_mode; }

void Sketch_nodes::set_snap_guide_color_node(float r, float g, float b)
{
  s_snap_guide_color_node.x = std::clamp(r, 0.0f, 1.0f);
  s_snap_guide_color_node.y = std::clamp(g, 0.0f, 1.0f);
  s_snap_guide_color_node.z = std::clamp(b, 0.0f, 1.0f);
}

void Sketch_nodes::get_snap_guide_color_node(float& r, float& g, float& b)
{
  r = s_snap_guide_color_node.x;
  g = s_snap_guide_color_node.y;
  b = s_snap_guide_color_node.z;
}

void Sketch_nodes::set_snap_guide_color_axis(float r, float g, float b)
{
  s_snap_guide_color_axis.x = std::clamp(r, 0.0f, 1.0f);
  s_snap_guide_color_axis.y = std::clamp(g, 0.0f, 1.0f);
  s_snap_guide_color_axis.z = std::clamp(b, 0.0f, 1.0f);
}

glm::vec3 Sketch_nodes::get_snap_guide_color_axis() { return s_snap_guide_color_axis; }

void Sketch_nodes::set_snap_guide_line_width(float width) { s_snap_guide_line_width = std::clamp(width, 0.5f, 8.0f); }

float Sketch_nodes::get_snap_guide_line_width() { return s_snap_guide_line_width; }

void Sketch_nodes::set_annotate_all_coaxial_nodes(bool enable) { s_annotate_all_coaxial_nodes = enable; }

bool Sketch_nodes::get_annotate_all_coaxial_nodes() { return s_annotate_all_coaxial_nodes; }

void Sketch_nodes::set_origin_snap_enabled(bool enabled) { m_impl->set_origin_snap_enabled(enabled); }

bool Sketch_nodes::origin_snap_enabled() const { return m_impl->origin_snap_enabled(); }