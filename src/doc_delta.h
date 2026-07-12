#pragma once

#include "delta.h"

#include <cstdint>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

/// Add or remove one sketch (full per-sketch JSON), without copying the whole document.
class Sketch_struct_delta : public Delta
{
public:
  enum class Kind
  {
    Add,
    Remove
  };

  Sketch_struct_delta(Kind kind, nlohmann::json sketch_json, bool was_current,
                      std::optional<nlohmann::json> auto_created_default_json = std::nullopt);

  void                   apply_forward(Occt_view& view) override;
  void                   apply_reverse(Occt_view& view) override;
  std::unique_ptr<Delta> clone() const override;

private:
  void add_sketch_(Occt_view& view, const nlohmann::json& sketch_json, bool make_current) const;
  void remove_sketch_by_id_(Occt_view& view, size_t sketch_id) const;

  Kind                          m_kind;
  nlohmann::json                m_sketch_json;
  bool                          m_was_current{false};
  std::optional<nlohmann::json> m_auto_created_default_json;
};

/// Underlay placement / image change on one sketch (asset pixels stay in the store).
class Underlay_delta : public Delta
{
public:
  Underlay_delta(size_t sketch_id, nlohmann::json before, nlohmann::json after);

  void                   apply_forward(Occt_view& view) override;
  void                   apply_reverse(Occt_view& view) override;
  std::unique_ptr<Delta> clone() const override;

private:
  void apply_json_(Occt_view& view, const nlohmann::json& j) const;

  size_t         m_sketch_id{0};
  nlohmann::json m_before;
  nlohmann::json m_after;
};

/// Runs child deltas in order (forward) / reverse order (reverse).
class Composite_delta : public Delta
{
public:
  explicit Composite_delta(std::vector<std::unique_ptr<Delta>> parts);

  void                   apply_forward(Occt_view& view) override;
  void                   apply_reverse(Occt_view& view) override;
  std::unique_ptr<Delta> clone() const override;

private:
  std::vector<std::unique_ptr<Delta>> m_parts;
};
