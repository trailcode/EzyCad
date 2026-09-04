"""Create Sketcher_CreateBone.svg + .png matching FreeCAD sketcher icon style."""
from __future__ import annotations

import math
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "res" / "icons"

# Geometry in 64x64 icon space (same as FreeCAD SVGs).
c1 = (18.0, 32.0)
c2 = (46.0, 32.0)
r1, r2 = 11.0, 15.5
cut_r = 22.0

dist = c2[0] - c1[0]
R1, R2 = r1 + cut_r, r2 + cut_r
a = (R1 * R1 - R2 * R2 + dist * dist) / (2 * dist)
h = math.sqrt(max(0.0, R1 * R1 - a * a))
mx = c1[0] + a
cut_plus = (mx, c1[1] - h)
cut_minus = (mx, c1[1] + h)


def ang(c: tuple[float, float], p: tuple[float, float]) -> float:
    return math.atan2(p[1] - c[1], p[0] - c[0])


def pt_on(c: tuple[float, float], r: float, a0: float) -> tuple[float, float]:
    return (c[0] + r * math.cos(a0), c[1] + r * math.sin(a0))


def contact(c: tuple[float, float], r: float, cut: tuple[float, float]) -> tuple[float, float]:
    dx, dy = cut[0] - c[0], cut[1] - c[1]
    d = math.hypot(dx, dy)
    return (c[0] + dx * r / d, c[1] + dy * r / d)


c1p = contact(c1, r1, cut_plus)
c1m = contact(c1, r1, cut_minus)
c2p = contact(c2, r2, cut_plus)
c2m = contact(c2, r2, cut_minus)
c1o = (c1[0] - r1, c1[1])
c2o = (c2[0] + r2, c2[1])


def sample_arc(
    cx: float, cy: float, r: float, a0: float, a1: float, sweep_ccw: bool, n: int = 48
) -> list[tuple[float, float]]:
    if sweep_ccw:
        while a1 < a0:
            a1 += 2 * math.pi
        return [pt_on((cx, cy), r, a0 + (a1 - a0) * i / n) for i in range(n + 1)]
    while a1 > a0:
        a1 -= 2 * math.pi
    return [pt_on((cx, cy), r, a0 + (a1 - a0) * i / n) for i in range(n + 1)]


def poly_outline() -> list[tuple[float, float]]:
    pts: list[tuple[float, float]] = []
    # left outer c1o -> c1p CCW
    pts += sample_arc(c1[0], c1[1], r1, ang(c1, c1o), ang(c1, c1p), True)[:-1]
    # upper waist c1p -> c2p CW around cut_plus
    pts += sample_arc(cut_plus[0], cut_plus[1], cut_r, ang(cut_plus, c1p), ang(cut_plus, c2p), False)[:-1]
    # right outer c2p -> c2m CCW
    pts += sample_arc(c2[0], c2[1], r2, ang(c2, c2p), ang(c2, c2m), True)[:-1]
    # lower waist c2m -> c1m CW around cut_minus
    pts += sample_arc(cut_minus[0], cut_minus[1], cut_r, ang(cut_minus, c2m), ang(cut_minus, c1m), False)[:-1]
    # left bottom c1m -> c1o CCW
    pts += sample_arc(c1[0], c1[1], r1, ang(c1, c1m), ang(c1, c1o), True)
    return pts


def svg_arc(r: float, p_end: tuple[float, float], a0: float, a1: float, sweep_ccw: bool) -> str:
    da = (a1 - a0) if sweep_ccw else (a0 - a1)
    while da <= 0:
        da += 2 * math.pi
    large = 1 if da > math.pi else 0
    sweep = 1 if sweep_ccw else 0
    return f"A {r:.3f},{r:.3f} 0 {large},{sweep} {p_end[0]:.3f},{p_end[1]:.3f}"


def path_d() -> str:
    parts = [f"M {c1o[0]:.3f},{c1o[1]:.3f}"]
    parts.append(svg_arc(r1, c1p, ang(c1, c1o), ang(c1, c1p), True))
    parts.append(svg_arc(cut_r, c2p, ang(cut_plus, c1p), ang(cut_plus, c2p), False))
    parts.append(svg_arc(r2, c2m, ang(c2, c2p), ang(c2, c2m), True))
    parts.append(svg_arc(cut_r, c1m, ang(cut_minus, c2m), ang(cut_minus, c1m), False))
    parts.append(svg_arc(r1, c1o, ang(c1, c1m), ang(c1, c1o), True))
    parts.append("Z")
    return " ".join(parts)


def write_svg(d: str) -> None:
    svg = f"""<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<!-- EzyCad bone tool icon; style adapted from FreeCAD Sketcher icons (LGPL2+) -->
<svg width="64" height="64" viewBox="0 0 64 64" version="1.1"
   xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">
  <defs>
    <linearGradient id="edgeHi" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0" stop-color="#ffffff" stop-opacity="1"/>
      <stop offset="1" stop-color="#ffffff" stop-opacity="0"/>
    </linearGradient>
    <linearGradient id="edgeLo" x1="0" y1="1" x2="0" y2="0">
      <stop offset="0" stop-color="#d3d7cf" stop-opacity="1"/>
      <stop offset="1" stop-color="#ffffff" stop-opacity="1"/>
    </linearGradient>
    <linearGradient id="redFill" x1="-18" y1="18" x2="-22" y2="5" gradientUnits="userSpaceOnUse"
      gradientTransform="matrix(0.82607043,0,0,0.82533448,-4.0098079,1.346708)">
      <stop offset="0" stop-color="#a40000"/>
      <stop offset="1" stop-color="#ef2929"/>
    </linearGradient>
  </defs>
  <g id="bone">
    <path d="{d}" fill="none" stroke="#151819" stroke-width="8" stroke-linecap="round" stroke-linejoin="round"/>
    <path d="{d}" fill="none" stroke="#d3d7cf" stroke-width="4" stroke-linecap="round" stroke-linejoin="round"/>
    <path d="{d}" fill="none" stroke="url(#edgeLo)" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
    <path d="{d}" fill="none" stroke="url(#edgeHi)" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
  </g>
  <g transform="translate({c1[0] - 11.0:.3f},{c1[1] - 11.0:.3f}) scale(0.78)">
    <circle cx="14.1" cy="14.1" r="8.35" fill="none" stroke="#2e0000" stroke-width="2.57"/>
    <circle cx="14.1" cy="14.1" r="5.78" fill="url(#redFill)" stroke="#ef2929" stroke-width="2.57"/>
  </g>
  <g transform="translate({c2[0] - 11.0:.3f},{c2[1] - 11.0:.3f}) scale(0.78)">
    <circle cx="14.1" cy="14.1" r="8.35" fill="none" stroke="#2e0000" stroke-width="2.57"/>
    <circle cx="14.1" cy="14.1" r="5.78" fill="url(#redFill)" stroke="#ef2929" stroke-width="2.57"/>
  </g>
</svg>
"""
    (OUT_DIR / "Sketcher_CreateBone.svg").write_text(svg, encoding="utf-8")


def draw_icon(size: int, pts: list[tuple[float, float]]) -> Image.Image:
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    s = size / 64.0
    poly = [(p[0] * s, p[1] * s) for p in pts]

    for width, color in [
        (8 * s, (21, 24, 25, 255)),
        (4 * s, (211, 215, 207, 255)),
        (2.2 * s, (255, 255, 255, 230)),
    ]:
        layer = Image.new("RGBA", (size, size), (0, 0, 0, 0))
        dr = ImageDraw.Draw(layer)
        dr.line(poly + [poly[0]], fill=color, width=max(1, int(round(width))), joint="curve")
        img = Image.alpha_composite(img, layer)

    dr = ImageDraw.Draw(img)
    for c in (c1, c2):
        cx, cy = c[0] * s, c[1] * s
        R = 5.5 * s
        r = 3.8 * s
        dr.ellipse([cx - R, cy - R, cx + R, cy + R], outline=(46, 0, 0, 255), width=max(1, int(1.5 * s)))
        dr.ellipse([cx - r, cy - r, cx + r, cy + r], fill=(239, 41, 41, 255), outline=(239, 41, 41, 255))
    return img


def main() -> None:
    d = path_d()
    write_svg(d)
    pts = poly_outline()
    draw_icon(48, pts).save(OUT_DIR / "Sketcher_CreateBone.png")
    print("wrote", OUT_DIR / "Sketcher_CreateBone.svg")
    print("wrote", OUT_DIR / "Sketcher_CreateBone.png")


if __name__ == "__main__":
    main()
