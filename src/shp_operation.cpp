#include "shp_operation.h"
#include "occt_view.h"

Shp_operation_base::Shp_operation_base(Occt_view& view)
    : m_view(view) {}

GUI& Shp_operation_base::gui()
{
  return m_view.gui();
}

Occt_view& Shp_operation_base::view()
{
  return m_view;
}

AIS_InteractiveContext& Shp_operation_base::ctx()
{
  return view().ctx();
}

std::vector<ShapeBase_ptr> Shp_operation_base::get_selected_shps_() const
{
  std::vector<ShapeBase_ptr> ret;
  for (AIS_Shape_ptr& obj : m_view.get_selected())
    if (auto shp = ShapeBase_ptr::DownCast(obj); shp)
      ret.push_back(shp);

  return ret;
}

Status Shp_operation_base::ensure_operation_shps_()
{
  if (m_shps.empty())
  {
    m_shps = get_selected_shps_();
    if (m_shps.empty())
      return Status::user_error("Select one or more shapes.");
  }

  return Status::ok();
}

[[nodiscard]] Status Shp_operation_base::ensure_operation_multi_shps_()
{
  if (m_shps.empty())
  {
    m_shps = get_selected_shps_();
    if (m_shps.size() < 2)
    {
      m_shps.clear();
      return Status::user_error("Select two or more shapes.");
    }
  }

  return Status::ok();
}

void Shp_operation_base::delete_operation_shps_()
{
  std::vector<AIS_Shape_ptr> to_delete;
  for (ShapeBase_ptr& shp : m_shps)
    to_delete.push_back(shp);

  m_view.delete_(to_delete);
  m_shps.clear();
}

AIS_Shape_ptr Shp_operation_base::get_shape_(const ScreenCoords& screen_coords)
{
  return m_view.get_shape(screen_coords);
}

const TopoDS_Face* Shp_operation_base::get_face_(const ScreenCoords& screen_coords) const
{
  return m_view.get_face_(screen_coords);
}

const TopoDS_Wire* Shp_operation_base::get_wire_(const ScreenCoords& screen_coords) const
{
  return m_view.get_wire_(screen_coords);
}

const TopoDS_Edge* Shp_operation_base::get_edge_(const ScreenCoords& screen_coords) const
{
  return m_view.get_edge_(screen_coords);
}

void Shp_operation_base::add_shp_(ShapeBase_ptr& shp)
{
  m_view.add_shp_(shp);
}