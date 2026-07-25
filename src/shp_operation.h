#pragma once

#include "shp.h"
#include "utl_types.h"

class GUI;
class Occt_view;
class AIS_InteractiveContext;
class TopoDS_Face;
class TopoDS_Wire;
class TopoDS_Edge;

class Shp_operation_base
{
protected:
  Shp_operation_base(Occt_view& view);
  GUI&                    gui();
  Occt_view&              view();
  AIS_InteractiveContext& ctx();

  std::vector<Shp_ptr> get_selected_shps_() const;
  /// Parent id for new shapes from multi-selection (see Occt_view::result_parent_id).
  Shape_id             result_parent_from_operands_() const;
  void                 assign_result_parent_(Shp_ptr& result, const std::vector<Shp_ptr>& operands);
  [[nodiscard]] Status ensure_operation_shps_();
  [[nodiscard]] Status ensure_operation_multi_shps_();
  void                 delete_operation_shps_();
  void                 operation_shps_finalize_();
  void                 operation_shps_cancel_();

  /// Deactivate pick/highlight on `m_shps` once during transform preview (selection BVHs stay
  /// at the pre-transform location when we skip Redisplay).
  void deactivate_operation_shps_pick_();
  /// Re-enable pick after bake / cancel.
  void reactivate_operation_shps_pick_();

  AIS_Shape_ptr      get_shape_(const ScreenCoords& screen_coords);
  const TopoDS_Face* get_face_(const ScreenCoords& screen_coords) const;
  const TopoDS_Wire* get_wire_(const ScreenCoords& screen_coords) const;
  const TopoDS_Edge* get_edge_(const ScreenCoords& screen_coords) const;

  void add_shp_(Shp_ptr& shp, bool use_current_group = false);

  /// Remove \a old_shp from the viewer and register \a new_shp (fillet/chamfer in-place replace).
  void replace_picked_shape_(Shp_ptr& old_shp, Shp_ptr& new_shp, const std::string& name);

  /// Replace `dest` presentation material with `src` (used after add_shp_, which applies the view default).
  void copy_shape_material_from_(Shp_ptr& dest, const Shp_ptr& src);

  /// After transform-only changes on `m_shps` (SetLocalTransformation), redraw the viewer once.
  /// Does not Redisplay/recompute presentations - the local transform is applied by
  /// UpdateTransformation(), so a recompute would only rebuild identical geometry.
  void redisplay_operation_shps_after_transform_();

  std::vector<Shp_ptr> m_shps;

private:
  Occt_view& m_view;
  bool       m_transform_pick_deactivated{false};
};