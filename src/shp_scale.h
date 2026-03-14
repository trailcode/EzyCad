#pragma once

#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <optional>

#include "shp_operation.h"

class Shp_scale : private Shp_operation_base
{
 public:
  Shp_scale(Occt_view& view);

  [[nodiscard]] Status scale_selected(const ScreenCoords& screen_coords);
  void                 finalize();
  void                 cancel();
  void                 reset();

 private:
  [[nodiscard]] Status ensure_start_state_();
  void                 preview_scale_();

  std::optional<gp_Pln> m_scale_pln;
  std::optional<gp_Pnt> m_center;
  double                m_initial_distance {0};
  double                m_scale_factor {1.0};
};
