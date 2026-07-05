#pragma once

#include <Standard_Handle.hxx>
#include <cstdint>
#include <glm/glm.hpp>
#include <gp_Pnt2d.hxx>
#include <gp_Vec2d.hxx>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

class AIS_InteractiveObject;
class AIS_InteractiveContext;
class gp_Pln;
class Ezy_asset_store;

/// Raster image drawn in the sketch plane (below sketch edges) for tracing / digitizing.
class Sketch_underlay
{
public:
  Sketch_underlay() = delete;
  Sketch_underlay(AIS_InteractiveContext& ctx);
  ~Sketch_underlay();

  Sketch_underlay(const Sketch_underlay&)            = delete;
  Sketch_underlay& operator=(const Sketch_underlay&) = delete;
  Sketch_underlay(Sketch_underlay&&)                 = delete;
  Sketch_underlay& operator=(Sketch_underlay&&)      = delete;

  [[nodiscard]] bool has_image() const;

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

  [[nodiscard]] bool     key_white_transparent() const;
  [[nodiscard]] bool     line_tint_enabled()     const;
  [[nodiscard]] float    opacity()               const;
  [[nodiscard]] bool     visible()               const;
  [[nodiscard]] gp_Pnt2d base()                  const;
  [[nodiscard]] gp_Vec2d axis_u()                const;
  [[nodiscard]] gp_Vec2d axis_v()                const;
  [[nodiscard]] int      image_w()               const;
  [[nodiscard]] int      image_h()               const;

  void                set_raw_shear_display(bool on);
  [[nodiscard]] bool  raw_shear_display() const;

  void set_flip_image_u(bool on);
  void set_flip_image_v(bool on);
  [[nodiscard]] bool flip_image_u() const;
  [[nodiscard]] bool flip_image_v() const;

  // clang-format on

  void ui_params(double& cx, double& cy, double& half_w, double& half_h, double& rot_deg) const;
  void get_affine(gp_Pnt2d& base, gp_Vec2d& axis_u, gp_Vec2d& axis_v) const;
  /// True when texture U and V directions are perpendicular (no shear). Orthogonal UI assumes this.
  [[nodiscard]] bool axes_orthogonal() const;

  /// Decode PNG/JPEG/BMP bytes, register in \a store, apply line tint, and display when \a sketch_shown.
  [[nodiscard]] bool load_from_file_bytes(const std::string& file_bytes, Ezy_asset_store& store, uint8_t tint_r, uint8_t tint_g,
                                          uint8_t tint_b, uint8_t tint_a, const gp_Pln& pln, bool sketch_shown);

  void clear_and_update();
  /// Rebuild AIS when \a sketch_shown, then refresh the viewer.
  void rebuild_display(const gp_Pln& pln, bool sketch_shown);
  void set_center_extents_rotation_display(const glm::dvec2& center, const glm::dvec2& half_extents, double rot_deg,
                                           const gp_Pln& pln, bool sketch_shown);
  void set_visible_sync(bool v, const gp_Pln& pln);
  void set_opacity_live(float opaque01);

  void rebuild_and_display(const gp_Pln& pln);
  void ctx_erase();
  void clear();
  void sync_visibility(const gp_Pln& pln);
  /// Force viewer update after live property changes (e.g. threshold) without a full rebuild.
  void redisplay();

  /// Add displayed AIS objects for Sketch List row hover emphasis (image quad and border wire).
  void append_list_hover_ais(std::vector<opencascade::handle<AIS_InteractiveObject>>& out) const;

  nlohmann::json to_json(const Ezy_asset_store& store) const;
  /// Returns false if JSON is invalid or image decode fails.
  [[nodiscard]] bool from_json(const nlohmann::json& j, Ezy_asset_store& store);

private:
  class Impl;
  std::unique_ptr<Impl> m_impl;
};
