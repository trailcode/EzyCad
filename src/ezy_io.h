#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

class Ezy_asset_store;

inline constexpr const char* k_ezy_manifest_path = "manifest.json";
inline constexpr const char* k_ezy_assets_dir    = "assets/";

[[nodiscard]] bool is_ezy_zip(const std::string& bytes);

[[nodiscard]] bool is_ezy_json(const std::string& bytes);

struct Ezy_unpack_result
{
  std::string                                      manifest_json;
  std::unordered_map<std::string, std::vector<uint8_t>> assets; // asset_id -> raw RGBA bytes
};

/// Read a v3 zip `.ezy` archive. Returns nullopt on invalid zip or missing manifest.
[[nodiscard]] std::optional<Ezy_unpack_result> unpack_ezy(const std::string& bytes);

/// Build a v3 zip `.ezy` from manifest JSON and assets referenced by underlay `"asset"` fields.
[[nodiscard]] std::vector<uint8_t> pack_ezy(const std::string& manifest_json, const Ezy_asset_store& store);

/// Base64 encode/decode for binary startup project storage (Emscripten localStorage).
[[nodiscard]] std::string          ezy_base64_encode(const std::vector<uint8_t>& bytes);
[[nodiscard]] std::vector<uint8_t> ezy_base64_decode(const std::string& b64);
