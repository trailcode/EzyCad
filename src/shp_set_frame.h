#pragma once

#include "shp_operation.h"

/// Pick a planar or cylindrical face to redefine a solid's local frame (Shape List only).
class Shp_set_frame : private Shp_operation_base
{
public:
  enum class Pick
  {
    Planar_face,
    Cylindrical_face
  };

  explicit Shp_set_frame(Occt_view& view);

  void begin(const Shp_ptr& target, Pick pick);
  [[nodiscard]] Status pick(const ScreenCoords& screen_coords);
  void                 cancel();
  [[nodiscard]] Pick   get_pick() const { return m_pick; }
  [[nodiscard]] bool   has_target() const { return !m_target.IsNull(); }

private:
  Pick    m_pick{Pick::Planar_face};
  Shp_ptr m_target;
};
