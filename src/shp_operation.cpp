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

std::vector<AIS_Shape_ptr> Shp_operation_base::get_selected() const
{
  return m_view.get_selected();
}

void Shp_operation_base::delete_selected_()
{
  return m_view.delete_selected();
}

AIS_Shape_ptr Shp_operation_base::get_shape(const ScreenCoords& screen_coords)
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