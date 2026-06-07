#include "sketch_nodes.h"

#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Wire.hxx>
#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

#include "dbg.h"
#include "geom.h"
#include "imgui.h"
#include "occt_view.h"

namespace
{
double                        s_snap_dist_pixels = 35.0;
Sketch_nodes::Snap_guide_mode s_snap_guide_mode  = Sketch_nodes::Snap_guide_mode::Traditional;
glm::vec3                     s_snap_guide_color{0.0f, 1.0f, 0.0f};
} // namespace

struct Sketch_nodes::Impl
{
  Impl(Occt_view& view, const gp_Pln& pln)
      : view(view)
      , ctx(view.ctx())
      , pln(pln)
  {
  }

  std::vector<Node>       nodes;
  std::set<gp_Pnt2d>      outside_snap_pts; // Projected snap points from other sketches.
  AIS_Shape_ptr           snap_anno_axis[2];
  std::optional<gp_Pnt2d> last_snap_pt; // Used for snap annotation
  AIS_Shape_ptr           snap_anno;
  size_t                  prev_num_nodes{0}; // Used when an operation is canceled.

  // Owner related
  Occt_view&              view;
  AIS_InteractiveContext& ctx;
  const gp_Pln            pln;

  /// World-space snap radius at `pt` (same convention as `try_get_node_idx_snap` / `try_pick_existing_node`).
  double snap_radius_world_(const gp_Pnt2d& pt) const;
  bool   view_bounds_2d_(double& min_u, double& min_v, double& max_u, double& max_v) const;
  void   update_node_snap_anno_(const gp_Pnt2d& pt, const double snap_dist);
  void   update_axis_snap_anno_(int axis_index, const gp_Pnt2d& axis_pt, double snap_dist);
};

Sketch_nodes::Sketch_nodes(Occt_view& view, const gp_Pln& pln)
    : m_impl(std::make_unique<Impl>(view, pln))
{
}

Sketch_nodes::~Sketch_nodes()
{
  hide_snap_annos(); // Deletes them from context
}

// === Iteration =============================================================

std::vector<Sketch_nodes::Node>::iterator       Sketch_nodes::begin() { return m_impl->nodes.begin(); }
std::vector<Sketch_nodes::Node>::iterator       Sketch_nodes::end() { return m_impl->nodes.end(); }
std::vector<Sketch_nodes::Node>::const_iterator Sketch_nodes::begin() const { return m_impl->nodes.begin(); }
std::vector<Sketch_nodes::Node>::const_iterator Sketch_nodes::end() const { return m_impl->nodes.end(); }
std::vector<Sketch_nodes::Node>::const_iterator Sketch_nodes::cbegin() const { return m_impl->nodes.cbegin(); }
std::vector<Sketch_nodes::Node>::const_iterator Sketch_nodes::cend() const { return m_impl->nodes.cend(); }

// === Public API ============================================================

std::optional<gp_Pnt2d> Sketch_nodes::snap(const ScreenCoords& screen_coords)
{
  std::optional<gp_Pnt2d> pt = m_impl->view.pt_on_plane(screen_coords, m_impl->pln);
  if (pt)
    try_get_node_idx_snap(*pt);

  return pt;
}

size_t Sketch_nodes::get_node_exact(const gp_Pnt2d& pt, bool permanent_for_new)
{
  std::optional<size_t> deleted_match;
  for (size_t idx = 0, num = m_impl->nodes.size(); idx < num; ++idx)
    if (equal(pt, gp_Pnt2d(m_impl->nodes[idx])))
    {
      Node& n = m_impl->nodes[idx];
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
    Node& n   = m_impl->nodes[*deleted_match];
    n.deleted = false;
    if (permanent_for_new)
      n.permanent = true;
    return *deleted_match;
  }

  Node n(pt);
  n.permanent      = permanent_for_new;
  const size_t ret = m_impl->nodes.size();
  m_impl->nodes.push_back(n);
  return ret;
}

std::optional<size_t> Sketch_nodes::get_node(const ScreenCoords& screen_coords)
{
  std::optional<gp_Pnt2d> pt = m_impl->view.pt_on_plane(screen_coords, m_impl->pln);
  if (!pt)
    // View plane and sketch plane must be perpendicular.
    return std::nullopt;

  std::optional<size_t> idx = try_get_node_idx_snap(*pt);
  if (idx.has_value())
    return *idx;

  return add_new_node(*pt);
}

std::optional<size_t> Sketch_nodes::try_pick_existing_node(const ScreenCoords& screen_coords)
{
  std::optional<gp_Pnt2d> pt_opt = m_impl->view.pt_on_plane(screen_coords, m_impl->pln);
  if (!pt_opt)
  {
    hide_snap_annos();
    return std::nullopt;
  }

  const gp_Pnt2d pt        = *pt_opt;
  const double   snap_dist = m_impl->snap_radius_world_(pt);
  size_t         best_idx  = static_cast<size_t>(-1);
  double         best_sq   = std::numeric_limits<double>::max();
  for (size_t idx = 0, num = m_impl->nodes.size(); idx < num; ++idx)
  {
    if (m_impl->nodes[idx].deleted)
      continue;

    const double sq = m_impl->nodes[idx].SquareDistance(pt);
    if (sq < best_sq)
    {
      best_sq  = sq;
      best_idx = idx;
    }
  }
  if (best_idx == static_cast<size_t>(-1))
  {
    hide_snap_annos();
    return std::nullopt;
  }
  if (best_sq <= snap_dist * 0.25 * snap_dist)
  {
    // `try_get_node_idx_snap` can modify the input `pt`, we call this function to display snapping annotations.
    gp_Pnt2d pt_snapped = m_impl->nodes[best_idx];
    try_get_node_idx_snap(pt_snapped, {});
    return best_idx;
  }
  hide_snap_annos();
  return std::nullopt;
}

std::optional<size_t> Sketch_nodes::try_get_node_idx_snap(
    gp_Pnt2d&                  pt, // `pt` could be snapped to a node, an axis of another node, or an outside snap point.
    const std::vector<size_t>& to_exclude)
{
  const double snap_dist = m_impl->snap_radius_world_(pt);

  hide_snap_annos();

  gp_Pnt2d              pt_original = pt;
  std::optional<size_t> snap_node_idx[2];
  for (int axis_idx = 0; axis_idx < 2; ++axis_idx)
  {
    std::optional<gp_Pnt2d> snap_axis_point;
    double                  best_dist = std::numeric_limits<double>::max();

    auto try_nd_pt = [&](const gp_Pnt2d& nd_pt) -> bool
    {
      double dist = pt_original.SquareDistance(nd_pt);
      // axis_dist needs to be compared against a linear snap distance in screen pixels.
      // This part becomes tricky because axis_dist is a world coordinate difference.
      // We'd ideally convert m_snap_dist_pixels * 0.5 to a world distance along the axis.
      // For simplicity here, we'll continue using a fraction of the calculated world snap_dist_sq,
      // but this might need refinement for axis snapping to feel right.
      // The original logic used snap_dist (linear pixels) * 0.5 for axis snapping.
      // We need a world-space equivalent for axis snapping.
      // Let's use sqrt(snap_dist_sq) * 0.5 for now.
      double axis_snap_threshold_world = sqrt(snap_dist) * 0.5;
      double axis_dist                 = std::fabs(pt_original.XY().Coord(axis_idx + 1) - nd_pt.XY().Coord(axis_idx + 1));

      if (dist < best_dist && axis_dist <= axis_snap_threshold_world)
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

    for (size_t nd_idx = 0, num = m_impl->nodes.size(); nd_idx < num; ++nd_idx)
    {
      if (m_impl->nodes[nd_idx].deleted)
        continue;

      if (std::find(to_exclude.begin(), to_exclude.end(), nd_idx) != to_exclude.end())
        continue;

      if (try_nd_pt(m_impl->nodes[nd_idx]))
        snap_node_idx[axis_idx] = nd_idx;
    }

    for (const gp_Pnt2d& nd_pt : m_impl->outside_snap_pts)
      try_nd_pt(nd_pt);

    if (snap_axis_point)
      m_impl->update_axis_snap_anno_(axis_idx, *snap_axis_point, sqrt(snap_dist));
    else if (!m_impl->snap_anno_axis[axis_idx].IsNull())
      m_impl->ctx.Erase(m_impl->snap_anno_axis[axis_idx], true);
  }

  if (snap_node_idx[0] == snap_node_idx[1] && snap_node_idx[0].has_value())
    return snap_node_idx[0];

  return {};
}

void Sketch_nodes::hide_snap_annos()
{
  if (m_impl->snap_anno)
    m_impl->ctx.Remove(m_impl->snap_anno, false);

  m_impl->snap_anno = nullptr;

  for (AIS_Shape_ptr& anno : m_impl->snap_anno_axis)
    if (anno)
    {
      m_impl->ctx.Remove(anno, false);
      anno = nullptr;
    }

  m_impl->ctx.UpdateCurrentViewer();
  m_impl->last_snap_pt = std::nullopt;
}

size_t Sketch_nodes::add_new_node(const gp_Pnt2d& pt, bool is_edge_mid_point, bool is_permanent)
{
  size_t ret = m_impl->nodes.size();
  Node   n(pt);
  n.midpoint  = is_edge_mid_point;
  n.permanent = is_permanent;
  m_impl->nodes.emplace_back(n);
  // DBG_MSG("Add node: " << pt.Coord().X() << "," << pt.Coord().Y() << " midpoint: " << (int) is_edge_mid_point);
  return ret;
}

void Sketch_nodes::get_snap_pts_3d(std::vector<gp_Pnt>& out)
{
  for (const Node& n : m_impl->nodes)
    if (!n.deleted)
      out.push_back(to_3d(m_impl->pln, n));
}

Sketch_nodes::Node& Sketch_nodes::operator[](size_t idx)
{
  EZY_ASSERT(idx < size());
  return m_impl->nodes[idx];
}

const Sketch_nodes::Node& Sketch_nodes::operator[](size_t idx) const
{
  EZY_ASSERT(idx < size());
  return m_impl->nodes[idx];
}

Sketch_nodes::Node& Sketch_nodes::operator[](const std::optional<size_t> idx)
{
  EZY_ASSERT(idx.has_value());
  EZY_ASSERT(*idx < size());
  return m_impl->nodes[idx.value()];
}

const Sketch_nodes::Node& Sketch_nodes::operator[](const std::optional<size_t> idx) const
{
  EZY_ASSERT(idx.has_value());
  EZY_ASSERT(*idx < size());
  return m_impl->nodes[idx.value()];
}

bool Sketch_nodes::empty() const { return m_impl->nodes.empty(); }

size_t Sketch_nodes::size() const { return m_impl->nodes.size(); }

void Sketch_nodes::json_resize(size_t count) { m_impl->nodes.assign(count, Node{}); }

void Sketch_nodes::json_set_node(size_t idx, const gp_Pnt2d& pt, bool deleted, bool midpoint, bool permanent,
                                 const std::string& name)
{
  EZY_ASSERT(idx < m_impl->nodes.size());
  Node& n = m_impl->nodes[idx];
  n.SetX(pt.X());
  n.SetY(pt.Y());
  n.deleted   = deleted;
  n.midpoint  = midpoint;
  n.permanent = permanent;
  n.name      = name;
}

void Sketch_nodes::finalize() { m_impl->prev_num_nodes = m_impl->nodes.size(); }

void Sketch_nodes::cancel() { m_impl->nodes.resize(m_impl->prev_num_nodes); }

void Sketch_nodes::clear_outside_snap_pnts() { m_impl->outside_snap_pts.clear(); }

void Sketch_nodes::add_outside_snap_pnt(const gp_Pnt& pt3d) { m_impl->outside_snap_pts.insert(to_2d(m_impl->pln, pt3d)); }

// === Impl helpers ==========================================================

double Sketch_nodes::Impl::snap_radius_world_(const gp_Pnt2d& pt) const
{
  if (!view.is_headless())
  {
    gp_Pnt       pt3d_on_plane       = to_3d(pln, pt);
    ScreenCoords screen_coords_at_pt = view.get_screen_coords(pt3d_on_plane);

    ScreenCoords screen_coords_offset = screen_coords_at_pt;
    screen_coords_offset.unsafe_get().x += s_snap_dist_pixels;

    std::optional<gp_Pnt2d> pt_offset_on_plane_2d;
    if (std::optional<gp_Pnt> pt_offset_on_plane_3d = view.pt3d_on_plane(screen_coords_offset, pln))
      pt_offset_on_plane_2d = to_2d(pln, *pt_offset_on_plane_3d);

    if (pt_offset_on_plane_2d)
      return pt.Distance(*pt_offset_on_plane_2d);
    return 5.0;
  }
  return s_snap_dist_pixels;
}

bool Sketch_nodes::Impl::view_bounds_2d_(double& min_u, double& min_v, double& max_u, double& max_v) const
{
  if (view.is_headless())
    return false;

  const ImGuiIO& io = ImGui::GetIO();
  return view.sketch_plane_view_aabb_2d(pln, static_cast<double>(io.DisplaySize.x), static_cast<double>(io.DisplaySize.y),
                                        min_u, min_v, max_u, max_v);
}

void Sketch_nodes::Impl::update_node_snap_anno_(const gp_Pnt2d& pt, const double snap_dist)
{
  if (last_snap_pt && equal(*last_snap_pt, pt))
    return;

  last_snap_pt = pt;

  const auto mode = s_snap_guide_mode;
  const bool show_traditional =
      mode == Sketch_nodes::Snap_guide_mode::Traditional || mode == Sketch_nodes::Snap_guide_mode::Both;
  const bool show_fullscreen = mode == Sketch_nodes::Snap_guide_mode::Fullscreen || mode == Sketch_nodes::Snap_guide_mode::Both;

  TopoDS_Shape fullscreen_shape;
  if (show_fullscreen)
  {
    double min_u{}, min_v{}, max_u{}, max_v{};
    if (view_bounds_2d_(min_u, min_v, max_u, max_v))
    {
      BRep_Builder    builder;
      TopoDS_Compound comp;
      builder.MakeCompound(comp);

      const gp_Pnt p_h0 = to_3d(pln, gp_Pnt2d(min_u, pt.Y()));
      const gp_Pnt p_h1 = to_3d(pln, gp_Pnt2d(max_u, pt.Y()));
      const gp_Pnt p_v0 = to_3d(pln, gp_Pnt2d(pt.X(), min_v));
      const gp_Pnt p_v1 = to_3d(pln, gp_Pnt2d(pt.X(), max_v));
      builder.Add(comp, BRepBuilderAPI_MakeEdge(p_h0, p_h1).Edge());
      builder.Add(comp, BRepBuilderAPI_MakeEdge(p_v0, p_v1).Edge());
      fullscreen_shape = comp;
    }
  }

  TopoDS_Shape       anno_shape;
  const TopoDS_Shape traditional_shape = create_wire_box(pln, to_3d(pln, pt), snap_dist, snap_dist);
  if (show_traditional && !fullscreen_shape.IsNull())
  {
    BRep_Builder    builder;
    TopoDS_Compound comp;
    builder.MakeCompound(comp);
    builder.Add(comp, fullscreen_shape);
    builder.Add(comp, traditional_shape);
    anno_shape = comp;
  }
  else if (!fullscreen_shape.IsNull())
    anno_shape = fullscreen_shape;
  else
    anno_shape = traditional_shape;

  const glm::vec3& c = s_snap_guide_color;
  if (snap_anno.IsNull())
  {
    snap_anno = new AIS_Shape(anno_shape);
    snap_anno->SetWidth(3.0);
    snap_anno->SetColor(Quantity_Color(c.x, c.y, c.z, Quantity_TOC_RGB));
    ctx.Display(snap_anno, true);
  }
  else
  {
    snap_anno->Set(anno_shape);
    ctx.Redisplay(snap_anno, true);
  }
}

void Sketch_nodes::Impl::update_axis_snap_anno_(int axis_index, const gp_Pnt2d& axis_pt, double snap_dist)
{
  const auto mode = s_snap_guide_mode;
  const bool show_traditional =
      mode == Sketch_nodes::Snap_guide_mode::Traditional || mode == Sketch_nodes::Snap_guide_mode::Both;
  const bool show_fullscreen = mode == Sketch_nodes::Snap_guide_mode::Fullscreen || mode == Sketch_nodes::Snap_guide_mode::Both;

  TopoDS_Shape fullscreen_shape;
  if (show_fullscreen)
  {
    double min_u{}, min_v{}, max_u{}, max_v{};
    if (view_bounds_2d_(min_u, min_v, max_u, max_v))
    {
      if (axis_index == 0)
      {
        const gp_Pnt p0  = to_3d(pln, gp_Pnt2d(axis_pt.X(), min_v));
        const gp_Pnt p1  = to_3d(pln, gp_Pnt2d(axis_pt.X(), max_v));
        fullscreen_shape = BRepBuilderAPI_MakeEdge(p0, p1).Edge();
      }
      else
      {
        const gp_Pnt p0  = to_3d(pln, gp_Pnt2d(min_u, axis_pt.Y()));
        const gp_Pnt p1  = to_3d(pln, gp_Pnt2d(max_u, axis_pt.Y()));
        fullscreen_shape = BRepBuilderAPI_MakeEdge(p0, p1).Edge();
      }
    }
  }

  TopoDS_Shape       anno_shape;
  const TopoDS_Shape traditional_shape = create_wire_box(pln, to_3d(pln, axis_pt), snap_dist, snap_dist);
  if (show_traditional && !fullscreen_shape.IsNull())
  {
    BRep_Builder    builder;
    TopoDS_Compound comp;
    builder.MakeCompound(comp);
    builder.Add(comp, fullscreen_shape);
    builder.Add(comp, traditional_shape);
    anno_shape = comp;
  }
  else if (!fullscreen_shape.IsNull())
    anno_shape = fullscreen_shape;
  else
    anno_shape = traditional_shape;

  const glm::vec3& c = s_snap_guide_color;
  if (snap_anno_axis[axis_index].IsNull())
  {
    snap_anno_axis[axis_index] = new AIS_Shape(anno_shape);
    snap_anno_axis[axis_index]->SetWidth(1.0);
    snap_anno_axis[axis_index]->SetColor(Quantity_Color(c.x, c.y, c.z, Quantity_TOC_RGB));
    ctx.Display(snap_anno_axis[axis_index], true);
  }
  else
  {
    snap_anno_axis[axis_index]->Set(anno_shape);
    ctx.Redisplay(snap_anno_axis[axis_index], true);
  }
}

// === Snap settings =========================================================

void Sketch_nodes::set_snap_dist(double snap_dist_pixels) { s_snap_dist_pixels = snap_dist_pixels; }

double Sketch_nodes::get_snap_dist() { return s_snap_dist_pixels; }

void Sketch_nodes::set_snap_guide_mode(Snap_guide_mode mode) { s_snap_guide_mode = mode; }

Sketch_nodes::Snap_guide_mode Sketch_nodes::get_snap_guide_mode() { return s_snap_guide_mode; }

void Sketch_nodes::set_snap_guide_color(float r, float g, float b)
{
  s_snap_guide_color.x = std::clamp(r, 0.0f, 1.0f);
  s_snap_guide_color.y = std::clamp(g, 0.0f, 1.0f);
  s_snap_guide_color.z = std::clamp(b, 0.0f, 1.0f);
}

void Sketch_nodes::get_snap_guide_color(float& r, float& g, float& b)
{
  r = s_snap_guide_color.x;
  g = s_snap_guide_color.y;
  b = s_snap_guide_color.z;
}
