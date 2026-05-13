#include "sketch_nodes.h"

#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Wire.hxx>
#include <algorithm>
#include <limits>

#include "dbg.h"
#include "geom.h"
#include "imgui.h"
#include "occt_view.h"

Sketch_nodes::Sketch_nodes(Occt_view& view, const gp_Pln& pln)
    : m_view(view), m_ctx(m_view.ctx()), m_pln(pln)
{
}

Sketch_nodes::~Sketch_nodes()
{
  hide_snap_annos();  // Deletes them from context
}

std::optional<gp_Pnt2d> Sketch_nodes::snap(const ScreenCoords& screen_coords)
{
  std::optional<gp_Pnt2d> pt = m_view.pt_on_plane(screen_coords, m_pln);
  if (pt)
    try_get_node_idx_snap(*pt);

  return pt;
}

size_t Sketch_nodes::get_node_exact(const gp_Pnt2d& pt, bool permanent_for_new)
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
    Node& n = m_nodes[*deleted_match];
    n.deleted = false;
    if (permanent_for_new)
      n.permanent = true;
    return *deleted_match;
  }

  Node n(pt);
  n.permanent = permanent_for_new;
  const size_t ret = m_nodes.size();
  m_nodes.push_back(n);
  return ret;
}

std::optional<size_t> Sketch_nodes::get_node(const ScreenCoords& screen_coords)
{
  std::optional<gp_Pnt2d> pt = m_view.pt_on_plane(screen_coords, m_pln);
  if (!pt)
    // View plane and sketch plane must be perpendicular.
    return std::nullopt;

  std::optional<size_t> idx = try_get_node_idx_snap(*pt);
  if (idx.has_value())
    return *idx;

  return add_new_node(*pt);
}

double Sketch_nodes::snap_radius_world_(const gp_Pnt2d& pt) const
{
  if (!m_view.is_headless())
  {
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
  return s_snap_dist_pixels;
}

bool Sketch_nodes::view_bounds_2d_(double& min_u, double& min_v, double& max_u, double& max_v) const
{
  if (m_view.is_headless())
    return false;

  const ImGuiIO& io = ImGui::GetIO();
  return m_view.sketch_plane_view_aabb_2d(m_pln, static_cast<double>(io.DisplaySize.x),
                                          static_cast<double>(io.DisplaySize.y), min_u, min_v, max_u, max_v);
}

std::optional<size_t> Sketch_nodes::try_pick_existing_node(const ScreenCoords& screen_coords)
{
  std::optional<gp_Pnt2d> pt_opt = m_view.pt_on_plane(screen_coords, m_pln);
  if (!pt_opt)
  {
    hide_snap_annos();
    return std::nullopt;
  }

  const gp_Pnt2d pt        = *pt_opt;
  const double   snap_dist = snap_radius_world_(pt);
  size_t         best_idx  = static_cast<size_t>(-1);
  double         best_sq   = std::numeric_limits<double>::max();
  for (size_t idx = 0, num = m_nodes.size(); idx < num; ++idx)
  {
    if (m_nodes[idx].deleted)
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
    hide_snap_annos();
    return std::nullopt;
  }
  if (best_sq <= snap_dist * 0.25 * snap_dist)
  {
    update_node_snap_anno_(m_nodes[best_idx], sqrt(snap_dist));
    return best_idx;
  }
  hide_snap_annos();
  return std::nullopt;
}

std::optional<size_t> Sketch_nodes::try_get_node_idx_snap(
    gp_Pnt2d&                  pt,  // `pt` could be snapped to a node, an axis of another node, or an outside snap point.
    const std::vector<size_t>& to_exclude)
{
  const double snap_dist = snap_radius_world_(pt);

  size_t best_idx  = -1;
  double best_dist = std::numeric_limits<double>::max();
  for (size_t idx = 0, num = m_nodes.size(); idx < num; ++idx)
  {
    if (std::find(to_exclude.begin(), to_exclude.end(), idx) != to_exclude.end())
      continue;

    const Node& n = m_nodes[idx];
    if (n.deleted)
      continue;

    double dist = n.SquareDistance(pt);
    if (dist < best_dist)
    {
      best_dist = dist;
      best_idx  = idx;
    }
  }

  if (best_dist <= snap_dist * 0.25)
  {
    pt = gp_Pnt2d(m_nodes[best_idx]);
    update_node_snap_anno_(m_nodes[best_idx], sqrt(snap_dist));
    return best_idx;
  }

  // Try to snap to outside points or do outside axis snap. `pt` might be modified.
  if (try_snap_outside_(pt, sqrt(snap_dist)))
    return {};

  hide_snap_annos();

  gp_Pnt2d pt_original = pt;

  for (int i = 0; i < 2; ++i)
  {
    std::optional<gp_Pnt2d> snap_axis_point;
    best_dist = std::numeric_limits<double>::max();

    auto try_nd_pt = [&](const gp_Pnt2d& nd_pt)
    {
      double dist                      = pt_original.SquareDistance(nd_pt);
      // axis_dist needs to be compared against a linear snap distance in screen pixels.
      // This part becomes tricky because axis_dist is a world coordinate difference.
      // We'd ideally convert m_snap_dist_pixels * 0.5 to a world distance along the axis.
      // For simplicity here, we'll continue using a fraction of the calculated world snap_dist_sq,
      // but this might need refinement for axis snapping to feel right.
      // The original logic used snap_dist (linear pixels) * 0.5 for axis snapping.
      // We need a world-space equivalent for axis snapping.
      // Let's use sqrt(snap_dist_sq) * 0.5 for now.
      double axis_snap_threshold_world = sqrt(snap_dist) * 0.5;
      double axis_dist                 = std::fabs(pt_original.XY().Coord(i + 1) - nd_pt.XY().Coord(i + 1));

      if (dist < best_dist && axis_dist <= axis_snap_threshold_world)
      {
        best_dist       = dist;
        snap_axis_point = nd_pt;
        if (!i)
          pt.SetX(nd_pt.X());
        else
          pt.SetY(nd_pt.Y());
      }
    };

    for (const Node& n : m_nodes)
      if (!n.deleted)
        try_nd_pt(n);

    for (const gp_Pnt2d& nd_pt : m_outside_snap_pts)
      try_nd_pt(nd_pt);

    if (snap_axis_point)
      update_axis_snap_anno_(i, *snap_axis_point, sqrt(snap_dist));
    else if (!m_snap_anno_axis[i].IsNull())
      m_ctx.Erase(m_snap_anno_axis[i], true);
  }

  return {};
}

void Sketch_nodes::hide_snap_annos()
{
  if (m_snap_anno)
    m_ctx.Remove(m_snap_anno, false);

  m_snap_anno = nullptr;

  for (AIS_Shape_ptr& anno : m_snap_anno_axis)
    if (anno)
    {
      m_ctx.Remove(anno, false);
      anno = nullptr;
    }

  m_ctx.UpdateCurrentViewer();
  m_last_snap_pt = std::nullopt;
}

size_t Sketch_nodes::add_new_node(const gp_Pnt2d& pt, bool is_edge_mid_point, bool is_permanent)
{
  size_t ret = m_nodes.size();
  Node   n(pt);
  n.midpoint  = is_edge_mid_point;
  n.permanent = is_permanent;
  m_nodes.emplace_back(n);
  //DBG_MSG("Add node: " << pt.Coord().X() << "," << pt.Coord().Y() << " midpoint: " << (int) is_edge_mid_point);
  return ret;
}

void Sketch_nodes::get_snap_pts_3d(std::vector<gp_Pnt>& out)
{
  for (const Node& n : m_nodes)
    if (!n.deleted)
      out.push_back(to_3d(m_pln, n));
}

void Sketch_nodes::update_node_snap_anno_(const gp_Pnt2d& pt, const double snap_dist)
{
  if (m_last_snap_pt && equal(*m_last_snap_pt, pt))
    return;

  m_last_snap_pt = pt;

  const bool show_traditional =
      s_snap_guide_mode == Snap_guide_mode::Traditional || s_snap_guide_mode == Snap_guide_mode::Both;
  const bool show_fullscreen =
      s_snap_guide_mode == Snap_guide_mode::Fullscreen || s_snap_guide_mode == Snap_guide_mode::Both;

  TopoDS_Shape fullscreen_shape;
  if (show_fullscreen)
  {
    double min_u {}, min_v {}, max_u {}, max_v {};
    if (view_bounds_2d_(min_u, min_v, max_u, max_v))
    {
      BRep_Builder    builder;
      TopoDS_Compound comp;
      builder.MakeCompound(comp);

      const gp_Pnt p_h0 = to_3d(m_pln, gp_Pnt2d(min_u, pt.Y()));
      const gp_Pnt p_h1 = to_3d(m_pln, gp_Pnt2d(max_u, pt.Y()));
      const gp_Pnt p_v0 = to_3d(m_pln, gp_Pnt2d(pt.X(), min_v));
      const gp_Pnt p_v1 = to_3d(m_pln, gp_Pnt2d(pt.X(), max_v));
      builder.Add(comp, BRepBuilderAPI_MakeEdge(p_h0, p_h1).Edge());
      builder.Add(comp, BRepBuilderAPI_MakeEdge(p_v0, p_v1).Edge());
      fullscreen_shape = comp;
    }
  }

  TopoDS_Shape anno_shape;
  const TopoDS_Shape traditional_shape = create_wire_box(m_pln, to_3d(m_pln, pt), snap_dist, snap_dist);
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

  if (m_snap_anno.IsNull())
  {
    m_snap_anno = new AIS_Shape(anno_shape);
    m_snap_anno->SetWidth(3.0);
    m_snap_anno->SetColor(
        Quantity_Color(s_snap_guide_color.x, s_snap_guide_color.y, s_snap_guide_color.z, Quantity_TOC_RGB));
    m_ctx.Display(m_snap_anno, true);
  }
  else
  {
    m_snap_anno->Set(anno_shape);
    m_ctx.Redisplay(m_snap_anno, true);
  }
}

void Sketch_nodes::update_axis_snap_anno_(int axis_index, const gp_Pnt2d& axis_pt, double snap_dist)
{
  const bool show_traditional =
      s_snap_guide_mode == Snap_guide_mode::Traditional || s_snap_guide_mode == Snap_guide_mode::Both;
  const bool show_fullscreen =
      s_snap_guide_mode == Snap_guide_mode::Fullscreen || s_snap_guide_mode == Snap_guide_mode::Both;

  TopoDS_Shape fullscreen_shape;
  if (show_fullscreen)
  {
    double min_u {}, min_v {}, max_u {}, max_v {};
    if (view_bounds_2d_(min_u, min_v, max_u, max_v))
    {
      if (axis_index == 0)
      {
        const gp_Pnt p0 = to_3d(m_pln, gp_Pnt2d(axis_pt.X(), min_v));
        const gp_Pnt p1 = to_3d(m_pln, gp_Pnt2d(axis_pt.X(), max_v));
        fullscreen_shape = BRepBuilderAPI_MakeEdge(p0, p1).Edge();
      }
      else
      {
        const gp_Pnt p0 = to_3d(m_pln, gp_Pnt2d(min_u, axis_pt.Y()));
        const gp_Pnt p1 = to_3d(m_pln, gp_Pnt2d(max_u, axis_pt.Y()));
        fullscreen_shape = BRepBuilderAPI_MakeEdge(p0, p1).Edge();
      }
    }
  }

  TopoDS_Shape anno_shape;
  const TopoDS_Shape traditional_shape =
      create_wire_box(m_pln, to_3d(m_pln, axis_pt), snap_dist * 0.5, snap_dist * 0.5);
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

  if (m_snap_anno_axis[axis_index].IsNull())
  {
    m_snap_anno_axis[axis_index] = new AIS_Shape(anno_shape);
    m_snap_anno_axis[axis_index]->SetWidth(3.0);
    m_snap_anno_axis[axis_index]->SetColor(
        Quantity_Color(s_snap_guide_color.x, s_snap_guide_color.y, s_snap_guide_color.z, Quantity_TOC_RGB));
    m_ctx.Display(m_snap_anno_axis[axis_index], true);
  }
  else
  {
    m_snap_anno_axis[axis_index]->Set(anno_shape);
    m_ctx.Redisplay(m_snap_anno_axis[axis_index], true);
  }
}

bool Sketch_nodes::try_snap_outside_(gp_Pnt2d& pt, const double snap_dist)
{
  gp_Pnt2d snapped;
  double   best_dist = std::numeric_limits<double>::max();
  for (const gp_Pnt2d& outside_pt : m_outside_snap_pts)
  {
    double dist = outside_pt.SquareDistance(pt);
    if (dist < best_dist)
    {
      best_dist = dist;
      snapped   = outside_pt;
    }
  }

  if (best_dist <= snap_dist * 0.25 * snap_dist)
  {
    pt = snapped;
    update_node_snap_anno_(pt, snap_dist);
    return true;
  }

  return false;
}

Sketch_nodes::Node& Sketch_nodes::operator[](size_t idx)
{
  EZY_ASSERT(idx < size());
  return m_nodes[idx];
}

const Sketch_nodes::Node& Sketch_nodes::operator[](size_t idx) const
{
  EZY_ASSERT(idx < size());
  return m_nodes[idx];
}

Sketch_nodes::Node& Sketch_nodes::operator[](const std::optional<size_t> idx)
{
  EZY_ASSERT(idx.has_value());
  EZY_ASSERT(*idx < size());
  return m_nodes[idx.value()];
}

const Sketch_nodes::Node& Sketch_nodes::operator[](const std::optional<size_t> idx) const
{
  EZY_ASSERT(idx.has_value());
  EZY_ASSERT(*idx < size());
  return m_nodes[idx.value()];
}

bool Sketch_nodes::empty() const
{
  return m_nodes.empty();
}

size_t Sketch_nodes::size() const
{
  return m_nodes.size();
}

void Sketch_nodes::json_resize(size_t count)
{
  m_nodes.assign(count, Node {});
}

void Sketch_nodes::json_set_node(size_t idx, const gp_Pnt2d& pt, bool deleted, bool midpoint, bool permanent, const std::string& name)
{
  EZY_ASSERT(idx < m_nodes.size());
  Node& n = m_nodes[idx];
  n.SetX(pt.X());
  n.SetY(pt.Y());
  n.deleted   = deleted;
  n.midpoint  = midpoint;
  n.permanent = permanent;
  n.name      = name;
}

void Sketch_nodes::finalize()
{
  m_prev_num_nodes = m_nodes.size();
}

void Sketch_nodes::cancel()
{
  m_nodes.resize(m_prev_num_nodes);
}

void Sketch_nodes::clear_outside_snap_pnts()
{
  m_outside_snap_pts.clear();
}

void Sketch_nodes::add_outside_snap_pnt(const gp_Pnt& pt3d)
{
  m_outside_snap_pts.insert(to_2d(m_pln, pt3d));
}

// Snap distance related
double                        Sketch_nodes::s_snap_dist_pixels = 35.0;
Sketch_nodes::Snap_guide_mode Sketch_nodes::s_snap_guide_mode  = Snap_guide_mode::Traditional;
glm::vec3                     Sketch_nodes::s_snap_guide_color {0.0f, 1.0f, 0.0f};

void Sketch_nodes::set_snap_dist(double snap_dist_pixels)
{
  s_snap_dist_pixels = snap_dist_pixels;
}

double Sketch_nodes::get_snap_dist()
{
  return s_snap_dist_pixels;
}

void Sketch_nodes::set_snap_guide_mode(Snap_guide_mode mode)
{
  s_snap_guide_mode = mode;
}

Sketch_nodes::Snap_guide_mode Sketch_nodes::get_snap_guide_mode()
{
  return s_snap_guide_mode;
}

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

