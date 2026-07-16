#pragma once

#include <AIS_Shape.hxx>
#include <AIS_DisplayMode.hxx>
#include <cstdint>

#include "utl.h"

class AIS_InteractiveContext;

using Shape_id = uint64_t;

class Shp : public AIS_Shape
{
public:
  Shp(AIS_InteractiveContext& ctx, const TopoDS_Shape& shp);
  virtual ~Shp();

  Shape_id           get_id() const;
  void               set_id(Shape_id id);
  const std::string& get_name() const;
  void               set_name(const std::string& name);
  AIS_DisplayMode    get_disp_mode() const;
  void               set_disp_mode(const AIS_DisplayMode mode);
  bool               get_visible() const;
  void               set_visible(const bool visible);
  void               set_selection_mode(const TopAbs_ShapeEnum mode);

  /// Temporary display override for sketch-mode ghost/wire (does not change get_disp_mode()).
  /// When active, the shape is not selectable (no hover/selection highlight).
  void set_sketch_faint(bool enabled, AIS_DisplayMode faint_mode, float transparency);
  bool sketch_faint_active() const { return m_sketch_faint_active; }

protected:
  void            update_display_();
  void            redisplay_();
  AIS_DisplayMode effective_disp_mode_() const;

  AIS_InteractiveContext& m_ctx;
  Shape_id                m_id{0};
  std::string             m_name;
  AIS_DisplayMode         m_disp_mode;
  bool                    m_visible;
  TopAbs_ShapeEnum        m_selection_mode;
  bool                    m_sketch_faint_active{false};
  AIS_DisplayMode         m_faint_disp_mode{AIS_Shaded};
};

using Shp_ptr  = opencascade::handle<Shp>;
using Shp_rslt = Result<Shp_ptr>;
