#pragma once

#include <AIS_Shape.hxx>
#include <AIS_DisplayMode.hxx>
#include <cstdint>

#include "utl.h"

class AIS_InteractiveContext;

using Shape_id = uint64_t;

class Shp;
using Shp_ptr = opencascade::handle<Shp>;

/// Document shape or organizational group (group nodes have no viewer display).
class Shp : public AIS_Shape
{
public:
  Shp(AIS_InteractiveContext& ctx, const TopoDS_Shape& shp);
  /// Organizational group (empty compound geometry; never displayed).
  static Shp_ptr create_group(AIS_InteractiveContext& ctx, const std::string& name);
  virtual ~Shp();

  Shape_id           get_id() const;
  void               set_id(Shape_id id);
  const std::string& get_name() const;
  void               set_name(const std::string& name);
  AIS_DisplayMode    get_disp_mode() const;
  void               set_disp_mode(const AIS_DisplayMode mode);
  bool               get_visible() const;
  /// User visibility preference (does not alone control viewer Erase/Display when overlays apply).
  void               set_visible(const bool visible);
  void               set_selection_mode(const TopAbs_ShapeEnum mode);

  bool     is_group() const { return m_is_group; }
  Shape_id get_parent_id() const { return m_parent_id; }
  void     set_parent_id(Shape_id parent_id) { m_parent_id = parent_id; }
  int      get_sibling_order() const { return m_sibling_order; }
  void     set_sibling_order(int order) { m_sibling_order = order; }

  /// Show or erase in the interactive context without changing get_visible().
  /// No-op for group nodes.
  void apply_context_shown(bool shown);

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
  bool                    m_is_group{false};
  Shape_id                m_parent_id{0};
  int                     m_sibling_order{0};
};

using Shp_rslt = Result<Shp_ptr>;
