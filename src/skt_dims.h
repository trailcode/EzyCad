#pragma once

#include <gp_Dir2d.hxx>
#include <gp_Pnt.hxx>
#include <optional>
#include <string>
#include <vector>

#include "utl_types.h"

class Sketch;

/// Length dimensions, typed distance/angle input while drawing, and dimension-tool pick state.
class Sketch_dims
{
public:
  /// When user types in an edge length while drawing.
  struct Edge_len
  {
    gp_Dir2d dir;
    double   len;
  };

  /// Linear distance between two sketch nodes (independent of which edge connects them).
  struct Length_dimension
  {
    size_t                     node_idx_lo{};
    size_t                     node_idx_hi{};
    PrsDim_LengthDimension_ptr dim;
    bool                       visible{true};
    std::optional<double>      flyout_offset;
    std::string                name;
  };

  explicit Sketch_dims(Sketch& sketch);

  void begin_dimension_input();
  void begin_angle_input();

  bool                       dimension_visible(size_t dim_index) const;
  void                       set_dimension_visible(size_t dim_index, bool visible);
  size_t                     dimension_node_lo(size_t dim_index) const;
  size_t                     dimension_node_hi(size_t dim_index) const;
  double                     dimension_offset(size_t dim_index) const;
  void                       set_dimension_offset(size_t dim_index, double offset);
  std::string                dimension_name(size_t dim_index) const;
  void                       set_dimension_name(size_t dim_index, const std::string& name);
  PrsDim_LengthDimension_ptr length_dimension_handle(size_t dim_index) const;

  void toggle_edge_dim_anno(const ScreenCoords& screen_coords);
  bool try_remove_length_dimension(PrsDim_LengthDimension* dim);

  [[nodiscard]] size_t     length_dimension_count() const;
  std::vector<std::string> inspector_dimension_labels() const;

  void               set_show_dims(bool show);
  [[nodiscard]] bool shows_dimensions() const;

  void refresh_all_length_dimensions();
  void purge_stale_length_dimensions();
  void remove_length_dimensions_referencing_node_(size_t node_idx);

  void json_add_length_dimension_(size_t node_a, size_t node_b, bool visible = true,
                                  std::optional<double> flyout_offset = std::nullopt, const std::string& name = {});

  void remove_displayed();
  void clear_pick_state();
  void clear_tmp_dim_anno();
  void on_finalize_elm_start();
  void on_clear_tmps();

  void update_len_dim_rubber_line_(const ScreenCoords& screen_coords);

  /// `kind` is `static_cast<int>(Sketch::Linestring_type)`.
  void check_dimension_seg_(int kind);
  void check_dimension_rubber_();

  void show_tmp_dim_preview(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b);
  void offer_dist_edit_for_segment(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, double dist);
  void offer_angle_edit_for_segment(const gp_Pnt2d& pt_a, const gp_Pnt2d& mouse_pt, double current_angle_deg);

  void on_sketch_shown();
  void on_sketch_hidden();

  [[nodiscard]] const std::vector<Length_dimension>& dimensions() const { return m_length_dimensions; }
  [[nodiscard]] std::vector<Length_dimension>&       dimensions() { return m_length_dimensions; }

  [[nodiscard]] std::optional<Edge_len>&       entered_edge_len() { return m_entered_edge_len; }
  [[nodiscard]] const std::optional<Edge_len>& entered_edge_len() const { return m_entered_edge_len; }
  [[nodiscard]] std::optional<double>&         entered_edge_angle() { return m_entered_edge_angle; }
  [[nodiscard]] const std::optional<double>&   entered_edge_angle() const { return m_entered_edge_angle; }
  [[nodiscard]] bool                           show_dim_input() const { return m_show_dim_input; }
  void                                         set_show_dim_input(bool on) { m_show_dim_input = on; }
  [[nodiscard]] bool                           show_angle_input() const { return m_show_angle_input; }
  void                                         set_show_angle_input(bool on) { m_show_angle_input = on; }

  void clear_typed_constraints();

private:
  friend class Sketch;

  std::optional<gp_Pnt> approx_sketch_interior_ref_3d_() const;
  void                  rebuild_length_dimension_display_(Length_dimension& d);
  void                  add_or_toggle_length_dim_between_node_indices_(size_t node_a, size_t node_b);
  void                  clear_len_dim_rubber_line_();

  Sketch& m_sketch;

  std::optional<Edge_len> m_entered_edge_len;
  bool                    m_show_dim_input{false};
  std::optional<double>   m_entered_edge_angle;
  bool                    m_show_angle_input{false};

  std::vector<Length_dimension> m_length_dimensions;
  std::optional<size_t>         m_len_dim_pick_anchor_node;
  AIS_Shape_ptr                 m_len_dim_rubber_shp;
  PrsDim_LengthDimension_ptr    m_tmp_dim_anno;
  bool                          m_show_dims{true};
};
