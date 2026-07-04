#include "ezy_io.h"

#include "ezy_asset_store.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <string_view>
#include <vector>

namespace
{

constexpr uint32_t k_zip_local_sig  = 0x04034b50u;
constexpr uint32_t k_zip_central_sig = 0x02014b50u;
constexpr uint32_t k_zip_eocd_sig   = 0x06054b50u;

struct Zip_entry
{
  std::string name;
  std::string data;
};

uint32_t crc32_bytes(const uint8_t* data, std::size_t len)
{
  static uint32_t table[256];
  static bool     init = false;
  if (!init)
  {
    for (uint32_t i = 0; i < 256; ++i)
    {
      uint32_t c = i;
      for (int k = 0; k < 8; ++k)
        c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
      table[i] = c;
    }
    init = true;
  }

  uint32_t crc = 0xFFFFFFFFu;
  for (std::size_t i = 0; i < len; ++i)
    crc = table[(crc ^ data[i]) & 0xFFu] ^ (crc >> 8);
  return crc ^ 0xFFFFFFFFu;
}

void write_u16(std::vector<uint8_t>& out, uint16_t v)
{
  out.push_back(static_cast<uint8_t>(v & 0xFFu));
  out.push_back(static_cast<uint8_t>((v >> 8) & 0xFFu));
}

void write_u32(std::vector<uint8_t>& out, uint32_t v)
{
  out.push_back(static_cast<uint8_t>(v & 0xFFu));
  out.push_back(static_cast<uint8_t>((v >> 8) & 0xFFu));
  out.push_back(static_cast<uint8_t>((v >> 16) & 0xFFu));
  out.push_back(static_cast<uint8_t>((v >> 24) & 0xFFu));
}

bool read_u16(const uint8_t* p, std::size_t avail, std::size_t& off, uint16_t& v)
{
  if (off + 2 > avail)
    return false;
  v = static_cast<uint16_t>(p[off]) | (static_cast<uint16_t>(p[off + 1]) << 8);
  off += 2;
  return true;
}

bool read_u32(const uint8_t* p, std::size_t avail, std::size_t& off, uint32_t& v)
{
  if (off + 4 > avail)
    return false;
  v = static_cast<uint32_t>(p[off]) | (static_cast<uint32_t>(p[off + 1]) << 8) |
      (static_cast<uint32_t>(p[off + 2]) << 16) | (static_cast<uint32_t>(p[off + 3]) << 24);
  off += 4;
  return true;
}

std::vector<uint8_t> zip_write_stored(const std::vector<Zip_entry>& entries)
{
  std::vector<uint8_t> out;
  struct Local_rec
  {
    uint32_t offset;
    uint16_t name_len;
    uint32_t crc;
    uint32_t size;
    std::string name;
  };
  std::vector<Local_rec> locals;

  for (const Zip_entry& e : entries)
  {
    Local_rec rec;
    rec.offset   = static_cast<uint32_t>(out.size());
    rec.name     = e.name;
    rec.name_len = static_cast<uint16_t>(e.name.size());
    rec.size     = static_cast<uint32_t>(e.data.size());
    rec.crc      = crc32_bytes(reinterpret_cast<const uint8_t*>(e.data.data()), e.data.size());

    write_u32(out, k_zip_local_sig);
    write_u16(out, 20); // version needed to extract
    write_u16(out, 0);  // flags
    write_u16(out, 0);  // compression: store
    write_u16(out, 0);  // mod time
    write_u16(out, 0);  // mod date
    write_u32(out, rec.crc);
    write_u32(out, rec.size);
    write_u32(out, rec.size);
    write_u16(out, rec.name_len);
    write_u16(out, 0); // extra len
    out.insert(out.end(), e.name.begin(), e.name.end());
    out.insert(out.end(), e.data.begin(), e.data.end());
    locals.push_back(std::move(rec));
  }

  const uint32_t central_start = static_cast<uint32_t>(out.size());
  for (const Local_rec& rec : locals)
  {
    write_u32(out, k_zip_central_sig);
    write_u16(out, 20); // version made by
    write_u16(out, 20); // version needed
    write_u16(out, 0);
    write_u16(out, 0);
    write_u16(out, 0);
    write_u16(out, 0);
    write_u32(out, rec.crc);
    write_u32(out, rec.size);
    write_u32(out, rec.size);
    write_u16(out, rec.name_len);
    write_u16(out, 0); // extra
    write_u16(out, 0); // comment
    write_u16(out, 0); // disk start
    write_u16(out, 0); // int attrs
    write_u32(out, 0); // ext attrs
    write_u32(out, rec.offset);
    out.insert(out.end(), rec.name.begin(), rec.name.end());
  }

  const uint32_t central_size = static_cast<uint32_t>(out.size()) - central_start;
  write_u32(out, k_zip_eocd_sig);
  write_u16(out, 0); // disk
  write_u16(out, 0); // disk with central
  write_u16(out, static_cast<uint16_t>(locals.size()));
  write_u16(out, static_cast<uint16_t>(locals.size()));
  write_u32(out, central_size);
  write_u32(out, central_start);
  write_u16(out, 0); // comment len

  return out;
}

bool zip_read_stored(const std::string& bytes, std::vector<Zip_entry>& out_entries)
{
  out_entries.clear();
  if (bytes.size() < 22)
    return false;

  const uint8_t* data  = reinterpret_cast<const uint8_t*>(bytes.data());
  const std::size_t n  = bytes.size();
  std::size_t       eocd = std::string::npos;

  for (std::size_t i = n; i >= 4; --i)
  {
    const std::size_t at = i - 4;
    if (data[at] == 0x50 && data[at + 1] == 0x4b && data[at + 2] == 0x05 && data[at + 3] == 0x06)
    {
      eocd = at;
      break;
    }
    if (i == 4)
      break;
  }

  if (eocd == std::string::npos)
    return false;

  std::size_t off = eocd + 4;
  uint16_t    disk_num, disk_with_cd, num_entries_cd, num_entries_total;
  uint32_t    cd_size, cd_offset;
  if (!read_u16(data, n, off, disk_num) || !read_u16(data, n, off, disk_with_cd) || !read_u16(data, n, off, num_entries_cd) ||
      !read_u16(data, n, off, num_entries_total) || !read_u32(data, n, off, cd_size) || !read_u32(data, n, off, cd_offset))
    return false;

  if (num_entries_cd != num_entries_total || cd_offset + cd_size > n)
    return false;

  off = cd_offset;
  for (uint16_t i = 0; i < num_entries_cd; ++i)
  {
    uint32_t sig;
    if (!read_u32(data, n, off, sig) || sig != k_zip_central_sig)
      return false;

    uint16_t ver_made, ver_need, flags, method, mod_t, mod_d, name_len, extra_len, comment_len, disk_start, int_attr;
    uint32_t crc, comp_size, uncomp_size, ext_attr, local_offset;
    if (!read_u16(data, n, off, ver_made) || !read_u16(data, n, off, ver_need) || !read_u16(data, n, off, flags) ||
        !read_u16(data, n, off, method) || !read_u16(data, n, off, mod_t) || !read_u16(data, n, off, mod_d) ||
        !read_u32(data, n, off, crc) || !read_u32(data, n, off, comp_size) || !read_u32(data, n, off, uncomp_size) ||
        !read_u16(data, n, off, name_len) || !read_u16(data, n, off, extra_len) || !read_u16(data, n, off, comment_len) ||
        !read_u16(data, n, off, disk_start) || !read_u16(data, n, off, int_attr) || !read_u32(data, n, off, ext_attr) ||
        !read_u32(data, n, off, local_offset))
      return false;

    if (off + name_len + extra_len + comment_len > n)
      return false;

    const std::string name(reinterpret_cast<const char*>(data + off), name_len);
    off += name_len + extra_len + comment_len;

    if (method != 0 || comp_size != uncomp_size)
      return false;

    if (local_offset + 30u > n)
      return false;
    std::size_t loc = local_offset + 4u;
    uint16_t    loc_ver, loc_flags, loc_method, loc_mod_t, loc_mod_d, loc_name_len, loc_extra_len;
    uint32_t    loc_crc, loc_comp, loc_uncomp;
    if (!read_u16(data, n, loc, loc_ver) || !read_u16(data, n, loc, loc_flags) || !read_u16(data, n, loc, loc_method) ||
        !read_u16(data, n, loc, loc_mod_t) || !read_u16(data, n, loc, loc_mod_d) || !read_u32(data, n, loc, loc_crc) ||
        !read_u32(data, n, loc, loc_comp) || !read_u32(data, n, loc, loc_uncomp) || !read_u16(data, n, loc, loc_name_len) ||
        !read_u16(data, n, loc, loc_extra_len))
      return false;

    const std::size_t data_start = local_offset + 30u + loc_name_len + loc_extra_len;
    if (data_start + comp_size > n)
      return false;

    Zip_entry entry;
    entry.name = name;
    entry.data.assign(reinterpret_cast<const char*>(data + data_start), comp_size);
    out_entries.push_back(std::move(entry));
  }

  return true;
}

void collect_underlay_asset_ids(const nlohmann::json& j, std::vector<std::string>& out)
{
  if (!j.contains("sketches") || !j["sketches"].is_array())
    return;

  for (const nlohmann::json& sk : j["sketches"])
  {
    if (!sk.contains("underlay") || !sk["underlay"].is_object())
      continue;
    const nlohmann::json& ul = sk["underlay"];
    if (ul.contains("asset") && ul["asset"].is_string())
      out.push_back(ul["asset"].get<std::string>());
  }
}

std::string asset_path(const std::string& asset_id) { return std::string(k_ezy_assets_dir) + asset_id + ".rgba"; }

bool parse_asset_path(std::string_view path, std::string& out_id)
{
  const std::string_view prefix = k_ezy_assets_dir;
  const std::string_view suffix = ".rgba";
  if (path.size() <= prefix.size() + suffix.size())
    return false;
  if (path.substr(0, prefix.size()) != prefix)
    return false;
  if (path.substr(path.size() - suffix.size()) != suffix)
    return false;
  out_id.assign(path.substr(prefix.size(), path.size() - prefix.size() - suffix.size()));
  return !out_id.empty();
}

int from_b64(char c)
{
  if (c >= 'A' && c <= 'Z')
    return c - 'A';
  if (c >= 'a' && c <= 'z')
    return c - 'a' + 26;
  if (c >= '0' && c <= '9')
    return c - '0' + 52;
  if (c == '+')
    return 62;
  if (c == '/')
    return 63;
  return -1;
}

} // namespace

bool is_ezy_zip(const std::string& bytes)
{
  return bytes.size() >= 4 && bytes[0] == 'P' && bytes[1] == 'K' && bytes[2] == 0x03 && bytes[3] == 0x04;
}

bool is_ezy_json(const std::string& bytes)
{
  std::size_t i = 0;
  while (i < bytes.size() && (bytes[i] == ' ' || bytes[i] == '\t' || bytes[i] == '\r' || bytes[i] == '\n'))
    ++i;
  return i < bytes.size() && bytes[i] == '{';
}

std::optional<Ezy_unpack_result> unpack_ezy(const std::string& bytes)
{
  std::vector<Zip_entry> entries;
  if (!zip_read_stored(bytes, entries))
    return std::nullopt;

  Ezy_unpack_result result;
  for (Zip_entry& e : entries)
  {
    if (e.name == k_ezy_manifest_path)
    {
      result.manifest_json = std::move(e.data);
      continue;
    }

    std::string asset_id;
    if (parse_asset_path(e.name, asset_id))
      result.assets.emplace(std::move(asset_id), std::vector<uint8_t>(e.data.begin(), e.data.end()));
  }

  if (result.manifest_json.empty())
    return std::nullopt;

  return result;
}

std::vector<uint8_t> pack_ezy(const std::string& manifest_json, const Ezy_asset_store& store)
{
  std::vector<std::string> asset_ids;
  try
  {
    const nlohmann::json j = nlohmann::json::parse(manifest_json);
    collect_underlay_asset_ids(j, asset_ids);
  }
  catch (...)
  {
    return {};
  }

  std::sort(asset_ids.begin(), asset_ids.end());
  asset_ids.erase(std::unique(asset_ids.begin(), asset_ids.end()), asset_ids.end());

  std::vector<Zip_entry> entries;
  entries.push_back({k_ezy_manifest_path, manifest_json});

  for (const std::string& id : asset_ids)
  {
    const auto pixels = store.get(id);
    if (!pixels)
      continue;
    entries.push_back({asset_path(id), std::string(reinterpret_cast<const char*>(pixels->data()), pixels->size())});
  }

  return zip_write_stored(entries);
}

std::string ezy_base64_encode(const std::vector<uint8_t>& bytes)
{
  static const char tbl[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string       out;
  const std::size_t len = bytes.size();
  out.reserve(((len + 2) / 3) * 4);
  for (std::size_t i = 0; i < len; i += 3)
  {
    const std::size_t n      = len - i;
    const unsigned    b0     = bytes[i];
    const unsigned    b1     = n > 1 ? bytes[i + 1] : 0u;
    const unsigned    b2     = n > 2 ? bytes[i + 2] : 0u;
    const unsigned    triple = (b0 << 16) | (b1 << 8) | b2;
    out.push_back(tbl[(triple >> 18) & 63]);
    out.push_back(tbl[(triple >> 12) & 63]);
    out.push_back(n > 1 ? tbl[(triple >> 6) & 63] : '=');
    out.push_back(n > 2 ? tbl[triple & 63] : '=');
  }
  return out;
}

std::vector<uint8_t> ezy_base64_decode(const std::string& b64)
{
  std::vector<uint8_t> out;
  if (b64.empty())
    return out;

  std::size_t len = b64.size();
  while (len > 0 && (b64[len - 1] == '=' || b64[len - 1] == '\n' || b64[len - 1] == '\r' || b64[len - 1] == ' '))
    --len;

  out.reserve((len * 3) / 4);
  unsigned buf  = 0;
  int      bits = 0;
  for (std::size_t i = 0; i < len; ++i)
  {
    const char c = b64[i];
    if (c == '\n' || c == '\r' || c == ' ')
      continue;
    const int v = from_b64(c);
    if (v < 0)
      return {};
    buf = (buf << 6) | static_cast<unsigned>(v);
    bits += 6;
    if (bits >= 8)
    {
      bits -= 8;
      out.push_back(static_cast<uint8_t>((buf >> bits) & 0xFF));
    }
  }
  return out;
}
