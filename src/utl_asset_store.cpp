#include "utl_asset_store.h"

#include <iomanip>
#include <sstream>

namespace
{

uint64_t fnv1a64_feed(uint64_t hash, const uint8_t* data, std::size_t len)
{
  for (std::size_t i = 0; i < len; ++i)
  {
    hash ^= static_cast<uint64_t>(data[i]);
    hash *= 1099511628211ULL;
  }

  return hash;
}

} // namespace

std::string Ezy_asset_store::make_asset_id(const uint8_t* rgba, std::size_t len, int w, int h)
{
  uint64_t hash = 14695981039346656037ULL;
  hash          = fnv1a64_feed(hash, reinterpret_cast<const uint8_t*>(&w), sizeof(w));
  hash          = fnv1a64_feed(hash, reinterpret_cast<const uint8_t*>(&h), sizeof(h));
  if (rgba && len > 0)
    hash = fnv1a64_feed(hash, rgba, len);

  std::ostringstream oss;
  oss << std::hex << std::setfill('0') << std::setw(16) << hash;
  return oss.str();
}

std::string Ezy_asset_store::register_rgba(const std::vector<uint8_t>& rgba, int w, int h)
{
  const std::string id = make_asset_id(rgba.data(), rgba.size(), w, h);
  if (m_by_id.find(id) == m_by_id.end())
    m_by_id[id] = std::make_shared<const std::vector<uint8_t>>(rgba);

  return id;
}

std::shared_ptr<const std::vector<uint8_t>> Ezy_asset_store::get(const std::string& asset_id) const
{
  const auto it = m_by_id.find(asset_id);
  if (it == m_by_id.end())
    return {};

  return it->second;
}

void Ezy_asset_store::import_asset(const std::string& asset_id, std::vector<uint8_t>&& rgba)
{
  m_by_id[asset_id] = std::make_shared<const std::vector<uint8_t>>(std::move(rgba));
}

void Ezy_asset_store::clear() { m_by_id.clear(); }
