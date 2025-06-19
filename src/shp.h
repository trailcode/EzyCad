#pragma once

#include <AIS_Shape.hxx>
#include <array>

#include "result.h"

class AIS_InteractiveContext;

enum class Shape_type
{
  Extruded,
  Revolved,
  Chamfer,
  _count
};

// Corresponds to the above
constexpr std::array<std::string_view, static_cast<size_t>(Shape_type::_count)> c_ShapeType_names = {
    "Extruded",
    "Revolved",
    "Chamfer"};

class Shape_base : public AIS_Shape
{
 public:
  virtual ~Shape_base();
  const std::string&      get_name() const;
  void                    set_name(const std::string& name);
  Shape_type              get_type() const;
  static Shape_type       get_type(const std::string& shp_type_name);
  const std::string_view& get_type_str() const;
  AIS_DisplayMode         get_disp_mode() const;
  void                    set_disp_mode(const AIS_DisplayMode mode);
  bool                    get_visible() const;
  void                    set_visible(const bool visible);
  void                    set_selection_mode(const TopAbs_ShapeEnum mode);

 protected:
  Shape_base(AIS_InteractiveContext& ctx,
            const TopoDS_Shape&     shp,
            const std::string&      name,
            const Shape_type        type);

  void update_display_();

  AIS_InteractiveContext& m_ctx;
  const Shape_type        m_type;
  std::string             m_name;
  AIS_DisplayMode         m_disp_mode;
  bool                    m_visible;
  TopAbs_ShapeEnum        m_selection_mode;
};

class ExtrudedShp : public Shape_base
{
 public:
  ExtrudedShp(AIS_InteractiveContext& ctx, const TopoDS_Shape& shp);

 private:
};

class RevolvedShp : public Shape_base
{
 public:
  RevolvedShp(AIS_InteractiveContext& ctx, const TopoDS_Shape& shp);

 private:
};

class ChamferShp : public Shape_base
{
 public:
  ChamferShp(AIS_InteractiveContext& ctx, const TopoDS_Shape& shp);

 private:
};

using ShapeBase_ptr   = opencascade::handle<Shape_base>;
using ExtrudedShp_ptr = opencascade::handle<ExtrudedShp>;
using RevolvedShp_ptr = opencascade::handle<RevolvedShp>;
using ChamferShp_ptr  = opencascade::handle<ChamferShp>;

using ExtrudedShp_rslt = Result<ExtrudedShp_ptr>;
using RevolvedShp_rslt = Result<RevolvedShp_ptr>;