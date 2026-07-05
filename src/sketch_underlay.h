#pragma once

#include <Standard_Handle.hxx>
#include <cstdint>
#include <glm/glm.hpp>
#include <gp_Pnt2d.hxx>
#include <gp_Vec2d.hxx>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

class AIS_InteractiveObject;
class AIS_TexturedShape;
class AIS_Shape;
class AIS_InteractiveContext;
class gp_Pln;
class Ezy_asset_store;

/// Raster image drawn in the sketch plane (below sketch edges) for tracing / digitizing.
class Sketch_underlay
{
public:
  Sketch_underlay() = default;

  Sketch_underlay(const Sketch_underlay&)            = delete;
  Sketch_underlay& operator=(const Sketch_underlay&) = delete;
  Sketch_underlay(Sketch_underlay&&)                 = delete;
  Sketch_underlay& operator=(Sketch_underlay&&)      = delete;

  [[nodiscard]] bool has_image() const { return m_rgba && !m_rgba->empty() && m_w > 0 && m_h > 0; }

  /// Replace image (RGBA8). Registers pixels in \a store. Sets default axis-aligned rectangle (0.1 unit per pixel).
  [[nodiscard]] bool set_image_rgba(std::vector<uint8_t>&& rgba, int w, int h, Ezy_asset_store& store);

  void set_affine(const gp_Pnt2d& base, const gp_Vec2d& axis_u, const gp_Vec2d& axis_v);
  /// Center \a center, half width/height \a half_extents.x/.y, rotation in degrees (matches Sketch UI transform).
  void set_center_extents_rotation(const glm::dvec2& center, const glm::dvec2& half_extents, double rot_deg);
  void set_opacity(float opaque01);
  void set_visible(bool v);
  /// When true (default), bright pixels (white paper) become transparent in the texture; dark linework stays opaque.
  void set_key_white_transparent(bool on);

  void set_line_tint_enabled(bool on);
  void set_line_tint_rgb(uint8_t r, uint8_t g, uint8_t b);
  void set_line_tint_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

  void line_tint_rgb(uint8_t& r, uint8_t& g, uint8_t& b) const;
  void line_tint_rgba(uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) const;

  // clang-format off

  [[nodiscard]] bool     key_white_transparent() const { return m_key_white_transparent; }
  [[nodiscard]] bool     line_tint_enabled()     const { return m_line_tint_enabled; }
  [[nodiscard]] float    opacity()               const { return m_opacity; }
  [[nodiscard]] bool     visible()               const { return m_visible; }
  [[nodiscard]] gp_Pnt2d base()                  const { return m_base; }
  [[nodiscard]] gp_Vec2d axis_u()                const { return m_axis_u; }
  [[nodiscard]] gp_Vec2d axis_v()                const { return m_axis_v; }
  [[nodiscard]] int      image_w()               const { return m_w; }
  [[nodiscard]] int      image_h()               const { return m_h; }

  void                set_raw_shear_display(bool on);
  [[nodiscard]] bool  raw_shear_display() const { return m_raw_shear_display; }

  void set_flip_image_u(bool on) { m_flip_image_u = on; }
  void set_flip_image_v(bool on) { m_flip_image_v = on; }
  [[nodiscard]] bool flip_image_u() const { return m_flip_image_u; }
  [[nodiscard]] bool flip_image_v() const { return m_flip_image_v; }

  // clang-format on

  void rebuild_and_display(const gp_Pln& pln, AIS_InteractiveContext& ctx);
  void erase(AIS_InteractiveContext& ctx);
  void sync_visibility(const gp_Pln& pln, AIS_InteractiveContext& ctx);
  /// Force viewer update after live property changes (e.g. threshold) without a full rebuild.
  void redisplay(AIS_InteractiveContext& ctx);

  /// Add displayed AIS objects for Sketch List row hover emphasis (image quad and border wire).
  void append_list_hover_ais(std::vector<opencascade::handle<AIS_InteractiveObject>>& out) const;

  nlohmann::json to_json(const Ezy_asset_store& store) const;
  /// Returns false if JSON is invalid or image decode fails.
  [[nodiscard]] bool from_json(const nlohmann::json& j, Ezy_asset_store& store);

private:
  void remove_ais_(AIS_InteractiveContext& ctx);
  void build_ais_(const gp_Pln& pln, AIS_InteractiveContext& ctx);

  std::shared_ptr<const std::vector<uint8_t>> m_rgba;
  std::string                                 m_asset_id;
  int                                         m_w{0};
  int                                         m_h{0};

  gp_Pnt2d m_base{0., 0.};
  gp_Vec2d m_axis_u{100., 0.};
  gp_Vec2d m_axis_v{0., 100.};

  float m_opacity{0.88f};
  bool  m_visible{true};
  /// Luminance key applied only when building the GPU texture (stored pixels stay raw).
  bool m_key_white_transparent{true};
  /// Recolor pixels that remain visible (alpha > 0 after key) for contrast on dark views.
  bool    m_line_tint_enabled{true};
  uint8_t m_tint_r{255};
  uint8_t m_tint_g{220};
  uint8_t m_tint_b{0};
  uint8_t m_tint_a{255};

  /// When true and the axes are sheared, the source image is textured directly onto the
  /// sheared parallelogram quad (raw affine applied to pixels). This makes calibration
  /// mistakes (e.g. incorrect Y-axis) visible as skew/distortion in the raster image itself.
  /// Default false uses special resampling to preserve source image appearance relative to U/V.
  bool m_raw_shear_display{false};

  bool m_flip_image_u{false}; // for raw mode: flip source along U (horizontal in image)
  bool m_flip_image_v{false}; // for raw mode: flip source along V (vertical in image)

  opencascade::handle<AIS_TexturedShape> m_ais;
  opencascade::handle<AIS_Shape>         m_border;
};
