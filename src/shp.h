#pragma once

#include <AIS_Shape.hxx>

#include "utl_result.h"

class AIS_InteractiveContext;

class Shp : public AIS_Shape {
 public:
  Shp(AIS_InteractiveContext& ctx, const TopoDS_Shape& shp);
  virtual ~Shp();

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
  std::string             m_name;
  AIS_DisplayMode         m_disp_mode;
  bool                    m_visible;
  TopAbs_ShapeEnum        m_selection_mode;
};

using Shp_ptr  = opencascade::handle<Shp>;
using Shp_rslt = Result<Shp_ptr>;
