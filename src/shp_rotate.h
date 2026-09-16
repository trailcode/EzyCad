#pragma once

#include "shp_operation.h"
#include "shp_transform.h"

#include <gp_Pln.hxx>

enum class Rotation_axis
{
  View_to_object, // Rotate around view axis through the current pivot
  X_axis,         // Local or world X (see Transform_space)
  Y_axis,
  Z_axis
};

class Shp_rotate : private Shp_operation_base
{
public:
  Shp_rotate(Occt_view& view);

  /// Seed operands from the shapes selected when Rotate mode was entered (may be empty).
  void begin(std::vector<Shp_ptr> shps);
  /// True when operands are loaded (LMB will finalize rather than AIS-select).
  [[nodiscard]] bool   has_operation_shps() const { return !m_shps.empty(); }
  [[nodiscard]] Status rotate_selected(const ScreenCoords& screen_coords);
  [[nodiscard]] Status show_angle_edit(const ScreenCoords& screen_coords);
  void                 finalize();
  void                 cancel();

  void          set_rotation_axis(Rotation_axis axis);
  Rotation_axis get_rotation_axis() const { return m_rotation_axis; }
  /// Pivot / axis vis after Options Local/World change.
  void on_transform_space_changed();

private:
  [[nodiscard]] Status         ensure_start_state_();
  [[nodiscard]] Transform_axes current_axes_();
  [[nodiscard]] gp_Dir         current_axis_dir_();
  [[nodiscard]] gp_Pln         choose_rotate_pln_(const gp_Dir& axis_dir);
  void                         capture_drag_frame_();
  void                         refresh_guides_();
  void                         preview_rotate_();
  void                         reset();
  void                         update_rotation_axis_();
  void                         update_rotation_center_();
  void                         clear_rotation_vis_();

  friend class Shp_rotate_access;

  std::optional<gp_Pln> m_rotate_pln;
  std::optional<gp_Dir> m_captured_axis_dir;
  std::optional<gp_Pnt> m_initial_mouse_pos;
  std::optional<gp_Pnt> m_center;
  double                m_angle{0};
  Rotation_axis         m_rotation_axis{Rotation_axis::View_to_object};
  AIS_Shape_ptr         m_rotation_axis_vis;
  AIS_Shape_ptr         m_rotation_center_vis;
};
