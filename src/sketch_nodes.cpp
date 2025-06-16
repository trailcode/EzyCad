#include "sketch_nodes.h"
#include "occt_view.h"
#include "geom.h"

#include <TopoDS_Wire.hxx>

Sketch_nodes::Sketch_nodes(Occt_view& view, const gp_Pln& pln)
    : m_view(view), m_ctx(m_view.ctx()), m_pln(pln)
{

}

Sketch_nodes::~Sketch_nodes()
{
  hide_snap_annos(); // Deletes them from context
}

std::optional<gp_Pnt2d> Sketch_nodes::snap(const ScreenCoords& screen_coords)
{
  std::optional<gp_Pnt2d> pt = m_view.pt_on_plane(screen_coords, m_pln);
  if (pt)
    try_get_node_idx_snap(*pt);
  else
  {
    // TODO use result
  }

  return pt;
}

size_t Sketch_nodes::get_node_exact(const gp_Pnt2d& pt)
{
  for (size_t idx = 0, num = m_nodes.size(); idx < num; ++idx)
    if (equal(pt, gp_Pnt2d(m_nodes[idx])))
      return idx;

  size_t ret = m_nodes.size();
  m_nodes.push_back({pt});
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

std::optional<size_t> Sketch_nodes::try_get_node_idx_snap(
    gp_Pnt2d&                  pt,  // `pt` could be snapped to a node, an axis of another node, or an outside snap point.
    const std::vector<size_t>& to_exclude)
{
  // Calculate snap_dist in world coordinates
  double       snap_dist;
  if (!m_view.is_headless())
  {
    gp_Pnt       pt3d_on_plane       = to_3d(m_pln, pt);
    ScreenCoords screen_coords_at_pt = m_view.get_screen_coords(pt3d_on_plane);

    ScreenCoords screen_coords_offset = screen_coords_at_pt;
    // Offset by m_snap_dist_pixels in screen space (e.g., along X)
    // It might be more robust to offset in both X and Y by m_snap_dist_pixels / sqrt(2)
    // or handle cases where the offset point goes off-screen,
    // but for simplicity, a simple X offset is shown here.
    screen_coords_offset.unsafe_get().x += s_snap_dist_pixels;

    std::optional<gp_Pnt2d> pt_offset_on_plane_2d;
    if (std::optional<gp_Pnt> pt_offset_on_plane_3d = m_view.pt3d_on_plane(screen_coords_offset, m_pln))
    {
      pt_offset_on_plane_2d = to_2d(m_pln, *pt_offset_on_plane_3d);
    }

    if (pt_offset_on_plane_2d)
      snap_dist = pt.Distance(*pt_offset_on_plane_2d);
    else
      // It should never get here
      snap_dist = 5.0;  // This is not ideal as a fallback
  }
  else
    // Headless, probably a unit test calling, treating s_snap_dist_pixels as world units.
    snap_dist = s_snap_dist_pixels;

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
    pt = m_nodes[best_idx];
    update_node_snap_anno_(m_nodes[best_idx], sqrt(snap_dist));
    return best_idx;
  }

  // Try to snap to outside points or do outside axis snap. `pt` might be modified.
  if (try_snap_outside_(pt, sqrt(snap_dist)))
    return {};

  hide_snap_annos();

  gp_Pnt2d pt_origional = pt;

  for (int i = 0; i < 2; ++i)
  {
    std::optional<gp_Pnt2d> snap_axis_point;
    best_dist = std::numeric_limits<double>::max();

    auto try_nd_pt = [&](const gp_Pnt2d& nd_pt)
    {
      double dist      = pt_origional.SquareDistance(nd_pt);
      // axis_dist needs to be compared against a linear snap distance in screen pixels.
      // This part becomes tricky because axis_dist is a world coordinate difference.
      // We'd ideally convert m_snap_dist_pixels * 0.5 to a world distance along the axis.
      // For simplicity here, we'll continue using a fraction of the calculated world snap_dist_sq,
      // but this might need refinement for axis snapping to feel right.
      // The original logic used snap_dist (linear pixels) * 0.5 for axis snapping.
      // We need a world-space equivalent for axis snapping.
      // Let's use sqrt(snap_dist_sq) * 0.5 for now.
      double axis_snap_threshold_world = sqrt(snap_dist) * 0.5;
      double axis_dist = std::fabs(pt_origional.XY().Coord(i + 1) - nd_pt.XY().Coord(i + 1));

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
    {
      // create_wire_box and update_node_snap_anno_ expect linear distances.
      TopoDS_Wire box = create_wire_box(m_pln, to_3d(m_pln, *snap_axis_point), sqrt(snap_dist) * 0.5, sqrt(snap_dist) * 0.5);
      if (m_snap_anno_axis[i].IsNull())
      {
        m_snap_anno_axis[i] = new AIS_Shape(box);
        m_snap_anno_axis[i]->SetWidth(3.0);
        m_snap_anno_axis[i]->SetColor(Quantity_Color(1, 0.5, 0.7, Quantity_TOC_RGB));
        m_ctx.Display(m_snap_anno_axis[i], true);
      }
      else
      {
        m_snap_anno_axis[i]->Set(box);
        m_ctx.Redisplay(m_snap_anno_axis[i], true);
      }
    }
    else
    {
      if (!m_snap_anno_axis[i].IsNull())
        m_ctx.Erase(m_snap_anno_axis[i], true);
    }
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

size_t Sketch_nodes::add_new_node(const gp_Pnt2d& pt, bool is_edge_mid_point)
{
  size_t ret = m_nodes.size();
  Node   n {pt};
  n.is_midpoint = is_edge_mid_point;
  m_nodes.emplace_back(n);
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

  TopoDS_Wire box = create_wire_box(m_pln, to_3d(m_pln, pt), snap_dist, snap_dist);

  if (m_snap_anno.IsNull())
  {
    m_snap_anno = new AIS_Shape(box);
    m_snap_anno->SetWidth(3.0);
    m_snap_anno->SetColor(Quantity_Color(0, 0.5, 0.7, Quantity_TOC_RGB));
    m_ctx.Display(m_snap_anno, true);
  }
  else
  {
    m_snap_anno->Set(box);
    m_ctx.Redisplay(m_snap_anno, true);
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
double Sketch_nodes::s_snap_dist_pixels = 35.0;

void Sketch_nodes::set_snap_dist(double snap_dist_pixels)
{
  s_snap_dist_pixels = snap_dist_pixels;
}

double Sketch_nodes::get_snap_dist()
{
  return s_snap_dist_pixels;
}