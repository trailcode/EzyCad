#pragma once

#include <gp_Ax1.hxx>
#include <gp_Pln.hxx>
#include <optional>

#include "shp_operation.h"

struct Cyl_align_options
{
  bool flip_direction{false};
};

/// One-shot cylindrical insert align: pick moving face, pick fixed face, drag depth, bake.
class Shp_cyl_align : private Shp_operation_base
{
public:
  Shp_cyl_align(Occt_view& view);

  void begin();
  /// True while depth-drag preview is active (LMB finalizes).
  [[nodiscard]] bool   is_dragging() const;
  [[nodiscard]] Status pick(const ScreenCoords& screen_coords);
  [[nodiscard]] Status drag_depth(const ScreenCoords& screen_coords);
  void                 show_depth_edit(const ScreenCoords& screen_coords);
  void                 finalize();
  void                 cancel();
  void                 reset();

  Cyl_align_options& get_opts();
  /// Re-apply preview after Options flip toggle (no-op unless dragging).
  void apply_preview();

private:
  enum class Phase
  {
    Pick_moving,
    Pick_fixed,
    Drag_depth
  };

  void apply_preview_();
  void enter_drag_();

  Phase                 m_phase{Phase::Pick_moving};
  Cyl_align_options     m_opts;
  Shp_ptr               m_moving_shp;
  Shp_ptr               m_fixed_shp;
  std::optional<gp_Ax1> m_moving_axis;
  std::optional<gp_Ax1> m_fixed_axis;
  double                m_moving_radius{0};
  double                m_fixed_radius{0};
  double                m_axial_offset{0};
  std::optional<double> m_depth_override;
  std::optional<gp_Pln> m_drag_pln;
};
