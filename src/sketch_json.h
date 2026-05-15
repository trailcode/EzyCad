#pragma once

#include <cstddef>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>
#include <vector>

class Sketch;
class Occt_view;

class Sketch_json
{
public:
  static nlohmann::json to_json(const Sketch& sketch);

  static std::shared_ptr<Sketch> from_json(Occt_view& view, const nlohmann::json& j);

private:
  Sketch_json()  = default;
  ~Sketch_json() = default;

  static void load_nodes_(Sketch& sketch, const nlohmann::json& nodes_json);
  static void from_json_indexed_(Sketch& ret, const nlohmann::json& j,
                                 std::vector<std::pair<std::size_t, std::size_t>>* out_legacy_length_dim_endpoints);
  static void from_json_legacy_coords_(Sketch& ret, const nlohmann::json& j,
                                       std::vector<std::pair<std::size_t, std::size_t>>* out_legacy_length_dim_endpoints);
  static bool edges_use_node_indices_(const nlohmann::json& j);
};