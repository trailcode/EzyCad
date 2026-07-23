#pragma once

#include "shp_operation.h"

#include <Bnd_Box.hxx>
#include <gp_Ax3.hxx>
#include <gp_Pln.hxx>
#include <TopoDS_Shape.hxx>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#ifndef __EMSCRIPTEN__
#  include <future>
#endif

enum class Cross_section_plane
{
  XY,
  XZ,
  YZ
};

struct Cross_section_geometry
{
  TopoDS_Shape shape;
  std::size_t  edge_count{0};
  std::size_t  line_count{0};
  std::size_t  circle_count{0};
  std::size_t  ellipse_count{0};
  std::size_t  bspline_count{0};
  std::size_t  other_curve_count{0};
};

/// Compute a cross-section with a plane from \a frame local coordinates. Offset is in
/// model units along the selected local plane normal. Interactive preview uses one
/// shared plane for the whole selection (first-shape axes, selection bbox center).
Result<Cross_section_geometry> cross_section_shape(const TopoDS_Shape& shape, const gp_Ax3& frame, Cross_section_plane plane,
                                                   double offset);

class Shp_cross_section : private Shp_operation_base
{
public:
  explicit Shp_cross_section(Occt_view& view);
  ~Shp_cross_section();

  /// Blocking preview (plane + section). Used on mode enter and in tests.
  [[nodiscard]] Status preview_selected();
  [[nodiscard]] Status preview(const std::vector<Shp_ptr>& shapes);

  /// Non-blocking: update plane annotation now and enqueue section compute (running + latest pending).
  /// Returns immediately with a plane-build failure, or ok when the section job was enqueued.
  [[nodiscard]] Status request_preview_selected();
  [[nodiscard]] Status request_preview(const std::vector<Shp_ptr>& shapes);

  /// Drain finished section jobs / advance WASM chunks; start pending. Call each Options frame.
  /// Returns a status when a job for the current generation completes (success or failure toast).
  [[nodiscard]] std::optional<Status> poll();

  void clear();

  /// Half-space clip: keep the positive-normal side of each selected solid, delete the inputs,
  /// and add the clipped results (undoable).
  [[nodiscard]] Status clip_selected();
  [[nodiscard]] Status clip(const std::vector<Shp_ptr>& shapes);

  Cross_section_plane get_plane() const { return m_plane; }
  void                set_plane(Cross_section_plane plane) { m_plane = plane; }
  double              get_offset_display() const { return m_offset_display; }
  void                set_offset_display(double offset) { m_offset_display = offset; }
  bool                get_invert_normal() const { return m_invert_normal; }
  /// Flips the cutting-plane normal (and annotation arrow). Negates offset so the plane stays put.
  void                set_invert_normal(bool invert);
  bool                get_hide_back_side() const { return m_hide_back_side; }
  void                set_hide_back_side(bool hide) { m_hide_back_side = hide; }
  bool                has_preview() const { return !m_preview.IsNull(); }
  bool                section_busy() const;
  [[nodiscard]] bool  selection_stale() const;
  [[nodiscard]] bool  preview_inputs_stale() const;
  void                acknowledge_current_selection();
  [[nodiscard]] bool  try_get_offset_range_display(double& out_min, double& out_max);

private:
  struct Shared_plane
  {
    std::vector<Shp_ptr>      shapes;
    std::vector<TopoDS_Shape> world_shapes;
    std::vector<std::string>  shape_names;
    Bnd_Box                   bounds;
    gp_Pln                    plane;
  };

  struct Section_request
  {
    std::uint64_t             generation{0};
    std::vector<TopoDS_Shape> world_shapes;
    std::vector<std::string>  shape_names;
    gp_Pln                    plane;
  };

  struct Section_result
  {
    std::uint64_t generation{0};
    Status        status{Result_status::Success};
    TopoDS_Shape  compound;
  };

  static std::vector<Shape_id> selection_ids_(const std::vector<Shp_ptr>& shapes);
  void                         acknowledge_inputs_(const std::vector<Shp_ptr>& shapes);
  void                         clear_section_wires_();
  void                         clear_plane_annotation_();
  void                         clear_preview_ais_();
  void                         clear_ais_clips_();
  void                         apply_ais_clips_(const std::vector<Shp_ptr>& shapes, const gp_Pln& plane);
  void                         display_plane_annotation_(const Bnd_Box& bounds, const gp_Pln& plane);
  void                         display_section_wires_(const TopoDS_Shape& compound);
  void                         cancel_section_jobs_();
  void                         enqueue_section_(const Shared_plane& plane_ctx);
  void                         start_section_job_(Section_request req);
  [[nodiscard]] static Section_result compute_section_result_(Section_request req, std::atomic<bool>* cancel);
  [[nodiscard]] std::optional<Status> finish_section_result_(Section_result result);
  [[nodiscard]] Result<Shared_plane>  build_shared_plane_(const std::vector<Shp_ptr>& shapes);
  [[nodiscard]] Status                wait_section_();

  Cross_section_plane     m_plane{Cross_section_plane::XY};
  double                  m_offset_display{0.0};
  bool                    m_invert_normal{false};
  bool                    m_hide_back_side{true};
  Cross_section_plane     m_acked_plane{Cross_section_plane::XY};
  double                  m_acked_offset_display{0.0};
  bool                    m_acked_invert_normal{false};
  bool                    m_acked_hide_back_side{true};
  std::vector<Shape_id>   m_acked_selection_ids;
  AIS_Shape_ptr           m_preview;
  AIS_Shape_ptr           m_plane_fill;
  AIS_Shape_ptr           m_plane_lines;
  Graphic3d_ClipPlane_ptr m_ais_clip_plane;
  std::vector<Shp_ptr>    m_ais_clipped_shapes;

  std::atomic<std::uint64_t> m_generation{0};
  std::atomic<bool>          m_cancel{false};
  std::optional<Section_request> m_pending;
  Status                         m_last_section_status{Result_status::Success};

#ifndef __EMSCRIPTEN__
  std::future<Section_result> m_running;
  bool                        m_running_active{false};
#else
  struct Chunked_job
  {
    Section_request                                request;
    std::size_t                                    next{0};
    std::vector<Result<Cross_section_geometry>>    results;
  };
  std::optional<Chunked_job> m_chunked;
#endif
};
