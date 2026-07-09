#include "shp_operation.h"

#include <Graphic3d_MaterialAspect.hxx>
#include <unordered_set>

#include "gui_occt_view.h"

Shp_operation_base::Shp_operation_base(Occt_view& view)
    : m_view(view)
{
}

GUI& Shp_operation_base::gui() { return m_view.gui(); }

Occt_view& Shp_operation_base::view() { return m_view; }

AIS_InteractiveContext& Shp_operation_base::ctx() { return view().ctx(); }

std::vector<Shp_ptr> Shp_operation_base::get_selected_shps_() const
{
  std::vector<Shp_ptr>           ret;
  std::unordered_set<const Shp*> seen;
  for (AIS_Shape_ptr& obj : m_view.get_selected())
    if (Shp_ptr shp = Shp_ptr::DownCast(obj); !shp.IsNull())
      if (seen.insert(shp.get()).second)
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
  m_shps = get_selected_shps_();
  if (m_shps.size() < 2)
  {
    m_shps.clear();
    return Status::user_error("Select two or more shapes.");
  }

  return Status::ok();
}

void Shp_operation_base::delete_operation_shps_()
{
  std::vector<AIS_Shape_ptr> to_delete;
  for (Shp_ptr& shp : m_shps)
    to_delete.push_back(shp);

  m_view.delete_(to_delete);
  m_shps.clear();
}

void Shp_operation_base::operation_shps_finalize_()
{
  for (Shp_ptr& shape : m_shps)
  {
    AIS_Shape_ptr s = shape;
    view().bake_transform_into_geometry(s);
  }
}

void Shp_operation_base::operation_shps_cancel_()
{
  for (Shp_ptr& shape : m_shps)
    shape->ResetTransformation();
}

AIS_Shape_ptr Shp_operation_base::get_shape_(const ScreenCoords& screen_coords) { return m_view.get_shape(screen_coords); }

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

void Shp_operation_base::add_shp_(Shp_ptr& shp) { m_view.add_shp_(shp); }

void Shp_operation_base::replace_picked_shape_(Shp_ptr& old_shp, Shp_ptr& new_shp, const std::string& name)
{
  if (old_shp.IsNull() || new_shp.IsNull())
    return;

  Occt_view& v = view();
  v.apply_camera_projection();

  if (v.shape_list_hover() == old_shp)
    v.set_shape_list_hover(nullptr);

  ctx().ClearSelected(false);
  ctx().Unhilight(old_shp, false);
  ctx().Remove(old_shp, false);
  v.get_shapes().remove(old_shp);

  new_shp->set_name(name);
  add_shp_(new_shp);
  copy_shape_material_from_(new_shp, old_shp);
  ctx().Display(new_shp, new_shp->get_disp_mode(), AIS_Shape::SelectionMode(v.get_shp_selection_mode()), true);
  v.redraw_view();
}

void Shp_operation_base::copy_shape_material_from_(Shp_ptr& dest, const Shp_ptr& src)
{
  if (dest.IsNull() || src.IsNull())
    return;

  const int nmat    = Graphic3d_MaterialAspect::NumberOfMaterials();
  int       mat_idx = src->Material();
  if (mat_idx < 0 || mat_idx >= nmat)
    return;

  dest->SetMaterial(Graphic3d_MaterialAspect(static_cast<Graphic3d_NameOfMaterial>(mat_idx)));
  ctx().Redisplay(dest, true);
  ctx().UpdateCurrentViewer();
}

void Shp_operation_base::redisplay_operation_shps_after_transform_()
{
  for (Shp_ptr& shape : m_shps)
    ctx().Redisplay(shape, false);

  ctx().UpdateCurrentViewer();
}