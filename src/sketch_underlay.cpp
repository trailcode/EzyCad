#include "sketch_underlay.h"

#include "geom.h"

#include <AIS_InteractiveContext.hxx>
#include <AIS_InteractiveObject.hxx>
#include <AIS_TexturedShape.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <Image_PixMap.hxx>
#include <Precision.hxx>
#include <cmath>
#include <cstring>
#include <gp_Pln.hxx>
#include <gp_Vec.hxx>

namespace
{

constexpr int k_max_image_dim = 8192;
constexpr std::size_t k_max_rgba_bytes = 64u * 1024u * 1024u;  // 64 MiB safety cap

std::string base64_encode(const uint8_t* data, std::size_t len)
{
  static const char tbl[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string       out;
  out.reserve(((len + 2) / 3) * 4);
  for (std::size_t i = 0; i < len; i += 3)
  {
    const std::size_t n = len - i;
    const unsigned b0 = data[i];
    const unsigned b1 = n > 1 ? data[i + 1] : 0u;
    const unsigned b2 = n > 2 ? data[i + 2] : 0u;
    const unsigned triple = (b0 << 16) | (b1 << 8) | b2;
    out.push_back(tbl[(triple >> 18) & 63]);
    out.push_back(tbl[(triple >> 12) & 63]);
    out.push_back(n > 1 ? tbl[(triple >> 6) & 63] : '=');
    out.push_back(n > 2 ? tbl[triple & 63] : '=');
  }
  return out;
}

int from_base64_char(char c)
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

bool base64_decode(const std::string& in, std::vector<uint8_t>& out)
{
  out.clear();
  if (in.empty())
    return true;
  std::size_t len = in.size();
  while (len > 0 && (in[len - 1] == '=' || in[len - 1] == '\n' || in[len - 1] == '\r' || in[len - 1] == ' '))
    --len;
  out.reserve((len * 3) / 4);
  unsigned buf = 0;
  int      bits = 0;
  for (std::size_t i = 0; i < len; ++i)
  {
    const char c = in[i];
    if (c == '\n' || c == '\r' || c == ' ')
      continue;
    const int v = from_base64_char(c);
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

/// Rec. 601 luma in 0..255 (integer). White → high, black → low.
inline unsigned luminance_u8(uint8_t r, uint8_t g, uint8_t b)
{
  return (77u * static_cast<unsigned>(r) + 150u * static_cast<unsigned>(g) + 29u * static_cast<unsigned>(b)) >> 8;
}

Handle(Image_PixMap) make_pixmap_bottom_up_rgba(const uint8_t* rgba,
                                                int               w,
                                                int               h,
                                                bool              key_white_transparent,
                                                bool              line_tint_enabled,
                                                uint8_t           tr,
                                                uint8_t           tg,
                                                uint8_t           tb)
{
  Handle(Image_PixMap) pix = new Image_PixMap();
  if (!pix->InitTrash(Image_Format_RGBA, static_cast<Standard_Size>(w), static_cast<Standard_Size>(h)))
    return {};

  const Standard_Size rowBytes = static_cast<Standard_Size>(w) * 4u;
  uint8_t*            dst      = pix->ChangeData();
  for (int y = 0; y < h; ++y)
  {
    const uint8_t* srcRow = rgba + static_cast<std::size_t>(y) * rowBytes;
    uint8_t*       dstRow = dst + static_cast<std::size_t>(h - 1 - y) * rowBytes;
    if (!key_white_transparent && !line_tint_enabled)
    {
      std::memcpy(dstRow, srcRow, static_cast<std::size_t>(rowBytes));
      continue;
    }
    for (Standard_Size x = 0; x < rowBytes; x += 4)
    {
      uint8_t        r = srcRow[x + 0];
      uint8_t        g = srcRow[x + 1];
      uint8_t        b = srcRow[x + 2];
      const uint8_t  a = srcRow[x + 3];
      unsigned       a2;
      if (key_white_transparent)
      {
        const unsigned L = luminance_u8(r, g, b);
        a2               = (static_cast<unsigned>(a) * (255u - L)) / 255u;
      }
      else
        a2 = a;

      if (line_tint_enabled && a2 > 0)
      {
        r = tr;
        g = tg;
        b = tb;
      }

      dstRow[x + 0] = r;
      dstRow[x + 1] = g;
      dstRow[x + 2] = b;
      dstRow[x + 3] = static_cast<uint8_t>(a2);
    }
  }
  return pix;
}

}  // namespace

void Sketch_underlay::remove_ais_(AIS_InteractiveContext& ctx)
{
  if (!m_ais.IsNull())
  {
    ctx.Remove(m_ais, false);
    m_ais.Nullify();
  }
}

bool Sketch_underlay::set_image_rgba(std::vector<uint8_t>&& rgba, int w, int h)
{
  if (w <= 0 || h <= 0 || rgba.size() < static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4u)
    return false;
  if (w > k_max_image_dim || h > k_max_image_dim)
    return false;
  if (rgba.size() > k_max_rgba_bytes)
    return false;

  m_rgba = std::move(rgba);
  m_w    = w;
  m_h    = h;

  // Default: centered at plane origin, 0.1 model units per image pixel (adjust in UI).
  const double s = 0.1;
  m_axis_u       = gp_Vec2d(static_cast<double>(m_w) * s, 0.);
  m_axis_v       = gp_Vec2d(0., static_cast<double>(m_h) * s);
  m_base         = gp_Pnt2d(-0.5 * m_axis_u.X(), -0.5 * m_axis_v.Y());
  return true;
}

void Sketch_underlay::set_affine(const gp_Pnt2d& base, const gp_Vec2d& axis_u, const gp_Vec2d& axis_v)
{
  m_base   = base;
  m_axis_u = axis_u;
  m_axis_v = axis_v;
}

void Sketch_underlay::set_opacity(float opaque01)
{
  if (opaque01 < 0.f)
    opaque01 = 0.f;
  if (opaque01 > 1.f)
    opaque01 = 1.f;
  m_opacity = opaque01;
  if (!m_ais.IsNull())
    m_ais->SetTransparency(1.0 - static_cast<double>(m_opacity));
}

void Sketch_underlay::set_visible(bool v)
{
  m_visible = v;
}

void Sketch_underlay::set_key_white_transparent(bool on)
{
  m_key_white_transparent = on;
}

void Sketch_underlay::set_line_tint_enabled(bool on)
{
  m_line_tint_enabled = on;
}

void Sketch_underlay::set_line_tint_rgb(uint8_t r, uint8_t g, uint8_t b)
{
  m_tint_r = r;
  m_tint_g = g;
  m_tint_b = b;
}

void Sketch_underlay::line_tint_rgb(uint8_t& r, uint8_t& g, uint8_t& b) const
{
  r = m_tint_r;
  g = m_tint_g;
  b = m_tint_b;
}

void Sketch_underlay::build_ais_(const gp_Pln& pln, AIS_InteractiveContext& ctx)
{
  remove_ais_(ctx);
  if (!has_image())
    return;

  gp_Vec nudge(pln.Axis().Direction());
  nudge.Multiply(-10.0 * Precision::Confusion());

  const gp_Pnt p00 = to_3d(pln, m_base).Translated(nudge);
  const gp_Pnt p10 = to_3d(pln, m_base.Translated(m_axis_u)).Translated(nudge);
  const gp_Pnt p11 = to_3d(pln, m_base.Translated(m_axis_u).Translated(m_axis_v)).Translated(nudge);
  const gp_Pnt p01 = to_3d(pln, m_base.Translated(m_axis_v)).Translated(nudge);

  BRepBuilderAPI_MakeWire wireMk;
  wireMk.Add(BRepBuilderAPI_MakeEdge(p00, p10).Edge());
  wireMk.Add(BRepBuilderAPI_MakeEdge(p10, p11).Edge());
  wireMk.Add(BRepBuilderAPI_MakeEdge(p11, p01).Edge());
  wireMk.Add(BRepBuilderAPI_MakeEdge(p01, p00).Edge());
  if (!wireMk.IsDone())
    return;

  BRepBuilderAPI_MakeFace faceMk(wireMk.Wire(), Standard_True);
  if (!faceMk.IsDone())
    return;

  Handle(Image_PixMap) pix = make_pixmap_bottom_up_rgba(
      m_rgba.data(),
      m_w,
      m_h,
      m_key_white_transparent,
      m_line_tint_enabled,
      m_tint_r,
      m_tint_g,
      m_tint_b);
  if (pix.IsNull())
    return;

  m_ais = new AIS_TexturedShape(faceMk.Face());
  m_ais->SetTexturePixMap(pix);
  m_ais->SetTextureMapOn();
  m_ais->DisableTextureModulate();
  m_ais->SetTextureRepeat(Standard_False, 1., 1.);
  m_ais->SetTransparency(1.0 - static_cast<double>(m_opacity));
  m_ais->SetDisplayMode(3);

  if (m_visible)
  {
    ctx.Display(m_ais, 3, 0, false);
    ctx.Deactivate(opencascade::handle<AIS_InteractiveObject>(m_ais));
  }
}

void Sketch_underlay::rebuild_and_display(const gp_Pln& pln, AIS_InteractiveContext& ctx)
{
  remove_ais_(ctx);
  if (!has_image() || !m_visible)
    return;
  build_ais_(pln, ctx);
}

void Sketch_underlay::erase(AIS_InteractiveContext& ctx)
{
  remove_ais_(ctx);
}

void Sketch_underlay::sync_visibility(const gp_Pln& pln, AIS_InteractiveContext& ctx)
{
  if (!has_image())
    return;
  if (m_visible)
    rebuild_and_display(pln, ctx);
  else
    remove_ais_(ctx);
}

nlohmann::json Sketch_underlay::to_json() const
{
  using nlohmann::json;
  json j;
  if (!has_image())
    return j;
  j["rgba_b64"] = base64_encode(m_rgba.data(), m_rgba.size());
  j["w"]        = m_w;
  j["h"]        = m_h;
  j["base"]     = json::object({{"x", m_base.X()}, {"y", m_base.Y()}});
  j["axis_u"]   = json::object({{"x", m_axis_u.X()}, {"y", m_axis_u.Y()}});
  j["axis_v"]   = json::object({{"x", m_axis_v.X()}, {"y", m_axis_v.Y()}});
  j["opacity"]               = m_opacity;
  j["visible"]               = m_visible;
  j["key_white_transparent"] = m_key_white_transparent;
  j["line_tint_enabled"]     = m_line_tint_enabled;
  j["line_tint_rgb"]         = nlohmann::json::array({m_tint_r, m_tint_g, m_tint_b});
  return j;
}

bool Sketch_underlay::from_json(const nlohmann::json& j)
{
  using nlohmann::json;
  if (!j.is_object() || !j.contains("rgba_b64") || !j.contains("w") || !j.contains("h"))
    return false;
  const int w = j.at("w").get<int>();
  const int h = j.at("h").get<int>();
  if (w <= 0 || h <= 0 || w > k_max_image_dim || h > k_max_image_dim)
    return false;

  std::vector<uint8_t> decoded;
  if (!base64_decode(j.at("rgba_b64").get<std::string>(), decoded))
    return false;
  if (decoded.size() < static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4u)
    return false;
  if (decoded.size() > k_max_rgba_bytes)
    return false;

  m_rgba = std::move(decoded);
  m_w    = w;
  m_h    = h;

  if (j.contains("base"))
    m_base = gp_Pnt2d(j["base"].at("x").get<double>(), j["base"].at("y").get<double>());
  if (j.contains("axis_u"))
    m_axis_u = gp_Vec2d(j["axis_u"].at("x").get<double>(), j["axis_u"].at("y").get<double>());
  if (j.contains("axis_v"))
    m_axis_v = gp_Vec2d(j["axis_v"].at("x").get<double>(), j["axis_v"].at("y").get<double>());

  m_opacity                 = j.value("opacity", 0.88f);
  m_visible                 = j.value("visible", true);
  m_key_white_transparent   = j.value("key_white_transparent", true);
  m_line_tint_enabled       = j.value("line_tint_enabled", true);
  if (j.contains("line_tint_rgb") && j["line_tint_rgb"].is_array() && j["line_tint_rgb"].size() >= 3)
  {
    m_tint_r = static_cast<uint8_t>(j["line_tint_rgb"][0].get<int>());
    m_tint_g = static_cast<uint8_t>(j["line_tint_rgb"][1].get<int>());
    m_tint_b = static_cast<uint8_t>(j["line_tint_rgb"][2].get<int>());
  }
  if (m_opacity < 0.f)
    m_opacity = 0.f;
  if (m_opacity > 1.f)
    m_opacity = 1.f;

  m_ais.Nullify();
  return true;
}
