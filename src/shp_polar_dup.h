#pragma once

#include "shp_operation.h"

class Shp_polar_dup : private Shp_operation_base
{
 public:
  Shp_polar_dup(Occt_view& view);

  [[nodiscard]] Status move_point(const ScreenCoords& screen_coords);
  [[nodiscard]] Status add_point(const ScreenCoords& screen_coords);
  // bool                 can_polar_dup() const;
  double               get_polar_angle() const;
  void                 set_polar_angle(const double angle);
  size_t               get_num_elms() const;
  void                 set_num_selms(const size_t num);
  bool                 get_rotate_dups() const;
  void                 set_rotate_dups(const bool rotate);
  bool                 get_combine_dups() const;
  void                 set_combine_dups(const bool combine);

  [[nodiscard]] Status dup();
  void                 reset();

 private:
  gp_Pnt                     m_shps_center;
  AIS_Shape_ptr              m_polar_arm;
  std::optional<gp_Pnt2d>    m_polar_arm_end;
  std::optional<gp_Pnt2d>    m_polar_arm_origin;
  double                     m_polar_angle {360.0};
  bool                       m_rotate_dups {true};
  bool                       m_combine_dups {true};
  size_t                     m_num_elms {5};
};