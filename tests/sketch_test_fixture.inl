template <typename Callback>
void Sketch_test::for_all_edge_permutations_(const std::vector<gp_Pnt2d>&            points,
                                             const std::vector<std::pair<int, int>>& edge_indices, const gp_Pln& plane,
                                             Callback&& callback)
{
  std::vector<std::pair<int, int>> edges = edge_indices;

  // Generate all permutations of edge order
  do
  {
    // For each permutation, generate all combinations of orientations
    size_t n = edges.size();
    for (size_t mask = 0; mask < (1ull << n); ++mask)
    {
      // Prepare oriented edges for this combination
      std::vector<std::pair<int, int>> oriented_edges = edges;
      for (size_t i = 0; i < n; ++i)
        if (mask & (1ull << i))
          std::swap(oriented_edges[i].first, oriented_edges[i].second);

      // Create a new sketch and add edges in this order/orientation
      Sketch sketch("test_sketch", view(), plane);
      for (const auto& [a, b] : oriented_edges)
        Sketch_access::add_edge_(sketch, points[a], points[b]);

      callback(sketch);
    }
  } while (std::next_permutation(edges.begin(), edges.end()));
}

template <typename Callback>
void Sketch_test::add_edges_from_indices_(const std::vector<gp_Pnt2d>&            points,
                                          const std::vector<std::pair<int, int>>& edge_indices, const gp_Pln& plane,
                                          Callback&& callback)
{
  Sketch sketch("test_sketch", view(), plane);
  for (const auto& [i, j] : edge_indices)
    Sketch_access::add_edge_(sketch, points[i], points[j]);

  callback(sketch);
}
