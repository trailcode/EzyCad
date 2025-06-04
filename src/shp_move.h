#pragma once

#include <gp_Pln.hxx>
#include <optional>

#include "shp_operation.h"

struct Move_options
{
  bool                  axis_x {true};
  bool                  axis_y {true};
  bool                  axis_z {true};
  std::optional<double> axis_dist;
};

class Shp_move : private Shp_operation_base
{
 public:
  Shp_move(Occt_view& view);

  [[nodiscard]] Status move_selected(const ScreenCoords& screen_coords);
  void                 finalize_move_selected();
  void                 cancel_move_selected();
  Move_options&        get_opts();

 private:
  std::optional<gp_Pln> m_move_pln;
  Move_options          m_opts;
};