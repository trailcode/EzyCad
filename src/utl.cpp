#include <GLFW/glfw3.h>

#include "utl.h"

#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

uint32_t load_texture(const std::string& path)
{
  int width, height, channels;
  EZY_ASSERT_MSG(std::filesystem::exists(path), "Error cannot open: " + std::string(path));
  unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);  // Force RGBA
  EZY_ASSERT_MSG(data, "l");

  GLuint texture_id;
  glGenTextures(1, &texture_id);
  glBindTexture(GL_TEXTURE_2D, texture_id);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);  // Using GL_CLAMP
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

  stbi_image_free(data);
  return texture_id;
}

std::size_t Pair_hash::operator()(const std::pair<size_t, size_t>& p) const
{
  std::size_t seed = 0;
  seed ^= std::hash<size_t> {}(p.first) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  seed ^= std::hash<size_t> {}(p.second) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  return seed;
}