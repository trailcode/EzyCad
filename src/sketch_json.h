#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <memory>

class Sketch;
class Occt_view;

class Sketch_json
{
 public:
  static nlohmann::json to_json(const Sketch& sketch);

  static std::shared_ptr<Sketch> from_json(Occt_view& view, const nlohmann::json& j);
 private:
  Sketch_json() = default;
  ~Sketch_json() = default;
};