#pragma once

#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <optional>

#include "shp_operation.h"

class Shp_scale : private Shp_operation_base
{
public:
  Shp_scale(Occt_view& view);

  /// Seed operands from the shapes selected when Scale mode was entered (may be empty).
  void                 begin(std::vector<Shp_ptr> shps);
  /// True when operands are loaded (LMB will finalize rather than AIS-select).
  [[nodiscard]] bool   has_operation_shps() const { return !m_shps.empty(); }
  [[nodiscard]] Status scale_selected(const ScreenCoords& screen_coords);
  void                 finalize();
  void                 cancel();
  void                 reset();

private:
  [[nodiscard]] Status ensure_start_state_();
  void                 preview_scale_();

  std::optional<gp_Pln> m_scale_pln;
  std::optional<gp_Pnt> m_center;
  double                m_initial_distance{0};
  double                m_scale_factor{1.0};
};
