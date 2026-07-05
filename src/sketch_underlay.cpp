#include "sketch_underlay.h"

#include <AIS_InteractiveObject.hxx>

#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <AIS_TexturedShape.hxx>
#include <Quantity_NameOfColor.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <Image_PixMap.hxx>
#include <Precision.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Wire.hxx>
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <gp_Ax3.hxx>
#include <gp_Pln.hxx>
#include <gp_Vec.hxx>
#include <numbers>

#include "utl_geom.h"
#include "ezy_asset_store.h"
#include "occt_view.h"
#include "sketch_nodes.h"
#include "utl.h"

using namespace glm;

class Sketch_underlay::Impl
{
public:
  explicit Impl(AIS_InteractiveContext& ctx);

  [[nodiscard]] bool has_image() const;

  void set_key_white_transparent(bool on);

  void set_line_tint_enabled(bool on);
  void set_line_tint_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

  void line_tint_rgba(uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) const;

  [[nodiscard]] bool  key_white_transparent() const;
  [[nodiscard]] bool  line_tint_enabled() const;
  [[nodiscard]] float opacity() const;
  [[nodiscard]] bool  visible() const;
  [[nodiscard]] int   image_w() const;
  [[nodiscard]] int   image_h() const;

  void               set_raw_shear_display(bool on);
  [[nodiscard]] bool raw_shear_display() const;

  void               set_flip_image_u(bool on);
  void               set_flip_image_v(bool on);
  [[nodiscard]] bool flip_image_u() const;
  [[nodiscard]] bool flip_image_v() const;

  void               ui_params(double& cx, double& cy, double& half_w, double& half_h, double& rot_deg) const;
  void               get_affine(gp_Pnt2d& base, gp_Vec2d& axis_u, gp_Vec2d& axis_v) const;
  [[nodiscard]] bool axes_orthogonal() const;

  void               set_affine_plane_(const gp_Pnt2d& base, const gp_Vec2d& axis_u, const gp_Vec2d& axis_v, const gp_Pln& pln,
                                       bool sketch_shown);
  [[nodiscard]] bool rescale_uv_chord_to_length_(const gp_Pnt2d& p0, const gp_Pnt2d& p1, double target_len, const gp_Pln& pln,
                                                 bool sketch_shown);
  [[nodiscard]] bool rescale_v_chord_to_length_(const gp_Pnt2d& y0, const gp_Pnt2d& y1, double target_len, const gp_Pln& pln,
                                                bool sketch_shown);

  [[nodiscard]] bool load_from_file_bytes(const std::string& file_bytes, Ezy_asset_store& store, uint8_t tint_r, uint8_t tint_g,
                                          uint8_t tint_b, uint8_t tint_a, const gp_Pln& pln, bool sketch_shown);

  void clear_and_update();
  void rebuild_display(const gp_Pln& pln, bool sketch_shown);
  void set_center_extents_rotation_display(const glm::dvec2& center, const glm::dvec2& half_extents, double rot_deg,
                                           const gp_Pln& pln, bool sketch_shown);
  void set_visible_sync(bool v, const gp_Pln& pln);
  void set_opacity_live(float opaque01);

  void rebuild_and_display(const gp_Pln& pln);
  void ctx_erase();

  void append_list_hover_ais(std::vector<opencascade::handle<AIS_InteractiveObject>>& out) const;

  nlohmann::json     to_json(const Ezy_asset_store& store) const;
  [[nodiscard]] bool from_json(const nlohmann::json& j, Ezy_asset_store& store);

private:
  static constexpr int         k_max_image_dim  = 8192;
  static constexpr std::size_t k_max_rgba_bytes = 64u * 1024u * 1024u; // 64 MiB safety cap

  [[nodiscard]] bool set_image_rgba_(std::vector<uint8_t>&& rgba, int w, int h, Ezy_asset_store& store);
  void               set_affine_(const gp_Pnt2d& base, const gp_Vec2d& axis_u, const gp_Vec2d& axis_v);
  void               set_center_extents_rotation_(const glm::dvec2& center, const glm::dvec2& half_extents, double rot_deg);
  void               set_opacity_(float opaque01);
  void               set_visible_(bool v);
  void               clear_();
  void               sync_visibility_(const gp_Pln& pln);
  void               redisplay_();

  static int      from_base64_char_(char c);
  static bool     base64_decode_(const std::string& in, std::vector<uint8_t>& out);
  static unsigned luminance_u8_(uint8_t r, uint8_t g, uint8_t b);
  static void     apply_key_and_tint_(uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a, bool key_white_transparent,
                                      bool line_tint_enabled, uint8_t tr, uint8_t tg, uint8_t tb, uint8_t ta);
  static void     sample_rgba_bilinear_(const uint8_t* rgba, int w, int h, double xf, double yf, uint8_t out[4]);
  static Handle(Image_PixMap)
      make_pixmap_bottom_up_linear_(const uint8_t* rgba, int w, int h, bool key_white_transparent, bool line_tint_enabled,
                                    uint8_t tr, uint8_t tg, uint8_t tb, uint8_t ta);
  static Handle(Image_PixMap)
      make_pixmap_bottom_up_warped_(const uint8_t* rgba, int w, int h, const gp_Vec2d& axis_u, const gp_Vec2d& axis_v,
                                    bool key_white_transparent, bool line_tint_enabled, uint8_t tr, uint8_t tg, uint8_t tb,
                                    uint8_t ta);
  static bool underlay_axes_orthogonal_(const gp_Vec2d& au, const gp_Vec2d& av);
  static bool plane_to_uv_(const gp_Pnt2d& base, const gp_Vec2d& au, const gp_Vec2d& av, const gp_Pnt2d& p, double& out_u,
                           double& out_v);

  void build_ais_(const gp_Pln& pln);

  AIS_InteractiveContext&                     m_ctx;
  std::shared_ptr<const std::vector<uint8_t>> m_rgba;
  std::string                                 m_asset_id;
  int                                         m_w{0};
  int                                         m_h{0};

  gp_Pnt2d m_base{0., 0.};
  gp_Vec2d m_axis_u{100., 0.};
  gp_Vec2d m_axis_v{0., 100.};

  float   m_opacity{0.88f};
  bool    m_visible{true};
  bool    m_key_white_transparent{true};
  bool    m_line_tint_enabled{true};
  uint8_t m_tint_r{255};
  uint8_t m_tint_g{220};
  uint8_t m_tint_b{0};
  uint8_t m_tint_a{255};

  bool m_raw_shear_display{false};
  bool m_flip_image_u{false};
  bool m_flip_image_v{false};

  opencascade::handle<AIS_TexturedShape> m_ais;
  opencascade::handle<AIS_Shape>         m_border;
};

int Sketch_underlay::Impl::from_base64_char_(char c)
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

bool Sketch_underlay::Impl::base64_decode_(const std::string& in, std::vector<uint8_t>& out)
{
  out.clear();
  if (in.empty())
    return true;

  std::size_t len = in.size();
  while (len > 0 && (in[len - 1] == '=' || in[len - 1] == '\n' || in[len - 1] == '\r' || in[len - 1] == ' '))
    --len;

  out.reserve((len * 3) / 4);
  unsigned buf  = 0;
  int      bits = 0;
  for (std::size_t i = 0; i < len; ++i)
  {
    const char c = in[i];
    if (c == '\n' || c == '\r' || c == ' ')
      continue;

    const int v = from_base64_char_(c);
    if (v < 0)
      return false;

    buf = (buf << 6) | static_cast<unsigned>(v);
    bits += 6;
    if (bits >= 8)
    {
      bits -= 8;
      out.push_back(static_cast<uint8_t>((buf >> bits) & 0xFF));
    }
  }

  return true;
}

/// Rec. 601 luma in 0..255 (integer). White -> high, black -> low.
unsigned Sketch_underlay::Impl::luminance_u8_(uint8_t r, uint8_t g, uint8_t b)
{
  return (77u * static_cast<unsigned>(r) + 150u * static_cast<unsigned>(g) + 29u * static_cast<unsigned>(b)) >> 8;
}

void Sketch_underlay::Impl::apply_key_and_tint_(uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a, bool key_white_transparent,
                                                bool line_tint_enabled, uint8_t tr, uint8_t tg, uint8_t tb, uint8_t ta)
{
  unsigned a2;
  if (key_white_transparent)
  {
    const unsigned L = luminance_u8_(r, g, b);
    a2               = (static_cast<unsigned>(a) * (255u - L)) / 255u;
  }
  else
    a2 = a;

  if (line_tint_enabled && a2 > 0)
  {
    r  = tr;
    g  = tg;
    b  = tb;
    a2 = (a2 * static_cast<unsigned>(ta)) / 255u;
  }

  a = static_cast<uint8_t>(a2);
}

void Sketch_underlay::Impl::sample_rgba_bilinear_(const uint8_t* rgba, int w, int h, double xf, double yf, uint8_t out[4])
{
  if (w <= 0 || h <= 0 || xf < 0.0 || yf < 0.0 || xf > static_cast<double>(w - 1) || yf > static_cast<double>(h - 1))
  {
    out[0] = out[1] = out[2] = out[3] = 0;
    return;
  }

  const int    x0  = static_cast<int>(std::floor(xf));
  const int    y0  = static_cast<int>(std::floor(yf));
  const int    x1  = std::min(x0 + 1, w - 1);
  const int    y1  = std::min(y0 + 1, h - 1);
  const double tx  = xf - static_cast<double>(x0);
  const double ty  = yf - static_cast<double>(y0);
  const auto   pix = [&](int x, int y) -> const uint8_t*
  {
    return rgba + (static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x)) * 4u;
  };

  for (int c = 0; c < 4; ++c)
  {
    const double v00 = static_cast<double>(pix(x0, y0)[c]);
    const double v10 = static_cast<double>(pix(x1, y0)[c]);
    const double v01 = static_cast<double>(pix(x0, y1)[c]);
    const double v11 = static_cast<double>(pix(x1, y1)[c]);
    const double v0  = v00 * (1.0 - tx) + v10 * tx;
    const double v1  = v01 * (1.0 - tx) + v11 * tx;
    out[c]           = static_cast<uint8_t>(std::clamp(v0 * (1.0 - ty) + v1 * ty + 0.5, 0.0, 255.0));
  }
}

/// Straight copy of the image to a bottom-up pixmap (optional row flip for OCCT/OpenGL), with key + tint.
Handle(Image_PixMap) Sketch_underlay::Impl::make_pixmap_bottom_up_linear_(const uint8_t* rgba, int w, int h,
                                                                          bool key_white_transparent, bool line_tint_enabled,
                                                                          uint8_t tr, uint8_t tg, uint8_t tb, uint8_t ta)
{
  if (w <= 0 || h <= 0)
    return {};

  Handle(Image_PixMap) pix = new Image_PixMap();
  if (!pix->InitTrash(Image_Format_RGBA, static_cast<size_t>(w), static_cast<size_t>(h)))
    return {};

  const size_t rowBytes = static_cast<size_t>(w) * 4u;
  uint8_t*     dst      = pix->ChangeData();

  for (int rj = 0; rj < h; ++rj)
  {
    const int      src_row = h - 1 - rj;
    const uint8_t* srcRow  = rgba + static_cast<std::size_t>(src_row) * static_cast<std::size_t>(w) * 4u;
    uint8_t*       dstRow  = dst + static_cast<std::size_t>(rj) * rowBytes;
    for (int ox = 0; ox < w; ++ox)
    {
      uint8_t px[4] = {srcRow[ox * 4 + 0], srcRow[ox * 4 + 1], srcRow[ox * 4 + 2], srcRow[ox * 4 + 3]};
      apply_key_and_tint_(px[0], px[1], px[2], px[3], key_white_transparent, line_tint_enabled, tr, tg, tb, ta);
      dstRow[static_cast<std::size_t>(ox) * 4u + 0] = px[0];
      dstRow[static_cast<std::size_t>(ox) * 4u + 1] = px[1];
      dstRow[static_cast<std::size_t>(ox) * 4u + 2] = px[2];
      dstRow[static_cast<std::size_t>(ox) * 4u + 3] = px[3];
    }
  }

  return pix;
}

/// Builds a bottom-up pixmap for AIS_TexturedShape when the underlay axes are sheared (non-orthogonal): uses an
/// axis-aligned face in the sketch plane and inverse-rotated sampling so the bitmap matches OCCT UV on the AABB.
Handle(Image_PixMap) Sketch_underlay::Impl::make_pixmap_bottom_up_warped_(const uint8_t* rgba, int w, int h,
                                                                          const gp_Vec2d& axis_u, const gp_Vec2d& axis_v,
                                                                          bool key_white_transparent, bool line_tint_enabled,
                                                                          uint8_t tr, uint8_t tg, uint8_t tb, uint8_t ta)
{
  const double hw = 0.5 * axis_u.Magnitude();
  const double hh = 0.5 * axis_v.Magnitude();
  if (hw <= 1e-12 || hh <= 1e-12)
    return {};

  const double theta = std::atan2(axis_u.Y(), axis_u.X());
  const double c     = std::cos(theta);
  const double s     = std::sin(theta);
  const double hx    = std::max(std::abs(hw * c) + std::abs(hh * s), 1e-12);
  const double hy    = std::max(std::abs(hw * s) + std::abs(hh * c), 1e-12);

  int out_w = static_cast<int>(std::lround(static_cast<double>(w) * hx / hw));
  int out_h = static_cast<int>(std::lround(static_cast<double>(h) * hy / hh));
  out_w     = std::clamp(out_w, 1, k_max_image_dim);
  out_h     = std::clamp(out_h, 1, k_max_image_dim);

  Handle(Image_PixMap) pix = new Image_PixMap();
  if (!pix->InitTrash(Image_Format_RGBA, static_cast<size_t>(out_w), static_cast<size_t>(out_h)))
    return {};

  const size_t rowBytes = static_cast<size_t>(out_w) * 4u;
  uint8_t*     dst      = pix->ChangeData();

  constexpr double k_eps = 1e-9;
  for (int rj = 0; rj < out_h; ++rj)
  {
    // Bottom row rj = 0 -> bottom of texture (small plane dv).
    const double t_b    = (static_cast<double>(rj) + 0.5) / static_cast<double>(out_h);
    const double dv     = (2.0 * t_b - 1.0) * hy;
    uint8_t*     dstRow = dst + static_cast<std::size_t>(rj) * rowBytes;
    for (int ox = 0; ox < out_w; ++ox)
    {
      const double s01 = (static_cast<double>(ox) + 0.5) / static_cast<double>(out_w);
      const double du  = (2.0 * s01 - 1.0) * hx;
      // Inverse rotate from plane (du,dv) to image (u,v) offsets from quad center.
      const double img_u = du * c + dv * s;
      const double img_v = -du * s + dv * c;

      uint8_t px[4];
      if (std::abs(img_u) > hw + k_eps || std::abs(img_v) > hh + k_eps)
        px[0] = px[1] = px[2] = px[3] = 0;
      else
      {
        const double sx = (img_u + hw) / (2.0 * hw) * static_cast<double>(w - 1);
        // Match stored RGBA row order (row 0 = image top) to OCCT bottom-first pixmap / texture v.
        const double sy = (hh - img_v) / (2.0 * hh) * static_cast<double>(h - 1);
        sample_rgba_bilinear_(rgba, w, h, sx, sy, px);
      }

      apply_key_and_tint_(px[0], px[1], px[2], px[3], key_white_transparent, line_tint_enabled, tr, tg, tb, ta);
      const size_t o = static_cast<size_t>(ox) * 4u;
      dstRow[o + 0]  = px[0];
      dstRow[o + 1]  = px[1];
      dstRow[o + 2]  = px[2];
      dstRow[o + 3]  = px[3];
    }
  }

  // Row rj = 0 is texture bottom (dv = -hy); Image_PixMap bottom-up uses row 0 as OpenGL texture bottom.
  return pix;
}

bool Sketch_underlay::Impl::underlay_axes_orthogonal_(const gp_Vec2d& au, const gp_Vec2d& av)
{
  const double dot   = au.X() * av.X() + au.Y() * av.Y();
  const double scale = au.Magnitude() * av.Magnitude();
  if (scale < 1e-24)
    return true;

  return std::abs(dot) < 1e-9 * scale;
}

bool Sketch_underlay::Impl::plane_to_uv_(const gp_Pnt2d& base, const gp_Vec2d& au, const gp_Vec2d& av, const gp_Pnt2d& p,
                                         double& out_u, double& out_v)
{
  const double det = au.X() * av.Y() - au.Y() * av.X();
  if (std::abs(det) < 1e-18)
    return false;

  const gp_Vec2d r(p.X() - base.X(), p.Y() - base.Y());
  out_u = (r.X() * av.Y() - r.Y() * av.X()) / det;
  out_v = (-r.X() * au.Y() + r.Y() * au.X()) / det;
  return true;
}

Sketch_underlay::Impl::Impl(AIS_InteractiveContext& ctx)
    : m_ctx(ctx)
{
}

bool Sketch_underlay::Impl::has_image() const { return m_rgba && !m_rgba->empty() && m_w > 0 && m_h > 0; }

bool  Sketch_underlay::Impl::key_white_transparent() const { return m_key_white_transparent; }
bool  Sketch_underlay::Impl::line_tint_enabled() const { return m_line_tint_enabled; }
float Sketch_underlay::Impl::opacity() const { return m_opacity; }
bool  Sketch_underlay::Impl::visible() const { return m_visible; }
int   Sketch_underlay::Impl::image_w() const { return m_w; }
int   Sketch_underlay::Impl::image_h() const { return m_h; }
bool  Sketch_underlay::Impl::raw_shear_display() const { return m_raw_shear_display; }
void  Sketch_underlay::Impl::set_flip_image_u(bool on) { m_flip_image_u = on; }
void  Sketch_underlay::Impl::set_flip_image_v(bool on) { m_flip_image_v = on; }
bool  Sketch_underlay::Impl::flip_image_u() const { return m_flip_image_u; }
bool  Sketch_underlay::Impl::flip_image_v() const { return m_flip_image_v; }

bool Sketch_underlay::Impl::set_image_rgba_(std::vector<uint8_t>&& rgba, int w, int h, Ezy_asset_store& store)
{
  if (w <= 0 || h <= 0 || rgba.size() < static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4u)
    return false;

  if (w > k_max_image_dim || h > k_max_image_dim)
    return false;

  if (rgba.size() > k_max_rgba_bytes)
    return false;

  m_asset_id = store.register_rgba(rgba, w, h);
  m_rgba     = store.get(m_asset_id);
  m_w        = w;
  m_h        = h;

  // Default: centered at plane origin, 0.1 model units per image pixel (adjust in UI).
  const double s = 0.1;
  m_axis_u       = gp_Vec2d(static_cast<double>(m_w) * s, 0.);
  m_axis_v       = gp_Vec2d(0., static_cast<double>(m_h) * s);
  m_base         = gp_Pnt2d(-0.5 * m_axis_u.X(), -0.5 * m_axis_v.Y());

  return true;
}

void Sketch_underlay::Impl::set_affine_(const gp_Pnt2d& base, const gp_Vec2d& axis_u, const gp_Vec2d& axis_v)
{
  m_base   = base;
  m_axis_u = axis_u;
  m_axis_v = axis_v;
}

void Sketch_underlay::Impl::set_center_extents_rotation_(const dvec2& center, const dvec2& half_extents, double rot_deg)
{
  if (!has_image())
    return;

  double half_w = half_extents.x;
  double half_h = half_extents.y;

  constexpr double k_min = 1e-9;
  if (half_w < k_min)
    half_w = k_min;

  if (half_h < k_min)
    half_h = k_min;

  const double   rad = rot_deg * (std::numbers::pi / 180.0);
  const double   c   = std::cos(rad);
  const double   s   = std::sin(rad);
  const gp_Vec2d axis_u(2.0 * half_w * c, 2.0 * half_w * s);
  const gp_Vec2d axis_v(-2.0 * half_h * s, 2.0 * half_h * c);
  const gp_Pnt2d base(center.x - 0.5 * (axis_u.X() + axis_v.X()), center.y - 0.5 * (axis_u.Y() + axis_v.Y()));

  set_affine_(base, axis_u, axis_v);
}

void Sketch_underlay::Impl::set_opacity_(float opaque01)
{
  if (opaque01 < 0.f)
    opaque01 = 0.f;

  if (opaque01 > 1.f)
    opaque01 = 1.f;

  m_opacity = opaque01;
  if (!m_ais.IsNull())
    m_ais->SetTransparency(1.0 - static_cast<double>(m_opacity));
}

void Sketch_underlay::Impl::set_visible_(bool v) { m_visible = v; }

void Sketch_underlay::Impl::set_key_white_transparent(bool on) { m_key_white_transparent = on; }

void Sketch_underlay::Impl::set_line_tint_enabled(bool on) { m_line_tint_enabled = on; }

void Sketch_underlay::Impl::set_line_tint_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
  m_tint_r = r;
  m_tint_g = g;
  m_tint_b = b;
  m_tint_a = a;
}

void Sketch_underlay::Impl::set_raw_shear_display(bool on)
{
  if (m_raw_shear_display != on)
    m_raw_shear_display = on;
}

void Sketch_underlay::Impl::line_tint_rgba(uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) const
{
  r = m_tint_r;
  g = m_tint_g;
  b = m_tint_b;
  a = m_tint_a;
}

void Sketch_underlay::Impl::ui_params(double& cx, double& cy, double& half_w, double& half_h, double& rot_deg) const
{
  if (!has_image())
  {
    cx = cy = half_w = half_h = rot_deg = 0.;
    return;
  }

  half_w  = 0.5 * m_axis_u.Magnitude();
  half_h  = 0.5 * m_axis_v.Magnitude();
  cx      = m_base.X() + 0.5 * (m_axis_u.X() + m_axis_v.X());
  cy      = m_base.Y() + 0.5 * (m_axis_u.Y() + m_axis_v.Y());
  rot_deg = std::atan2(m_axis_u.Y(), m_axis_u.X()) * (180.0 / std::numbers::pi);
}

void Sketch_underlay::Impl::get_affine(gp_Pnt2d& base, gp_Vec2d& axis_u, gp_Vec2d& axis_v) const
{
  if (!has_image())
  {
    base   = gp_Pnt2d(0., 0.);
    axis_u = gp_Vec2d(0., 0.);
    axis_v = gp_Vec2d(0., 0.);
    return;
  }

  base   = m_base;
  axis_u = m_axis_u;
  axis_v = m_axis_v;
}

bool Sketch_underlay::Impl::axes_orthogonal() const
{
  if (!has_image())
    return true;

  return underlay_axes_orthogonal_(m_axis_u, m_axis_v);
}

void Sketch_underlay::Impl::set_affine_plane_(const gp_Pnt2d& base, const gp_Vec2d& axis_u, const gp_Vec2d& axis_v,
                                              const gp_Pln& pln, bool sketch_shown)
{
  if (!has_image())
    return;

  constexpr double k_min2 = 1e-18;
  if (axis_u.SquareMagnitude() < k_min2 || axis_v.SquareMagnitude() < k_min2)
    return;

  set_affine_(base, axis_u, axis_v);
  rebuild_display(pln, sketch_shown);
}

bool Sketch_underlay::Impl::rescale_uv_chord_to_length_(const gp_Pnt2d& p0, const gp_Pnt2d& p1, double target_len,
                                                        const gp_Pln& pln, bool sketch_shown)
{
  if (!has_image())
    return false;

  if (target_len <= 1e-12)
    return false;

  const gp_Pnt2d base = m_base;
  const gp_Vec2d au   = m_axis_u;
  const gp_Vec2d av   = m_axis_v;
  double         u0{};
  double         v0{};
  double         u1{};
  double         v1{};
  if (!plane_to_uv_(base, au, av, p0, u0, v0) || !plane_to_uv_(base, au, av, p1, u1, v1))
    return false;

  const double L = p0.Distance(p1);
  if (L <= 1e-12)
    return false;

  const double   k   = target_len / L;
  const gp_Vec2d au2 = au * k;
  const gp_Vec2d av2 = av * k;
  const gp_Vec2d off = au2 * u0 + av2 * v0;
  const gp_Pnt2d base2(p0.X() - off.X(), p0.Y() - off.Y());
  set_affine_plane_(base2, au2, av2, pln, sketch_shown);
  return true;
}

bool Sketch_underlay::Impl::rescale_v_chord_to_length_(const gp_Pnt2d& y0, const gp_Pnt2d& y1, double target_len,
                                                       const gp_Pln& pln, bool sketch_shown)
{
  if (!has_image())
    return false;

  if (target_len <= 1e-12)
    return false;

  const gp_Pnt2d base = m_base;
  const gp_Vec2d au   = m_axis_u;
  const gp_Vec2d av   = m_axis_v;
  double         u0{};
  double         v0{};
  double         u1{};
  double         v1{};
  if (!plane_to_uv_(base, au, av, y0, u0, v0) || !plane_to_uv_(base, au, av, y1, u1, v1))
    return false;

  const double du = u1 - u0;
  const double dv = v1 - v0;
  if (std::abs(dv) < 1e-9)
    return false;

  const double L = y0.Distance(y1);
  if (L <= 1e-12)
    return false;

  const double   r      = target_len / L;
  const gp_Vec2d av_new = av * r + au * (((r - 1.0) * du) / dv);
  const gp_Vec2d anchor = au * u0 + av_new * v0;
  const gp_Pnt2d base2(y0.X() - anchor.X(), y0.Y() - anchor.Y());
  set_affine_plane_(base2, au, av_new, pln, sketch_shown);
  return true;
}

bool Sketch_underlay::Impl::load_from_file_bytes(const std::string& file_bytes, Ezy_asset_store& store, uint8_t tint_r,
                                                 uint8_t tint_g, uint8_t tint_b, uint8_t tint_a, const gp_Pln& pln,
                                                 bool sketch_shown)
{
  const auto dec = decode_image_bytes(file_bytes);
  if (!dec)
    return false;

  std::vector<uint8_t> rgba = std::move(std::get<0>(*dec));
  const int            w    = std::get<1>(*dec);
  const int            h    = std::get<2>(*dec);

  if (!set_image_rgba_(std::move(rgba), w, h, store))
    return false;

  set_line_tint_rgba(tint_r, tint_g, tint_b, tint_a);
  if (sketch_shown)
    rebuild_and_display(pln);

  m_ctx.UpdateCurrentViewer();
  return true;
}

void Sketch_underlay::Impl::clear_and_update()
{
  if (!has_image())
    return;

  clear_();
  m_ctx.UpdateCurrentViewer();
}

void Sketch_underlay::Impl::rebuild_display(const gp_Pln& pln, bool sketch_shown)
{
  if (!has_image())
    return;

  if (sketch_shown)
    rebuild_and_display(pln);

  m_ctx.UpdateCurrentViewer();
}

void Sketch_underlay::Impl::set_center_extents_rotation_display(const dvec2& center, const dvec2& half_extents, double rot_deg,
                                                                const gp_Pln& pln, bool sketch_shown)
{
  if (!has_image())
    return;

  set_center_extents_rotation_(center, half_extents, rot_deg);
  if (sketch_shown)
    rebuild_and_display(pln);

  m_ctx.UpdateCurrentViewer();
}

void Sketch_underlay::Impl::set_visible_sync(bool v, const gp_Pln& pln)
{
  if (!has_image())
    return;

  set_visible_(v);
  sync_visibility_(pln);
  m_ctx.UpdateCurrentViewer();
}

void Sketch_underlay::Impl::set_opacity_live(float opaque01)
{
  if (!has_image())
    return;

  set_opacity_(opaque01);
  redisplay_();
  m_ctx.UpdateCurrentViewer();
}

void Sketch_underlay::Impl::rebuild_and_display(const gp_Pln& pln)
{
  ctx_erase();
  if (!has_image() || !m_visible)
    return;

  build_ais_(pln);
}

void Sketch_underlay::Impl::ctx_erase()
{
  if (!m_ais.IsNull())
  {
    m_ctx.Remove(m_ais, false);
    m_ais.Nullify();
  }
  if (!m_border.IsNull())
  {
    m_ctx.Remove(m_border, false);
    m_border.Nullify();
  }
}

void Sketch_underlay::Impl::clear_()
{
  ctx_erase();
  m_rgba.reset();
  m_asset_id.clear();
  m_w = 0;
  m_h = 0;
}

void Sketch_underlay::Impl::sync_visibility_(const gp_Pln& pln)
{
  if (!has_image())
    return;

  if (m_visible)
    rebuild_and_display(pln);
  else
    ctx_erase();
}

void Sketch_underlay::Impl::redisplay_()
{
  if (!m_ais.IsNull())
    m_ctx.Redisplay(m_ais, true);
}

void Sketch_underlay::Impl::append_list_hover_ais(std::vector<opencascade::handle<AIS_InteractiveObject>>& out) const
{
  if (!has_image() || !m_visible)
    return;

  if (!m_ais.IsNull())
    out.push_back(m_ais);

  if (!m_border.IsNull())
    out.push_back(m_border);
}

void Sketch_underlay::Impl::build_ais_(const gp_Pln& pln)
{
  ctx_erase();
  if (!has_image())
    return;

  gp_Vec nudge(pln.Axis().Direction());
  nudge.Multiply(-10.0 * Precision::Confusion());

  TopoDS_Face face;

  const gp_Pnt2d bu2(m_base.X() + m_axis_u.X(), m_base.Y() + m_axis_u.Y());
  const gp_Pnt2d bv2(m_base.X() + m_axis_v.X(), m_base.Y() + m_axis_v.Y());
  const gp_Pnt   P0 = to_3d(pln, m_base).Translated(nudge);
  const gp_Pnt   Pu = to_3d(pln, bu2).Translated(nudge);
  const gp_Pnt   Pv = to_3d(pln, bv2).Translated(nudge);
  gp_Vec         Du(P0, Pu);
  const double   du_len = Du.Magnitude();
  const double   dv_len = gp_Vec(P0, Pv).Magnitude();
  if (du_len <= Precision::Confusion() || dv_len <= Precision::Confusion())
    return;

  // Image quad corners in the sketch plane (always a parallelogram from base + u + v).
  // These define the tight bounds for the rendered border rectangle around the underlay.
  const gp_Pnt2d b00 = m_base;
  const gp_Pnt2d b10(m_base.X() + m_axis_u.X(), m_base.Y() + m_axis_u.Y());
  const gp_Pnt2d b11(b10.X() + m_axis_v.X(), b10.Y() + m_axis_v.Y());
  const gp_Pnt2d b01(m_base.X() + m_axis_v.X(), m_base.Y() + m_axis_v.Y());
  const gp_Pnt   Q00 = to_3d(pln, b00).Translated(nudge);
  const gp_Pnt   Q10 = to_3d(pln, b10).Translated(nudge);
  const gp_Pnt   Q11 = to_3d(pln, b11).Translated(nudge);
  const gp_Pnt   Q01 = to_3d(pln, b01).Translated(nudge);

  const bool is_ortho = underlay_axes_orthogonal_(m_axis_u, m_axis_v);

  Handle(Image_PixMap) pix;

  if (is_ortho || !m_raw_shear_display)
  {
    if (is_ortho)
    {
      // Rotation + uniform scale: build a rectangular face in a plane whose U/V match bitmap axes so texture is not
      // sheared (no inverse-resample pixmap needed).
      const gp_Dir            Zpl = pln.Position().Direction();
      const gp_Dir            Xu(Du);
      const gp_Ax3            ax3(P0, Zpl, Xu);
      const gp_Pln            local_pln(ax3);
      BRepBuilderAPI_MakeFace faceMk(local_pln, 0.0, du_len, 0.0, dv_len);
      if (!faceMk.IsDone())
        return;

      face = faceMk.Face();
      pix  = make_pixmap_bottom_up_linear_(m_rgba->data(), m_w, m_h, m_key_white_transparent, m_line_tint_enabled, m_tint_r,
                                           m_tint_g, m_tint_b, m_tint_a);
    }
    else
    {
      // Sheared affine (e.g. after Y-from-edge calibration): axis-aligned bbox + inverse-rotated pixmap resample.
      // This preserves the source image appearance relative to the U/V axes (no extra distortion in the raster).
      const double hw    = 0.5 * m_axis_u.Magnitude();
      const double hh    = 0.5 * m_axis_v.Magnitude();
      const double theta = std::atan2(m_axis_u.Y(), m_axis_u.X());
      const double c     = std::cos(theta);
      const double s     = std::sin(theta);
      const double hx    = std::max(std::abs(hw * c) + std::abs(hh * s), 1e-12);
      const double hy    = std::max(std::abs(hw * s) + std::abs(hh * c), 1e-12);
      const double cx    = m_base.X() + 0.5 * (m_axis_u.X() + m_axis_v.X());
      const double cy    = m_base.Y() + 0.5 * (m_axis_u.Y() + m_axis_v.Y());

      const gp_Pnt2d c00(cx - hx, cy - hy);
      const gp_Pnt2d c10(cx + hx, cy - hy);
      const gp_Pnt2d c11(cx + hx, cy + hy);
      const gp_Pnt2d c01(cx - hx, cy + hy);

      const gp_Pnt p00 = to_3d(pln, c00).Translated(nudge);
      const gp_Pnt p10 = to_3d(pln, c10).Translated(nudge);
      const gp_Pnt p11 = to_3d(pln, c11).Translated(nudge);
      const gp_Pnt p01 = to_3d(pln, c01).Translated(nudge);

      BRepBuilderAPI_MakeWire wireMk;
      wireMk.Add(BRepBuilderAPI_MakeEdge(p00, p10).Edge());
      wireMk.Add(BRepBuilderAPI_MakeEdge(p10, p11).Edge());
      wireMk.Add(BRepBuilderAPI_MakeEdge(p11, p01).Edge());
      wireMk.Add(BRepBuilderAPI_MakeEdge(p01, p00).Edge());
      if (!wireMk.IsDone())
        return;

      BRepBuilderAPI_MakeFace faceMk(wireMk.Wire(), true);
      if (!faceMk.IsDone())
        return;

      face = faceMk.Face();
      pix  = make_pixmap_bottom_up_warped_(m_rgba->data(), m_w, m_h, m_axis_u, m_axis_v, m_key_white_transparent,
                                           m_line_tint_enabled, m_tint_r, m_tint_g, m_tint_b, m_tint_a);
    }
  }
  else
  {
    // Raw shear mode (m_raw_shear_display true): use the *exact* parallelogram quad face
    // for the textured shape (the visible raster "is" the cyan parallelogram).
    // Linear (unmodified) pixmap + user-controlled flips. The sheared geometry of the
    // face applies the affine directly to the pixels, so a bad Y-axis calibration makes
    // the raster image visibly skewed/distorted, with content "skewed to the bounds".
    // Flips let the user put raster 0,0 at the desired corner of the para (addresses
    // the UV mapping on wire faces).
    BRepBuilderAPI_MakeWire wireMk;
    wireMk.Add(BRepBuilderAPI_MakeEdge(Q00, Q10).Edge());
    wireMk.Add(BRepBuilderAPI_MakeEdge(Q10, Q11).Edge());
    wireMk.Add(BRepBuilderAPI_MakeEdge(Q11, Q01).Edge());
    wireMk.Add(BRepBuilderAPI_MakeEdge(Q01, Q00).Edge());
    if (!wireMk.IsDone())
      return;

    BRepBuilderAPI_MakeFace faceMk(wireMk.Wire(), true);
    if (!faceMk.IsDone())
      return;

    face = faceMk.Face();
    pix  = make_pixmap_bottom_up_linear_(m_rgba->data(), m_w, m_h, m_key_white_transparent, m_line_tint_enabled, m_tint_r,
                                         m_tint_g, m_tint_b, m_tint_a);

    if (!pix.IsNull())
    {
      uint8_t*     data     = pix->ChangeData();
      const size_t ww       = pix->Width();
      const size_t hh       = pix->Height();
      const size_t rowBytes = ww * 4;

      if (m_flip_image_v)
      {
        // vertical flip (V / image rows)
        for (size_t r = 0; r < hh / 2; ++r)
        {
          uint8_t* r1 = data + r * rowBytes;
          uint8_t* r2 = data + (hh - 1 - r) * rowBytes;
          for (size_t c = 0; c < rowBytes; ++c)
            std::swap(r1[c], r2[c]);
        }
      }
      if (m_flip_image_u)
      {
        // horizontal flip (U / image columns)
        for (size_t r = 0; r < hh; ++r)
        {
          uint8_t* row = data + r * rowBytes;
          for (size_t c = 0; c < ww / 2; ++c)
            for (int ch = 0; ch < 4; ++ch)
              std::swap(row[c * 4 + ch], row[(ww - 1 - c) * 4 + ch]);
        }
      }
    }
  }

  if (pix.IsNull())
    return;

  m_ais = new AIS_TexturedShape(face);
  m_ais->SetTexturePixMap(pix);
  m_ais->SetTextureMapOn();
  m_ais->DisableTextureModulate();
  m_ais->SetTextureRepeat(false, 1., 1.);
  m_ais->SetTransparency(1.0 - static_cast<double>(m_opacity));
  m_ais->SetDisplayMode(3);

  // Build a thin wireframe border around the exact image quad (parallelogram) so the underlay extent
  // is always obvious, even for newly added images or when most content is keyed transparent.
  {
    BRepBuilderAPI_MakeWire wmk;
    wmk.Add(BRepBuilderAPI_MakeEdge(Q00, Q10).Edge());
    wmk.Add(BRepBuilderAPI_MakeEdge(Q10, Q11).Edge());
    wmk.Add(BRepBuilderAPI_MakeEdge(Q11, Q01).Edge());
    wmk.Add(BRepBuilderAPI_MakeEdge(Q01, Q00).Edge());
    if (wmk.IsDone())
    {
      m_border = new AIS_Shape(wmk.Wire());
      m_border->SetColor(Quantity_NOC_CYAN);
      m_border->SetWidth(1.5);
    }
  }

  if (m_visible)
  {
    m_ctx.Display(m_ais, 3, 0, false);
    m_ctx.Deactivate(opencascade::handle<AIS_InteractiveObject>(m_ais));
    if (!m_border.IsNull())
    {
      m_ctx.Display(m_border, false);
      m_ctx.Deactivate(opencascade::handle<AIS_InteractiveObject>(m_border));
    }
  }
}

nlohmann::json Sketch_underlay::Impl::to_json(const Ezy_asset_store& store) const
{
  using nlohmann::json;
  json j;
  if (!has_image() || m_asset_id.empty())
    return j;

  (void)store;
  j["asset"]                 = m_asset_id;
  j["w"]                     = m_w;
  j["h"]                     = m_h;
  j["base"]                  = json::object({{"x", m_base.X()}, {"y", m_base.Y()}});
  j["axis_u"]                = json::object({{"x", m_axis_u.X()}, {"y", m_axis_u.Y()}});
  j["axis_v"]                = json::object({{"x", m_axis_v.X()}, {"y", m_axis_v.Y()}});
  j["opacity"]               = m_opacity;
  j["visible"]               = m_visible;
  j["key_white_transparent"] = m_key_white_transparent;
  j["line_tint_enabled"]     = m_line_tint_enabled;
  j["line_tint_rgba"]        = nlohmann::json::array({m_tint_r, m_tint_g, m_tint_b, m_tint_a});
  j["raw_shear_display"]     = m_raw_shear_display;
  return j;
}

bool Sketch_underlay::Impl::from_json(const nlohmann::json& j, Ezy_asset_store& store)
{
  using nlohmann::json;
  if (!j.is_object() || !j.contains("w") || !j.contains("h"))
    return false;

  const int w = j.at("w").get<int>();
  const int h = j.at("h").get<int>();
  if (w <= 0 || h <= 0 || w > k_max_image_dim || h > k_max_image_dim)
    return false;

  if (j.contains("asset") && j["asset"].is_string())
  {
    m_asset_id = j["asset"].get<std::string>();
    m_rgba     = store.get(m_asset_id);
    if (!m_rgba || m_rgba->size() < static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4u)
      return false;
  }
  else if (j.contains("rgba_b64"))
  {
    std::vector<uint8_t> decoded;
    if (!base64_decode_(j.at("rgba_b64").get<std::string>(), decoded))
      return false;

    if (decoded.size() < static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4u)
      return false;

    if (decoded.size() > k_max_rgba_bytes)
      return false;

    m_asset_id = store.register_rgba(decoded, w, h);
    m_rgba     = store.get(m_asset_id);
  }
  else
    return false;

  m_w = w;
  m_h = h;

  if (j.contains("base"))
    m_base = gp_Pnt2d(j["base"].at("x").get<double>(), j["base"].at("y").get<double>());

  if (j.contains("axis_u"))
    m_axis_u = gp_Vec2d(j["axis_u"].at("x").get<double>(), j["axis_u"].at("y").get<double>());

  if (j.contains("axis_v"))
    m_axis_v = gp_Vec2d(j["axis_v"].at("x").get<double>(), j["axis_v"].at("y").get<double>());

  m_opacity               = j.value("opacity", 0.88f);
  m_visible               = j.value("visible", true);
  m_key_white_transparent = j.value("key_white_transparent", true);
  m_line_tint_enabled     = j.value("line_tint_enabled", true);
  m_tint_a                = 255;
  if (j.contains("line_tint_rgba") && j["line_tint_rgba"].is_array() && j["line_tint_rgba"].size() >= 4)
  {
    m_tint_r = static_cast<uint8_t>(j["line_tint_rgba"][0].get<int>());
    m_tint_g = static_cast<uint8_t>(j["line_tint_rgba"][1].get<int>());
    m_tint_b = static_cast<uint8_t>(j["line_tint_rgba"][2].get<int>());
    m_tint_a = static_cast<uint8_t>(j["line_tint_rgba"][3].get<int>());
  }
  else if (j.contains("line_tint_rgb") && j["line_tint_rgb"].is_array() && j["line_tint_rgb"].size() >= 3)
  {
    m_tint_r = static_cast<uint8_t>(j["line_tint_rgb"][0].get<int>());
    m_tint_g = static_cast<uint8_t>(j["line_tint_rgb"][1].get<int>());
    m_tint_b = static_cast<uint8_t>(j["line_tint_rgb"][2].get<int>());
  }
  m_raw_shear_display = j.value("raw_shear_display", false);
  if (m_opacity < 0.f)
    m_opacity = 0.f;

  if (m_opacity > 1.f)
    m_opacity = 1.f;

  m_ais.Nullify();

  return true;
}

Sketch_underlay::Sketch_underlay(AIS_InteractiveContext& ctx)
    : m_impl(std::make_unique<Impl>(ctx))
{
}

Sketch_underlay::~Sketch_underlay() = default;

bool Sketch_underlay::has_image() const { return m_impl->has_image(); }

void Sketch_underlay::set_key_white_transparent(bool on) { m_impl->set_key_white_transparent(on); }

void Sketch_underlay::set_line_tint_enabled(bool on) { m_impl->set_line_tint_enabled(on); }

void Sketch_underlay::set_line_tint_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) { m_impl->set_line_tint_rgba(r, g, b, a); }

void Sketch_underlay::line_tint_rgba(uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) const
{
  m_impl->line_tint_rgba(r, g, b, a);
}

bool Sketch_underlay::key_white_transparent() const { return m_impl->key_white_transparent(); }

bool Sketch_underlay::line_tint_enabled() const { return m_impl->line_tint_enabled(); }

float Sketch_underlay::opacity() const { return m_impl->opacity(); }

bool Sketch_underlay::visible() const { return m_impl->visible(); }

int Sketch_underlay::image_w() const { return m_impl->image_w(); }

int Sketch_underlay::image_h() const { return m_impl->image_h(); }

void Sketch_underlay::set_raw_shear_display(bool on) { m_impl->set_raw_shear_display(on); }

bool Sketch_underlay::raw_shear_display() const { return m_impl->raw_shear_display(); }

void Sketch_underlay::set_flip_image_u(bool on) { m_impl->set_flip_image_u(on); }

void Sketch_underlay::set_flip_image_v(bool on) { m_impl->set_flip_image_v(on); }

bool Sketch_underlay::flip_image_u() const { return m_impl->flip_image_u(); }

bool Sketch_underlay::flip_image_v() const { return m_impl->flip_image_v(); }

void Sketch_underlay::ui_params(double& cx, double& cy, double& half_w, double& half_h, double& rot_deg) const
{
  m_impl->ui_params(cx, cy, half_w, half_h, rot_deg);
}

void Sketch_underlay::get_affine(gp_Pnt2d& base, gp_Vec2d& axis_u, gp_Vec2d& axis_v) const
{
  m_impl->get_affine(base, axis_u, axis_v);
}

bool Sketch_underlay::axes_orthogonal() const { return m_impl->axes_orthogonal(); }

std::optional<gp_Pnt2d> Sketch_underlay::pick_point_for_calib(Occt_view& view, const gp_Pln& pln, Sketch_nodes& nodes,
                                                              const ScreenCoords& screen_coords)
{
  std::optional<gp_Pnt2d> on_pln = view.pt_on_plane(screen_coords, pln);
  if (!on_pln)
    return std::nullopt;

  if (std::optional<size_t> idx = nodes.try_get_node_idx_snap(*on_pln))
    return nodes[*idx];

  return *on_pln;
}

void Sketch_underlay::set_affine_plane(const gp_Pnt2d& base, const gp_Vec2d& axis_u, const gp_Vec2d& axis_v, const gp_Pln& pln,
                                       bool sketch_shown)
{
  m_impl->set_affine_plane_(base, axis_u, axis_v, pln, sketch_shown);
}

bool Sketch_underlay::rescale_uv_chord_to_length(const gp_Pnt2d& p0, const gp_Pnt2d& p1, double target_len, const gp_Pln& pln,
                                                 bool sketch_shown)
{
  return m_impl->rescale_uv_chord_to_length_(p0, p1, target_len, pln, sketch_shown);
}

bool Sketch_underlay::rescale_v_chord_to_length(const gp_Pnt2d& y0, const gp_Pnt2d& y1, double target_len, const gp_Pln& pln,
                                                bool sketch_shown)
{
  return m_impl->rescale_v_chord_to_length_(y0, y1, target_len, pln, sketch_shown);
}

bool Sketch_underlay::load_from_file_bytes(const std::string& file_bytes, Ezy_asset_store& store, uint8_t tint_r,
                                           uint8_t tint_g, uint8_t tint_b, uint8_t tint_a, const gp_Pln& pln, bool sketch_shown)
{
  return m_impl->load_from_file_bytes(file_bytes, store, tint_r, tint_g, tint_b, tint_a, pln, sketch_shown);
}

void Sketch_underlay::clear_and_update() { m_impl->clear_and_update(); }

void Sketch_underlay::rebuild_display(const gp_Pln& pln, bool sketch_shown) { m_impl->rebuild_display(pln, sketch_shown); }

void Sketch_underlay::set_center_extents_rotation_display(const glm::dvec2& center, const glm::dvec2& half_extents,
                                                          double rot_deg, const gp_Pln& pln, bool sketch_shown)
{
  m_impl->set_center_extents_rotation_display(center, half_extents, rot_deg, pln, sketch_shown);
}

void Sketch_underlay::set_visible_sync(bool v, const gp_Pln& pln) { m_impl->set_visible_sync(v, pln); }

void Sketch_underlay::set_opacity_live(float opaque01) { m_impl->set_opacity_live(opaque01); }

void Sketch_underlay::rebuild_and_display(const gp_Pln& pln) { m_impl->rebuild_and_display(pln); }

void Sketch_underlay::ctx_erase() { m_impl->ctx_erase(); }

void Sketch_underlay::append_list_hover_ais(std::vector<opencascade::handle<AIS_InteractiveObject>>& out) const
{
  m_impl->append_list_hover_ais(out);
}

nlohmann::json Sketch_underlay::to_json(const Ezy_asset_store& store) const { return m_impl->to_json(store); }

bool Sketch_underlay::from_json(const nlohmann::json& j, Ezy_asset_store& store) { return m_impl->from_json(j, store); }
