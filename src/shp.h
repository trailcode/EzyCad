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

protected:
  void update_display_();

  AIS_InteractiveContext& m_ctx;
  Shape_id                m_id{0};
  std::string             m_name;
  AIS_DisplayMode         m_disp_mode;
  bool                    m_visible;
  TopAbs_ShapeEnum        m_selection_mode;
};

using Shp_ptr  = opencascade::handle<Shp>;
using Shp_rslt = Result<Shp_ptr>;
