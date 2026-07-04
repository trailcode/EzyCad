#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

/// Session-lifetime deduplicated RGBA blobs referenced from sketch underlay JSON.
class Ezy_asset_store
{
public:
  /// Content-addressed id (FNV-1a 64-bit hex) from w, h, and pixel bytes.
  [[nodiscard]] std::string register_rgba(const std::vector<uint8_t>& rgba, int w, int h);

  [[nodiscard]] std::shared_ptr<const std::vector<uint8_t>> get(const std::string& asset_id) const;

  /// Insert or replace bytes for \a asset_id (e.g. when loading a zip archive).
  void import_asset(const std::string& asset_id, std::vector<uint8_t>&& rgba);

  void clear();

  [[nodiscard]] static std::string make_asset_id(const uint8_t* rgba, std::size_t len, int w, int h);

private:
  std::unordered_map<std::string, std::shared_ptr<const std::vector<uint8_t>>> m_by_id;
};
