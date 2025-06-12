#pragma once

#include <gp_Pln.hxx>
#include <optional>

#include "shp_operation.h"

struct Move_options
{
  bool constr_axis_x {false};
  bool constr_axis_y {false};
  bool constr_axis_z {false};
};

class Shp_move : private Shp_operation_base
{
  // Responsible for handling the movement of selected shapes in a 3D view.
  // It handles user interactions for moving shapes, including axis constraints, direct value entry,
  // and finalizing or canceling the move operation.
 public:
  Shp_move(Occt_view& view);

  [[nodiscard]] Status move_selected(const ScreenCoords& screen_coords);
  void                 show_dist_edit(const ScreenCoords& screen_coords);
  void                 finalize_move_selected();
  void                 cancel_move_selected();
  Move_options&        get_opts();

 private:
  struct Deltas
  {
    // User entered values;
    std::optional<double> override_x;
    std::optional<double> override_y;
    std::optional<double> override_z;

    gp_XYZ delta {0, 0, 0};
  };

  void check_finalize_();
  void post_opts_();

  std::optional<gp_Pln> m_move_pln;
  Move_options          m_opts;
  Deltas                m_delta;
};