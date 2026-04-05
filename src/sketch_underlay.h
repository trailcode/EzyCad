#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <Standard_Handle.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Vec2d.hxx>
#include <nlohmann/json.hpp>

class AIS_TexturedShape;
class AIS_InteractiveContext;
class gp_Pln;

/// Raster image drawn in the sketch plane (below sketch edges) for tracing / digitizing.
class Sketch_underlay
{
 public:
  Sketch_underlay() = default;

  Sketch_underlay(const Sketch_underlay&)            = delete;
  Sketch_underlay& operator=(const Sketch_underlay&) = delete;
  Sketch_underlay(Sketch_underlay&&)                 = delete;
  Sketch_underlay& operator=(Sketch_underlay&&)      = delete;

  [[nodiscard]] bool has_image() const
  {
    return !m_rgba.empty() && m_w > 0 && m_h > 0;
  }

  /// Replace image (RGBA8). Sets a default axis-aligned rectangle in plane coords (0.1 unit per pixel).
  [[nodiscard]] bool set_image_rgba(std::vector<uint8_t>&& rgba, int w, int h);

  void set_affine(const gp_Pnt2d& base, const gp_Vec2d& axis_u, const gp_Vec2d& axis_v);
  void set_opacity(float opaque01);
  void set_visible(bool v);
  /// When true (default), bright pixels (white paper) become transparent in the texture; dark linework stays opaque.
  void set_key_white_transparent(bool on);
  [[nodiscard]] bool key_white_transparent() const { return m_key_white_transparent; }

  void set_line_tint_enabled(bool on);
  void set_line_tint_rgb(uint8_t r, uint8_t g, uint8_t b);
  [[nodiscard]] bool     line_tint_enabled() const { return m_line_tint_enabled; }
  void                   line_tint_rgb(uint8_t& r, uint8_t& g, uint8_t& b) const;

  [[nodiscard]] float       opacity() const { return m_opacity; }
  [[nodiscard]] bool        visible() const { return m_visible; }
  [[nodiscard]] gp_Pnt2d    base() const { return m_base; }
  [[nodiscard]] gp_Vec2d    axis_u() const { return m_axis_u; }
  [[nodiscard]] gp_Vec2d    axis_v() const { return m_axis_v; }
  [[nodiscard]] int         image_w() const { return m_w; }
  [[nodiscard]] int         image_h() const { return m_h; }

  void rebuild_and_display(const gp_Pln& pln, AIS_InteractiveContext& ctx);
  void erase(AIS_InteractiveContext& ctx);
  void sync_visibility(const gp_Pln& pln, AIS_InteractiveContext& ctx);

  nlohmann::json to_json() const;
  /// Returns false if JSON is invalid or image decode fails.
  [[nodiscard]] bool from_json(const nlohmann::json& j);

 private:
  void remove_ais_(AIS_InteractiveContext& ctx);
  void build_ais_(const gp_Pln& pln, AIS_InteractiveContext& ctx);

  std::vector<uint8_t> m_rgba;
  int                  m_w {0};
  int                  m_h {0};

  gp_Pnt2d m_base {0., 0.};
  gp_Vec2d m_axis_u {100., 0.};
  gp_Vec2d m_axis_v {0., 100.};

  float m_opacity {0.88f};
  bool  m_visible {true};
  /// Luminance key applied only when building the GPU texture (stored pixels stay raw).
  bool  m_key_white_transparent {true};
  /// Recolor pixels that remain visible (alpha > 0 after key) for contrast on dark views.
  bool     m_line_tint_enabled {true};
  uint8_t  m_tint_r {255};
  uint8_t  m_tint_g {220};
  uint8_t  m_tint_b {0};

  opencascade::handle<AIS_TexturedShape> m_ais;
};
