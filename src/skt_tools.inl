#pragma once

// Template helpers for Sketch_tools. Include from skt_tools*.cpp after "skt.h".

template <typename Callback>
void Sketch_tools::add_sketch_pt_(const ScreenCoords& screen_coords, size_t required_num_pts, Callback&& callback)
{
  auto l = [&](const std::optional<size_t>& node_idx, const gp_Pnt2d& pt)
  {
    if (node_idx)
      m_tmp_node_idxs.push_back(*node_idx);
    else
      m_tmp_node_idxs.push_back(m_sketch.m_nodes.add_new_node(pt));

    if (m_tmp_node_idxs.size() >= required_num_pts)
      callback(m_tmp_node_idxs.back());
  };

  move_sketch_pt_(screen_coords, l);
}

template <typename Callback> void Sketch_tools::move_sketch_pt_(const ScreenCoords& screen_coords, Callback&& callback)
{
  m_last_pt = m_sketch.m_view.pt_on_plane(screen_coords, m_sketch.m_pln);
  if (!m_last_pt)
    // View plane and sketch plane must be perpendicular.
    return;

  std::optional<size_t> node_idx = m_sketch.m_nodes.try_get_node_idx_snap(*m_last_pt);

  callback(node_idx, *m_last_pt);
}

/// Invokes callback(e, pt_a, pt_b) with the last tmp edge only when it exists and is non-degenerate.
template <typename Callback> void Sketch_tools::if_edge_pt_valid_(Callback&& callback)
{
  if (m_tmp_edges.empty())
    return;

  Sketch_edge&    e    = m_tmp_edges.back();
  const gp_Pnt2d& pt_a = m_sketch.m_nodes[e.node_idx_a];
  if (m_last_pt.has_value())
    if (unique(pt_a, *m_last_pt))
      callback(e, pt_a, *m_last_pt);
}
