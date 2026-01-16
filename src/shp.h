#pragma once

#include <AIS_Shape.hxx>
#include <array>

#include "utl_result.h"

class AIS_InteractiveContext;

enum class Shape_type
{
  Extruded,
  Revolved,
  Chamfer,
  Fillet,
  _count
};

// Corresponds to the above
constexpr std::array<std::string_view, static_cast<size_t>(Shape_type::_count)> c_ShapeType_names = {
    "Extruded",
    "Revolved",
    "Chamfer",
    "Fillet"};

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

class Extruded_shp : public Shape_base
{
 public:
  Extruded_shp(AIS_InteractiveContext& ctx, const TopoDS_Shape& shp);

 private:
};

class Revolved_shp : public Shape_base
{
 public:
  Revolved_shp(AIS_InteractiveContext& ctx, const TopoDS_Shape& shp);

 private:
};

class Chamfer_shp : public Shape_base
{
 public:
  Chamfer_shp(AIS_InteractiveContext& ctx, const TopoDS_Shape& shp);

 private:
};

class Fillet_shp : public Shape_base
{
 public:
  Fillet_shp(AIS_InteractiveContext& ctx, const TopoDS_Shape& shp);

 private:
};

using ShapeBase_ptr    = opencascade::handle<Shape_base>;
using extruded_shp_ptr = opencascade::handle<Extruded_shp>;
using revolved_shp_ptr = opencascade::handle<Revolved_shp>;
using chamfer_shp_ptr  = opencascade::handle<Chamfer_shp>;
using fillet_shp_ptr   = opencascade::handle<Fillet_shp>;

using extruded_shp_rslt = Result<extruded_shp_ptr>;
using revolved_shp_rslt = Result<revolved_shp_ptr>;