#include "utl.h"

#include <GLFW/glfw3.h>

#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <set>
#include <tuple>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

std::string unique_sequential_name(const std::string& base_name, std::span<const std::string> existing_names)
{
  std::set<int>     used;
  const std::string prefix = base_name + ".";

  for (const std::string& n : existing_names)
  {
    if (n == base_name)
      used.insert(0);

    else if (n.size() > prefix.size() && n.compare(0, prefix.size(), prefix) == 0)
    {
      const std::string suffix = n.substr(prefix.size());
      if (!suffix.empty() &&
          std::all_of(suffix.begin(), suffix.end(), [](char c) { return std::isdigit(static_cast<unsigned char>(c)); }))
      {
        int idx = 0;
        try
        {
          idx = std::stoi(suffix);
          if (idx >= 0)
            used.insert(idx);
        }
        catch (...)
        {
          EZY_ASSERT(false);
        }
      }
    }
  }

  int next = 0;
  while (used.count(next))
    ++next;

  if (next == 0)
    return base_name;

  char buf[32];
  snprintf(buf, sizeof(buf), ".%03d", next);
  return base_name + buf;
}

uint32_t load_texture(const std::string& path)
{
  int width, height, channels;
  EZY_ASSERT_MSG(std::filesystem::exists(path), "Error cannot open: " + std::string(path));
  unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4); // Force RGBA
  EZY_ASSERT_MSG(data, "l");

  GLuint texture_id;
  glGenTextures(1, &texture_id);
  glBindTexture(GL_TEXTURE_2D, texture_id);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP); // Using GL_CLAMP
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

  stbi_image_free(data);
  return texture_id;
}

std::optional<std::tuple<std::vector<uint8_t>, int, int>> decode_image_bytes(const std::string& file_bytes)
{
  if (file_bytes.empty())
    return std::nullopt;
  int            w = 0, h = 0, ch = 0;
  unsigned char* data = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(file_bytes.data()),
                                              static_cast<int>(file_bytes.size()), &w, &h, &ch, 4);
  if (!data || w <= 0 || h <= 0)
  {
    if (data)
      stbi_image_free(data);
    return std::nullopt;
  }
  const std::size_t    n = static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4u;
  std::vector<uint8_t> rgba(n);
  std::memcpy(rgba.data(), data, n);
  stbi_image_free(data);
  return std::tuple{std::move(rgba), w, h};
}

std::size_t Pair_hash::operator()(const std::pair<size_t, size_t>& p) const
{
  std::size_t seed = 0;
  seed ^= std::hash<size_t>{}(p.first) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  seed ^= std::hash<size_t>{}(p.second) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  return seed;
}

// Portable safe copy for fixed-size char buffers used with ImGui::InputText.
// Uses strncpy_s (with _TRUNCATE) on MSVC to avoid C4996; falls back elsewhere.
void safe_cstr_copy(char* dest, size_t dest_size, const char* src)
{
#ifdef _MSC_VER
  strncpy_s(dest, dest_size, src, _TRUNCATE);
#else
  std::strncpy(dest, src, dest_size - 1);
  dest[dest_size - 1] = '\0';
#endif
}