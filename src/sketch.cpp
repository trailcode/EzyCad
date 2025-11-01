#include "sketch.h"

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>
#include <BRepTools.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <Graphic3d_AspectFillArea3d.hxx>
#include <PrsDim_LengthDimension.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>
#include <V3d_View.hxx>
#include <map>
#include <unordered_set>

#include "geom.h"
#include "gui.h"
#include "occt_view.h"
#include "utl.h"

Sketch::Sketch(const std::string& name, Occt_view& view, const gp_Pln& pln)
    : m_view(view), m_ctx(view.ctx()), m_pln(pln), m_name(name), m_nodes(view, pln)
{
}

Sketch::Sketch(const std::string& name, Occt_view& view, const gp_Pln& pln, const TopoDS_Wire& outer_wire)
    : m_view(view), m_ctx(view.ctx()), m_pln(pln), m_name(name), m_nodes(view, pln)
{
  m_originating_face = new AIS_Shape(outer_wire);
  m_ctx.Display(m_originating_face, true);
  update_originating_face_style();
}

Sketch::~Sketch()
{
  auto remove_edge = [&](Edge& e)
  {
    m_ctx.Remove(e.shp, false);
    m_ctx.Remove(e.dim, false);
  };

  for (Edge& e : m_edges)
    remove_edge(e);

  for (AIS_Shape_ptr& f : m_faces)
    m_ctx.Remove(f, false);

  if (m_operation_axis.has_value())
    remove_edge(*m_operation_axis);

  m_ctx.Remove(m_originating_face, true);
}

void Sketch::add_sketch_pt(const ScreenCoords& screen_coords)
{
  switch (get_mode())
  {
      // clang-format off
    case Mode::Sketch_add_rectangle:
    case Mode::Sketch_add_rectangle_center_pt:
    case Mode::Sketch_add_square:
    case Mode::Sketch_add_circle:
    case Mode::Sketch_add_edge:                 add_line_string_pt_  (screen_coords, Linestring_type::Single); break;
    case Mode::Sketch_add_slot:                 add_line_string_pt_  (screen_coords, Linestring_type::Two); break;
    case Mode::Sketch_add_multi_edges:          add_line_string_pt_  (screen_coords, Linestring_type::Multiple); break;
    case Mode::Sketch_add_seg_circle_arc:       add_arc_circle_pt_   (screen_coords); break;
    case Mode::Sketch_operation_axis:           add_operation_axis_pt_  (screen_coords); break;
    default:
      EZY_ASSERT(false);
      // clang-format on
  }
}

void Sketch::sketch_pt_move(const ScreenCoords& screen_coords)
{
  switch (get_mode())
  {
      // clang-format off
    case Mode::Sketch_add_edge:
    case Mode::Sketch_operation_axis:
    case Mode::Sketch_add_multi_edges:     move_line_string_pt_ (screen_coords); break;
    case Mode::Sketch_add_seg_circle_arc:  move_arc_circle_pt_  (screen_coords); break;
    case Mode::Sketch_add_square:          move_square_pt_      (screen_coords); break;
    case Mode::Sketch_add_rectangle:       move_rectangle_pt_   (screen_coords); break;
    case Mode::Sketch_add_circle:          move_circle_pt_      (screen_coords); break;
    case Mode::Sketch_add_slot:            move_slot_pt_        (screen_coords); break;
      // clang-format on
    default:
      break;
  }
}

void Sketch::dimension_input(const ScreenCoords& screen_coords)
{
  m_show_dim_input = true;
  sketch_pt_move(screen_coords);
}

void Sketch::remove_edge(const Sketch_AIS_edge& to_remove)
{
  // Some shapes are composed with multiple edges, so we cannot break when a edge is found.
  for (std::list<Edge>::iterator itr = m_edges.begin(); itr != m_edges.end();)
    if (itr->shp.get() == &to_remove)
    {
      if (itr->dim)
        m_ctx.Remove(itr->dim, false);

      itr = m_edges.erase(itr);
    }
    else
      ++itr;

  update_faces_();
}

void Sketch::dbg_append_str(std::string& out) const
{
  out += "    Nodes: " + std::to_string(m_nodes.size()) + "\n";
  out += "    Edges: " + std::to_string(m_edges.size()) + "\n";
  out += "    Faces: " + std::to_string(m_faces.size()) + "\n";
  out += "Tmp Nodes: " + std::to_string(m_tmp_node_idxs.size()) + "\n";
  out += "Tmp Edges: " + std::to_string(m_tmp_edges.size()) + "\n";
}

void Sketch::update_edge_style_(AIS_Shape_ptr& shp)
{
  switch (m_edge_style)
  {
    case Edge_style::Full:
      // shp->SetWidth(1.5);
      shp->SetWidth(1.0);
      shp->SetColor(Quantity_Color(0.0, 1.0, 0.0, Quantity_TOC_RGB));
      shp->SetTransparency(0.0);

      break;

    case Edge_style::Background:
      shp->SetWidth(1.0);
      shp->SetColor(Quantity_Color(0.3, 0.3, 0.3, Quantity_TOC_RGB));
      shp->SetTransparency(0.7);
      break;

    default:
      EZY_ASSERT(false);
  }
}

void Sketch::update_originating_face_style()
{
  if (!m_originating_face)
    return;

  switch (m_edge_style)
  {
    case Edge_style::Full:
      m_originating_face->SetWidth(1.0);
      m_originating_face->SetColor(Quantity_Color(0.3, 0.0, 0.0, Quantity_TOC_RGB));
      m_originating_face->SetTransparency(0.0);

      break;

    case Edge_style::Background:
      m_originating_face->SetWidth(1.0);
      m_originating_face->SetColor(Quantity_Color(0.3, 0.3, 0.3, Quantity_TOC_RGB));
      m_originating_face->SetTransparency(0.7);
      break;

    default:
      EZY_ASSERT(false);
  }

  m_ctx.Redisplay(m_originating_face, true);
}

void Sketch::update_face_style_(AIS_Shape_ptr& shp) const
{
}

void Sketch::update_edge_shp_(Edge& edge, const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b)
{
  EZY_ASSERT(unique(pt_a, pt_b));
  TopoDS_Shape edge_shape = BRepBuilderAPI_MakeEdge(to_3d(m_pln, pt_a), to_3d(m_pln, pt_b)).Edge();
  if (edge.shp)
  {
    edge.shp->Set(edge_shape);
    update_edge_style_(edge.shp);
    m_ctx.Redisplay(edge.shp, true);
  }
  else
  {
    edge.shp = new Sketch_AIS_edge(*this, edge_shape);
    update_edge_style_(edge.shp);
    m_ctx.Display(edge.shp, true);
  }
}

void Sketch::on_enter()
{
  switch (get_mode())
  {
      // clang-format off
    case Mode::Sketch_add_square:
    case Mode::Sketch_add_rectangle:
    case Mode::Sketch_add_circle:
    case Mode::Sketch_add_edge:         check_dimension_seg_(Linestring_type::Single);    break;
    case Mode::Sketch_add_slot:         check_dimension_seg_(Linestring_type::Two);       break;
    case Mode::Sketch_add_multi_edges:  check_dimension_seg_(Linestring_type::Multiple);  break;
      // clang-format on
    default:
      break;
  }
}

// Line string related
void Sketch::add_line_string_pt_(const ScreenCoords& screen_coords, Linestring_type linestring_type)
{
  auto l = [&](size_t node_idx)
  {
    if (m_tmp_edges.size())
    {
      Edge& last_edge = m_tmp_edges.back();
      // update_edge_end_pt_(last_edge, node_idx);
      if (last_edge.node_idx_b.has_value())
      {
        // The last end edge point snapped, we are done with this line string.
        update_edge_end_pt_(last_edge, node_idx);
        finalize_elm();
        return;
      }
      else
      {
        if (node_idx == last_edge.node_idx_a)
          // We cannot have edges with zero length.
          return;

        update_edge_end_pt_(last_edge, node_idx);

        if (linestring_type == Linestring_type::Single)
        {
          // update_edge_end_pt_(last_edge, node_idx);
          finalize_elm();
          return;
        }
        else if (linestring_type == Linestring_type::Two)
          if (m_tmp_edges.size() == 2)
          {
            // update_edge_end_pt_(last_edge, node_idx);
            finalize_elm();
            return;
          }
      }
    }

    // Start a new edge
    m_tmp_edges.push_back({node_idx});
  };
  add_sketch_pt_(screen_coords, 1, l);
}

void Sketch::move_line_string_pt_(const ScreenCoords& screen_coords)
{
  auto l = [&](const std::optional<size_t>& node_idx_b, const gp_Pnt2d& pt_b)
  {
    if (m_tmp_edges.empty())
      return;

    Edge&           edge = m_tmp_edges.back();
    const gp_Pnt2d& pt_a = m_nodes[edge.node_idx_a];

    if (!unique(pt_a, pt_b))
      // Cannot have a edge with zero length
      return;

    edge.node_idx_b = node_idx_b;

    double dist = pt_a.Distance(pt_b) / m_view.get_dimension_scale();
    m_ctx.Remove(m_tmp_dim_anno, true);
    m_tmp_dim_anno = create_distance_annotation(pt_a, pt_b, m_pln);
    m_tmp_dim_anno->SetCustomValue(dist);
    m_ctx.Display(m_tmp_dim_anno, true);

    if (m_show_dim_input)
    {
      gp_Dir2d     edge_dir = get_unit_dir(pt_a, pt_b);
      ScreenCoords spos     = m_view.get_screen_coords(to_3d(m_pln, center_point(pt_a, pt_b)));

      auto l = [&, edge_dir](float new_dist, bool is_finial)
      {
        m_entered_edge_len = {
            edge_dir,
            new_dist * m_view.get_dimension_scale()};

        m_show_dim_input = !is_finial;
      };

      m_view.gui().set_dist_edit(float(dist), std::move(std::function<void(float, bool)>(l)), spos);
    }

    update_edge_shp_(edge, pt_a, pt_b);
  };
  move_sketch_pt_(screen_coords, l);
}

void Sketch::finalize_edges_()
{
  if (m_nodes.empty() || m_tmp_edges.empty())
    return;

  if (!m_tmp_edges.back().node_idx_b.has_value())
  {
    // Last edge was not finalized, remove it
    m_ctx.Remove(m_tmp_edges.back().shp, false);
    m_tmp_edges.pop_back();
  }

  std::vector<size_t> split_mid_points;
  for (Edge& e : m_tmp_edges)
  {
    EZY_ASSERT(e.node_idx_b.has_value());
    EZY_ASSERT(e.node_idx_mid.has_value());
    if (m_nodes[e.node_idx_a].is_midpoint)
      split_mid_points.push_back(e.node_idx_a);

    if (m_nodes[e.node_idx_b].is_midpoint)
      split_mid_points.push_back(*e.node_idx_b);

    if (!m_nodes[e.node_idx_mid].is_midpoint)
    {
      int hi = 0;
    }
  }

  append(m_edges, m_tmp_edges);
  m_tmp_edges.clear();

  // Ensure topology is correct if snapping on a midpoint happened.
  // These edges need to be split.
  for (size_t mid_pt_idx : split_mid_points)
    for (auto itr = m_edges.begin(), end = m_edges.end(); itr != end; ++itr)
      if (itr->node_idx_mid.has_value() && *itr->node_idx_mid == mid_pt_idx)
        // Cannot split arc circles.
        if (!itr->node_idx_arc.has_value())
        {
          // Split the edge.
          Edge edge_a {itr->node_idx_a, mid_pt_idx};
          Edge edge_b {mid_pt_idx, *itr->node_idx_b};
          update_edge_shp_(edge_a, m_nodes[itr->node_idx_a], m_nodes[mid_pt_idx]);
          update_edge_shp_(edge_b, m_nodes[itr->node_idx_b], m_nodes[mid_pt_idx]);
          m_ctx.Remove(itr->shp, false);
          m_edges.erase(itr);
          m_nodes[mid_pt_idx].is_midpoint = false;
          m_edges.emplace_back(edge_a);
          m_edges.emplace_back(edge_b);
          break;
        }

  m_nodes.hide_snap_annos();
  update_faces_();
}

// Arc circle related
void Sketch::add_arc_circle_pt_(const ScreenCoords& screen_coords)
{
  std::optional<size_t> node_idx = m_nodes.get_node(screen_coords);
  if (!node_idx)
    return;

  if (!m_start_arc_circle_node_idx)
  {
    EZY_ASSERT(m_tmp_node_idxs.empty());
    m_start_arc_circle_node_idx = m_nodes.size();
  }

  if (contains(m_tmp_node_idxs, node_idx))
    // Arc points must be unique.
    return;

  m_tmp_node_idxs.push_back(*node_idx);

  if (m_tmp_node_idxs.size() == 3)
  {
    add_arc_circle_(m_tmp_node_idxs);
    m_tmp_node_idxs.clear();
    update_faces_();
  }
}

void Sketch::move_arc_circle_pt_(const ScreenCoords& screen_coords)
{
  std::optional<gp_Pnt2d> pt = m_nodes.snap(screen_coords);
  if (!pt)
    // View plane and sketch plane must be perpendicular.
    return;

  if (m_tmp_node_idxs.size() != 2)
    return;

  gp_Pnt a = to_3d_(m_tmp_node_idxs[0]);
  gp_Pnt b = to_3d(m_pln, *pt);
  gp_Pnt c = to_3d_(m_tmp_node_idxs[1]);

  Handle(Geom_TrimmedCurve) arc_circle = GC_MakeArcOfCircle(a, b, c);
  if (!arc_circle)
  {
    m_view.remove(m_tmp_shp);
    m_tmp_shp = nullptr;
    return;
  }

  TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(arc_circle);
  show(m_ctx, m_tmp_shp, edge);
}

void Sketch::move_square_pt_(const ScreenCoords& screen_coords)
{
  move_line_string_pt_(screen_coords);
  auto l = [&](Edge& e, const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b)
  {
    TopoDS_Wire square = make_square_wire(m_pln, pt_a, *m_last_pt);
    show(m_ctx, m_tmp_shp, square);
  };
  last_edge_(l);
}

void Sketch::finalize_square_()
{
  auto l = [&](Edge& e, const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b)
  {
    std::array<gp_Pnt2d, 4> corners = square_corners(pt_a, pt_b);
    add_edge_(corners[0], corners[1]);
    add_edge_(corners[1], corners[2]);
    add_edge_(corners[2], corners[3]);
    add_edge_(corners[3], corners[0]);
    clear_tmps_();
    update_faces_();
  };
  last_edge_(l);
}

void Sketch::move_rectangle_pt_(const ScreenCoords& screen_coords)
{
  move_line_string_pt_(screen_coords);
  // DBG_MSG(m_tmp_edges.size());
  auto l = [&](Edge& e, const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b)
  {
    EZY_ASSERT(m_tmp_edges.size());

    if (std::abs(pt_a.X() - pt_b.X()) > Precision::Confusion() &&
        std::abs(pt_a.Y() - pt_b.Y()) > Precision::Confusion())
    {
      TopoDS_Wire rectangle = make_rectangle_wire(m_pln, pt_a, pt_b);
      show(m_ctx, m_tmp_shp, rectangle);
    }
    else
    {
      m_ctx.Remove(m_tmp_shp, true);
      m_tmp_shp.Nullify();
    }
  };
  last_edge_(l);
}

void Sketch::finalize_rectangle_()
{
  auto l = [&](Edge& e, const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b)
  {
    if (std::abs(pt_a.X() - pt_b.X()) > Precision::Confusion() &&
        std::abs(pt_a.Y() - pt_b.Y()) > Precision::Confusion())
    {
      std::array<gp_Pnt2d, 4> corners = rectangle_corners(pt_a, pt_b);
      add_edge_(corners[0], corners[1]);
      add_edge_(corners[1], corners[2]);
      add_edge_(corners[2], corners[3]);
      add_edge_(corners[3], corners[0]);
      clear_tmps_();
      update_faces_();
    }
    else
    {
      // Start a new edge
      EZY_ASSERT(e.node_idx_b.has_value());
      m_tmp_edges.push_back({*e.node_idx_b});
    }
  };
  last_edge_(l);
}

// Slot related
void Sketch::move_slot_pt_(const ScreenCoords& screen_coords)
{
  move_line_string_pt_(screen_coords);
  auto l = [&](Edge& e, const gp_Pnt2d& pt_b, const gp_Pnt2d& pt_c)
  {
    if (m_tmp_edges.size() != 2)
      return;

    const gp_Pnt2d& pt_a = m_nodes[m_tmp_edges.front().node_idx_a];
    if (unique(pt_a, pt_b, pt_c))
      show(m_ctx, m_tmp_shp, make_slot_wire(m_pln, pt_a, pt_b, pt_c));
  };
  last_edge_(l);
}

void Sketch::finalize_slot_()
{
  EZY_ASSERT(m_tmp_edges.size() == 2);
  EZY_ASSERT(m_tmp_edges[1].node_idx_b.has_value());

  Slot_pnts pnts = get_slot_points(
      m_nodes[m_tmp_edges[0].node_idx_a],
      m_nodes[m_tmp_edges[1].node_idx_a],
      m_nodes[m_tmp_edges[1].node_idx_b]);

  add_edge_(pnts.a_top_2d, pnts.b_top_2d);
  add_edge_(pnts.a_bottom_2d, pnts.b_bottom_2d);
  add_arc_circle_(pnts.a_bottom_2d, pnts.a_mid_2d, pnts.a_top_2d);
  add_arc_circle_(pnts.b_bottom_2d, pnts.b_mid_2d, pnts.b_top_2d);
  clear_tmps_();
  update_faces_();
}

// Operation axis related
void Sketch::add_operation_axis_pt_(const ScreenCoords& screen_coords)
{
  if (!has_operation_axis())
    add_line_string_pt_(screen_coords, Linestring_type::Single);
}

void Sketch::finalize_operation_axis_()
{
  m_operation_axis = std::move(m_tmp_edges.back());
  cancel_elm();  // Reset state, we have the operation axis
}

void Sketch::clear_operation_axis()
{
  if (m_operation_axis.has_value())
    m_ctx.Remove(m_operation_axis->shp, false);

  m_operation_axis = std::nullopt;
}

bool Sketch::has_operation_axis() const
{
  return m_operation_axis.has_value();
}

void Sketch::mirror_selected_edges()
{
  if (!m_operation_axis.has_value())
    // TODO report error!
    return;

  const std::vector<Edge> mirror_edges = get_selected_edges_();
  if (mirror_edges.empty())
  {
    // TODO present error.
  }

  EZY_ASSERT(!m_operation_axis->shp.IsNull());
  const auto [mirror_pt_a, mirror_pt_b] = get_edge_endpoints(m_pln, TopoDS::Edge(m_operation_axis->shp->Shape()));

  std::map<AIS_Shape_ptr, std::set<const Edge*>> arc_circles;

  for (const Edge& e : m_edges)
    for (const Edge& selected : mirror_edges)
      if (e.shp == selected.shp)
      {
        if (e.circle_arc)
        {
          std::set<const Edge*>& arc_circle_edges = arc_circles[e.shp];
          arc_circle_edges.insert(&e);
          if (arc_circle_edges.size() == 2)
            break;
        }
        else
        {
          const gp_Pnt2d a = mirror_point(mirror_pt_a, mirror_pt_b, m_nodes[e.node_idx_a]);
          const gp_Pnt2d b = mirror_point(mirror_pt_a, mirror_pt_b, m_nodes[e.node_idx_b]);
          add_edge_(a, b);
          break;
        }
      }

  for (auto& [_, arc_circle_edges] : arc_circles)
  {
    EZY_ASSERT(arc_circle_edges.size() == 2);
    const Edge* a;
    const Edge* b;
    for (const Edge* e : arc_circle_edges)
      if (e->node_idx_arc.has_value())
        a = e;
      else
        b = e;

    EZY_ASSERT(a && b);
    EZY_ASSERT(a->node_idx_arc.has_value());
    EZY_ASSERT(b->node_idx_b);
    const gp_Pnt2d pt_a = mirror_point(mirror_pt_a, mirror_pt_b, m_nodes[a->node_idx_a]);
    const gp_Pnt2d pt_b = mirror_point(mirror_pt_a, mirror_pt_b, m_nodes[a->node_idx_arc]);
    const gp_Pnt2d pt_c = mirror_point(mirror_pt_a, mirror_pt_b, m_nodes[b->node_idx_b]);
    add_arc_circle_(pt_a, pt_b, pt_c);
  }

  m_nodes.hide_snap_annos();
  update_faces_();
}

RevolvedShp_rslt Sketch::revolve_selected(const double angle)
{
  EZY_ASSERT_MSG(m_operation_axis.has_value(),
                 "No defined operation axis.");

  TopoDS_Compound compound;
  BRep_Builder    builder;
  builder.MakeCompound(compound);

  const std::vector<Sketch_face_shp_ptr> faces          = get_selected_faces_();
  const std::vector<Edge>                selected_edges = get_selected_edges_();
  if (selected_edges.size())
  {
    std::set<AIS_Shape*> seen;
    for (const Edge& e : selected_edges)
      // Arc circles share the same shape
      if (!seen.count(e.shp.get()))
      {
        seen.insert(e.shp.get());
        builder.Add(compound, e.shp->Shape());
      }
  }
  else if (faces.size())
    for (const Sketch_face_shp_ptr& face : faces)
      builder.Add(compound, face->Shape());

  else
    return RevolvedShp_rslt(Result_status::User_error, "No selected faces or edges.");

  try
  {
    const auto [pt_a, pt_b] = get_edge_endpoints(TopoDS::Edge(m_operation_axis->shp->Shape()));

    const gp_Dir direction((pt_b.XYZ() - pt_a.XYZ()).Normalized());
    const gp_Ax1 axis(pt_a, direction);

    BRepPrimAPI_MakeRevol revolMaker(compound, axis, angle);
    return RevolvedShp_rslt(new RevolvedShp(m_ctx, revolMaker.Shape()));
  }
  catch (const Standard_Failure& e)
  {
    // Catch OCCT exception and return error with message
    std::string error_msg = "Revolution failed: ";
    error_msg += e.GetMessageString() ? e.GetMessageString() : "Unknown OCCT error";
    return RevolvedShp_rslt(Result_status::Topo_error, error_msg);
  }
  catch (const std::exception& e)
  {
    // Catch other unexpected errors
    return RevolvedShp_rslt(Result_status::User_error, "Unexpected error: " + std::string(e.what()));
  }
}

// General sketch point related
template <typename Callback>
void Sketch::add_sketch_pt_(const ScreenCoords& screen_coords, size_t required_num_pts, Callback&& callback)
{
  auto l = [&](const std::optional<size_t>& node_idx, const gp_Pnt2d& pt)
  {
    if (node_idx)
      m_tmp_node_idxs.push_back(*node_idx);
    else
      m_tmp_node_idxs.push_back(m_nodes.add_new_node(pt));

    if (m_tmp_node_idxs.size() >= required_num_pts)
      callback(m_tmp_node_idxs.back());
  };
  move_sketch_pt_(screen_coords, l);
}

template <typename Callback>
void Sketch::move_sketch_pt_(const ScreenCoords& screen_coords, Callback&& callback)
{
  m_last_pt = m_view.pt_on_plane(screen_coords, m_pln);
  if (!m_last_pt)
    // View plane and sketch plane must be perpendicular.
    return;

  std::optional<size_t> node_idx = m_nodes.try_get_node_idx_snap(*m_last_pt);

  callback(node_idx, *m_last_pt);
}

template <typename Callback>
void Sketch::last_edge_(Callback&& callback)
{
  if (m_tmp_edges.empty())
    return;

  Edge&           e    = m_tmp_edges.back();
  const gp_Pnt2d& pt_a = m_nodes[e.node_idx_a];
  if (m_last_pt.has_value())
    if (unique(pt_a, *m_last_pt))
      callback(e, pt_a, *m_last_pt);
}

void Sketch::check_dimension_seg_(Linestring_type linestring_type)
{
  if (!m_entered_edge_len.has_value())
    return;

  if (m_entered_edge_len->len <= Precision::Confusion())
    return;

  EZY_ASSERT(m_tmp_edges.size());

  Edge&           edge = m_tmp_edges.back();
  const gp_Pnt2d& pt_a = m_nodes[edge.node_idx_a];
  m_last_pt            = gp_Pnt2d(pt_a).Translated(gp_Vec2d(m_entered_edge_len->dir) * m_entered_edge_len->len);

  update_edge_end_pt_(edge, m_nodes.get_node_exact(*m_last_pt));

  EZY_ASSERT(edge.node_idx_b.has_value());

  clear_all(m_entered_edge_len);

  if (linestring_type == Linestring_type::Single)
    finalize_elm();

  else if (linestring_type == Linestring_type::Two)
  {
    switch (m_tmp_edges.size())
    {
      case 1:
        m_tmp_edges.push_back({*edge.node_idx_b});
        break;
      case 2:
        finalize_elm();
        break;
      default:
        EZY_ASSERT(false);
    }
  }
  else
    m_tmp_edges.push_back({*edge.node_idx_b});
}

void Sketch::add_edge_(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, bool add_dim_anno)
{
  Edge edge {m_nodes.get_node_exact(pt_a)};
  update_edge_end_pt_(edge, m_nodes.get_node_exact(pt_b));
  set_edge_dim_anno_visible_(edge, add_dim_anno);
  m_edges.emplace_back(edge);
  m_nodes.finalize();
}

std::vector<Sketch::Edge> Sketch::get_selected_edges_() const
{
  std::vector<Edge> ret;
  for (const AIS_Shape_ptr& selected : m_view.get_selected())
    for (const Edge& e : m_edges)
      if (e.shp == selected)
        ret.push_back(e);

  return ret;
}

std::vector<Sketch_face_shp_ptr> Sketch::get_selected_faces_() const
{
  std::vector<Sketch_face_shp_ptr> ret;
  for (const AIS_Shape_ptr& selected : m_view.get_selected())
    for (const Sketch_face_shp_ptr& face : m_faces)
      if (face == selected)
        ret.push_back(face);

  return ret;
}

std::list<Sketch::Edge>::iterator Sketch::get_edge_at_(const ScreenCoords& screen_coords)
{
  AIS_Shape_ptr shp = m_view.get_shape(screen_coords);
  if (auto edge = dynamic_cast<Sketch_AIS_edge*>(shp.get()); edge)
    if (&edge->owner_sketch == this)
      for (std::list<Edge>::iterator itr = m_edges.begin(); itr != m_edges.end(); ++itr)
        if (itr->shp.get() == edge)
          return itr;

  return m_edges.end();
}

void Sketch::set_edge_dim_anno_visible_(Edge& e, bool visible)
{
  if (visible)
  {
    EZY_ASSERT(e.node_idx_b);
    if (e.dim)
      m_ctx.Remove(e.dim, false);  // Remove existing to update position

    e.dim       = create_distance_annotation(m_nodes[e.node_idx_a], m_nodes[e.node_idx_b], m_pln);
    double dist = m_nodes[e.node_idx_a].Distance(m_nodes[e.node_idx_b]);
    e.dim->SetCustomValue(dist / m_view.get_dimension_scale());
    m_ctx.Display(e.dim, true);
  }
  else
  {
    m_ctx.Remove(e.dim, true);
    e.dim.Nullify();
  }
}

void Sketch::toggle_edge_dim(const ScreenCoords& screen_coords)
{
  if (std::list<Edge>::iterator itr = get_edge_at_(screen_coords); itr != m_edges.end())
    set_edge_dim_anno_visible_(*itr, itr->dim.IsNull());
}

void Sketch::finalize_elm()
{
  m_show_dim_input = false;
  m_ctx.Remove(m_tmp_dim_anno, true);

  switch (get_mode())
  {
      // clang-format off
    case Mode::Sketch_add_edge:
    case Mode::Sketch_add_multi_edges:  finalize_edges_();          break;
    case Mode::Sketch_add_square:       finalize_square_();         break;
    case Mode::Sketch_add_rectangle:    finalize_rectangle_();      break;
    case Mode::Sketch_add_circle:       finalize_circle_();         break;
    case Mode::Sketch_add_slot:         finalize_slot_();           break;
    case Mode::Sketch_operation_axis:   finalize_operation_axis_(); break;
      // clang-format on
    default:
      EZY_ASSERT(false);
  }
}

// Cancel related
bool Sketch::cancel_elm()
{
  bool operation_canceled = clear_tmps_();
  m_ctx.Remove(m_tmp_dim_anno, true);
  m_nodes.hide_snap_annos();
  m_nodes.cancel();

  return operation_canceled;
}

// Function to extract faces from the planar graph
void Sketch::update_faces_()
{
  m_nodes.finalize();

  m_view.remove(m_faces);
  m_faces.clear();

  // Used to cleanup dangling nodes;
  std::vector<size_t> used_nodes(m_nodes.size());

  // Build adjacency list
  std::unordered_map<size_t, std::vector<std::pair<size_t, const Edge*>>> adj_list;

  for (const auto& edge : m_edges)
  {
    EZY_ASSERT(edge.node_idx_b.has_value());

    size_t a = edge.node_idx_a;
    size_t b = edge.node_idx_b.value();
    adj_list[a].push_back({b, &edge});
    adj_list[b].push_back({a, &edge});

    // Keep track of used nodes.
    used_nodes[a] = true;
    used_nodes[b] = true;
    if (edge.node_idx_mid.has_value())
      used_nodes[*edge.node_idx_mid] = true;

    if (edge.node_idx_arc.has_value())
      used_nodes[*edge.node_idx_arc] = true;
  }

  std::vector<Face> faces;

  std::unordered_set<std::pair<size_t, size_t>, Pair_hash> seen_edges;

  for (auto& [a_idx, edges] : adj_list)
    for (auto& [b_idx, start_edge] : edges)
    {
      if (seen_edges.count(std::make_pair(a_idx, b_idx)))
        continue;

      size_t      start_idx = a_idx;
      size_t      prev_idx  = a_idx;
      size_t      curr_idx  = b_idx;
      const Edge* curr_edge = start_edge;
      Face_edges  face;

      for (;;)
      {
        face.push_back({*curr_edge, curr_edge->reversed(prev_idx, curr_idx)});

        // Base case
        if (curr_idx == start_idx)
        {
          EZY_ASSERT(face.size() > 2);
          auto is_clock_wise = [&]()
          {
            // Compute signed area using the shoelace formula
            double signed_area = 0.0;
            for (const Face_edge& e : face)
            {
              // `start_nd_idx` and `end_nd_idx` consider reversed edges
              const gp_Pnt2d& p1 = m_nodes[e.start_nd_idx()];
              const gp_Pnt2d& p2 = m_nodes[e.end_nd_idx()];
              signed_area += (p1.X() * p2.Y()) - (p2.X() * p1.Y());
            }
            // signed_area *= 0.5;  // Divide by 2 for actual area

            return signed_area > 0;
          };
          if (!is_clock_wise())
            break;

          for (const Face_edge& e : face)
            seen_edges.insert(std::make_pair(e.start_nd_idx(), e.end_nd_idx()));

          auto f = create_face_shape_(face);
          f->SetColor(Quantity_NOC_GRAY7);        // Base color
          f->SetTransparency(0.5);                // 0.5 = 50% transparent (0.0 = opaque, 1.0 = invisible)
          f->SetMaterial(Graphic3d_NOM_PLASTIC);  // Optional: material for shading

          m_ctx.Display(f, AIS_Shaded, AIS_Shape::SelectionMode(TopAbs_FACE), true);
          m_ctx.Activate(f, AIS_Shape::SelectionMode(TopAbs_FACE));
          // m_ctx.UpdateCurrentViewer();
          m_faces.push_back(f);
          break;
        }

        size_t      left_most_idx;
        double      min_angle      = std::numeric_limits<double>::max();
        const Edge* left_most_edge = nullptr;
        gp_Vec2d    edge_a         = edge_outgoing_dir_(prev_idx, curr_idx, *curr_edge);

        for (auto& [next_idx, next_edge] : adj_list[curr_idx])
        {
          if (next_idx == prev_idx)
            continue;

          gp_Vec2d edge_b = edge_outgoing_dir_(curr_idx, next_idx, *next_edge);
          double   angle  = std::atan2(edge_b.Crossed(edge_a), edge_b.Dot(edge_a));
          if (angle < min_angle)
          {
            min_angle      = angle;
            left_most_idx  = next_idx;
            left_most_edge = next_edge;
          }
        }
        if (!left_most_edge)
          // Invalid topology, probably a dangling edge
          break;

        prev_idx  = curr_idx;
        curr_idx  = left_most_idx;
        curr_edge = left_most_edge;
      }
    }

  // Update used nodes
  for (size_t idx = 0, num = m_nodes.size(); idx < num; ++idx)
    m_nodes[idx].deleted = !used_nodes[idx];

  // Book keeping
  struct Face_meta
  {
    Sketch_face_shp_ptr           shp;
    double                        area;
    int                           parent_idx;
    std::vector<const Face_meta*> holes;
  };
  std::vector<Face_meta> face_metas;
  face_metas.reserve(m_faces.size());
  for (Sketch_face_shp_ptr& face : m_faces)
    face_metas.push_back({face, compute_face_area(face), -1});

  // Sort faces by area (descending) to check larger faces first
  std::sort(face_metas.begin(), face_metas.end(),
            [](const Face_meta& a, const Face_meta& b)
            {
              return a.area > b.area;  // Descending by area
            });

  // Classify faces
  for (size_t face_i = 0, num = face_metas.size(); face_i < num; ++face_i)
  {
    Face_meta& face_a = face_metas[face_i];
    for (size_t face_j = face_i + 1; face_j < num; ++face_j)
    {
      Face_meta& face_b = face_metas[face_j];
      if (is_face_contained(face_b.shp->Shape(), face_a.shp->Shape()))
        // Check if face_a is better (smaller) parent than the current one
        if (face_b.parent_idx == -1 || face_a.area < face_metas[face_a.parent_idx].area)
          face_b.parent_idx = static_cast<int>(face_i);
    }
  }

  // Assign holes
  for (const Face_meta& face : face_metas)
    if (face.parent_idx != -1)
      face_metas[face.parent_idx].holes.push_back(&face);

  // Rebuild face shapes with their holes
  for (Face_meta& face : face_metas)
    if (face.holes.size())
    {
      EZY_ASSERT(face.shp->Shape().ShapeType() == TopAbs_FACE);
      BRepBuilderAPI_MakeFace face_maker(TopoDS::Face(face.shp->Shape()));
      for (const Face_meta* hole : face.holes)
      {
        EZY_ASSERT(hole->shp->Shape().ShapeType() == TopAbs_FACE);
        TopoDS_Wire hole_wire = BRepTools::OuterWire(TopoDS::Face(hole->shp->Shape()));
        // Winding order is important
        hole_wire.Reverse();
        face_maker.Add(hole_wire);
      }
      EZY_ASSERT(face_maker.IsDone());
      // auto dbg_face = to_boost(face_maker.Face(), m_pln);
      face.shp->SetShape(face_maker.Face());
    }

  // Use the nesting depth of the faces to define the face selection sensitivity
  for (const Face_meta& face : face_metas)
  {
    int              nesting_depth = 0;
    const Face_meta* curr_face     = &face;
    for (; curr_face->parent_idx != -1; ++nesting_depth)
      curr_face = &face_metas[curr_face->parent_idx];

    m_ctx.SetSelectionSensitivity(face.shp, 0, nesting_depth + 1);
  }
}

size_t Sketch::Face_edge::start_nd_idx() const
{
  if (!reversed)
    return edge.node_idx_a;

  EZY_ASSERT(edge.node_idx_b);
  return *edge.node_idx_b;
}

size_t Sketch::Face_edge::end_nd_idx() const
{
  if (reversed)
    return edge.node_idx_a;

  EZY_ASSERT(edge.node_idx_b);
  return *edge.node_idx_b;
}

Sketch_face_shp_ptr Sketch::create_face_shape_(const Face_edges& face)
{
  EZY_ASSERT(face.size() > 2);

  // Create edges for the wire
  BRepBuilderAPI_MakeWire wire_maker;

  struct Circle_arc
  {
    std::optional<size_t> nd_idx_a;
    std::optional<size_t> nd_idx_b;
    std::optional<size_t> nd_idx_c;
    std::optional<bool>   dbg_reversed;
  };

  std::map<Geom_TrimmedCurve*, Circle_arc> circle_arcs;
  std::vector<gp_Pnt>                      node_verts;
  std::optional<size_t>                    dbg_last_node_idx;

  for (const Face_edge& e : face)
  {
    EZY_ASSERT(e.edge.node_idx_b);

    auto add_node_vert_unique = [&](const gp_Pnt& pt)
    {
      if (node_verts.empty())
        node_verts.push_back(pt);
      else if (!node_verts.back().IsEqual(pt, Precision::Confusion()))
        node_verts.push_back(pt);
    };

    if (e.edge.circle_arc.get())
    {
      Circle_arc& arc = circle_arcs[e.edge.circle_arc.get()];

      auto try_arc_circle = [&](bool reversed)
      {
        if (!arc.dbg_reversed.has_value())
          arc.dbg_reversed = reversed;
        else
          EZY_ASSERT(*arc.dbg_reversed == reversed);

        if (arc.nd_idx_a && arc.nd_idx_b && arc.nd_idx_c)
        {
          gp_Pnt a = to_3d_(arc.nd_idx_a);
          gp_Pnt b = to_3d_(arc.nd_idx_b);
          gp_Pnt c = to_3d_(arc.nd_idx_c);

          add_node_vert_unique(a);
          add_node_vert_unique(b);
          add_node_vert_unique(c);

          Geom_TrimmedCurve_ptr arc_circle;
          if (reversed)
            arc_circle = GC_MakeArcOfCircle(c, b, a);
          else
            arc_circle = GC_MakeArcOfCircle(a, b, c);

          BRepBuilderAPI_MakeEdge edge(arc_circle);
          EZY_ASSERT(edge.IsDone());
          wire_maker.Add(edge.Edge());
        }
      };

      // Depending on the order of the node indexes in face,
      // the first or second circle arc edge might come first.
      if (e.edge.node_idx_arc)
      {
        // This is the first part of the circle arc.
        EZY_ASSERT(e.edge.node_idx_arc);
        EZY_ASSERT(*e.edge.node_idx_b == *e.edge.node_idx_arc);
        arc.nd_idx_a = e.edge.node_idx_a;
        arc.nd_idx_b = e.edge.node_idx_arc;
        try_arc_circle(e.reversed);
      }
      else
      {
        // This is the second part of the circle arc.
        arc.nd_idx_c = *e.edge.node_idx_b;
        try_arc_circle(e.reversed);
      }
    }
    else
    {
      gp_Pnt a = to_3d_(e.start_nd_idx());
      gp_Pnt b = to_3d_(e.end_nd_idx());

      add_node_vert_unique(a);
      add_node_vert_unique(b);

      BRepBuilderAPI_MakeEdge edge(a, b);
      EZY_ASSERT(edge.IsDone());
      wire_maker.Add(edge.Edge());
    }
  }

  EZY_ASSERT(wire_maker.IsDone());
  TopoDS_Wire wire = wire_maker.Wire();

  // Create a face from the wire
  BRepBuilderAPI_MakeFace face_maker(wire);
  EZY_ASSERT(face_maker.IsDone());

  // auto dbg_face = to_boost(face_maker.Face(), m_pln);

  Sketch_face_shp* ret = new Sketch_face_shp(*this, face_maker.Face());
  std::swap(node_verts, ret->verts_3d);
  return ret;
}

void Sketch::update_edge_end_pt_(Edge& edge, size_t end_pt_idx)
{
  EZY_ASSERT(end_pt_idx < m_nodes.size());
  edge.node_idx_b      = end_pt_idx;
  const gp_Pnt2d& pt_a = m_nodes[edge.node_idx_a];
  const gp_Pnt2d& pt_b = m_nodes[end_pt_idx];
  update_edge_shp_(edge, pt_a, pt_b);
  edge.node_idx_mid = m_nodes.add_new_node(get_midpoint(pt_a, pt_b), true);
}

void Sketch::add_arc_circle_(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, const gp_Pnt2d& pt_c)
{
  std::vector<size_t> node_idxs {
      m_nodes.get_node_exact(pt_a),
      m_nodes.get_node_exact(pt_c),
      m_nodes.get_node_exact(pt_b)};

  add_arc_circle_(node_idxs);
}

void Sketch::add_arc_circle_(const std::vector<size_t>& node_idxs)
{
  EZY_ASSERT(node_idxs.size() == 3);
  Geom_TrimmedCurve_ptr arc_of_circle = GC_MakeArcOfCircle(
      to_3d_(node_idxs[0]),
      to_3d_(node_idxs[2]),
      to_3d_(node_idxs[1]));

  /*
  auto l = get_start_end_tangents(anArcOfCircle);

  l.first *= 10.0;
  l.second *= 10.0;

  {
    TopoDS_Shape edge_shape = BRepBuilderAPI_MakeEdge(anArcOfCircle->StartPoint(), anArcOfCircle->StartPoint().XYZ() + l.first.XYZ()).Edge();
    AIS_Shape_ptr s     = new AIS_Shape(edge_shape);
    s->SetWidth(2.0);
    s->SetColor(Quantity_Color(1, 0.0, 0.7, Quantity_TOC_RGB));
    m_view.display(s);
  }
  TopoDS_Shape edge_shape = BRepBuilderAPI_MakeEdge(anArcOfCircle->EndPoint(), anArcOfCircle->EndPoint().XYZ() + l.second.XYZ()).Edge();
  AIS_Shape_ptr s     = new AIS_Shape(edge_shape);
  s->SetWidth(2.0);
  s->SetColor(Quantity_Color(1, 0.0, 0.7, Quantity_TOC_RGB));
  m_view.display(s);
  */

  Sketch_AIS_edge_ptr shp = new Sketch_AIS_edge(*this, BRepBuilderAPI_MakeEdge(arc_of_circle));
  update_edge_style_(shp);
  m_ctx.Display(shp, false);

  // Split into two edges for valid planer graph topology.
  m_edges.push_back({node_idxs[0], node_idxs[2], node_idxs[2], std::nullopt, arc_of_circle, shp});
  m_edges.push_back({node_idxs[2], node_idxs[1], std::nullopt, std::nullopt, arc_of_circle, shp});
}

// Circle related
void Sketch::move_circle_pt_(const ScreenCoords& screen_coords)
{
  move_line_string_pt_(screen_coords);
  auto l = [&](Edge& e, const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b)
  {
    TopoDS_Wire circle = make_circle_wire(m_pln, pt_a, *m_last_pt);
    show(m_ctx, m_tmp_shp, circle);
  };
  last_edge_(l);
}

void Sketch::finalize_circle_()
{
  auto l = [&](Edge& e, const gp_Pnt2d& center, const gp_Pnt2d& edge_pt)
  {
    std::array<gp_Pnt2d, 4> points = xy_stencil_pnts(center, edge_pt);
    add_arc_circle_(points[0], points[2], points[1]);
    add_arc_circle_(points[0], points[3], points[1]);
    clear_tmps_();
    update_faces_();
  };
  last_edge_(l);
}

bool Sketch::clear_tmps_()
{
  m_view.remove(m_tmp_shp);
  for (Edge& e : m_tmp_edges)
    m_view.remove(e.shp);

  if (m_tmp_shp)
  {
    m_ctx.Remove(m_tmp_shp, true);
    m_tmp_shp = nullptr;
  }

  bool operation_canceled = m_tmp_edges.size();
  clear_all(m_tmp_node_idxs, m_tmp_shp, m_tmp_edges, m_entered_edge_len, m_show_dim_input);

  return operation_canceled;
}

void Sketch::on_mode()
{
  // Reset state
  cancel_elm();
}

Mode Sketch::get_mode() const
{
  return m_view.get_mode();
}

const gp_Pln& Sketch::get_plane() const
{
  return m_pln;
}

Sketch_nodes& Sketch::get_nodes()
{
  return m_nodes;
}

void Sketch::get_snap_pts_3d_(std::vector<gp_Pnt>& out)
{
  out.clear();
  get_originating_face_snp_pts_3d_(out);
  m_nodes.get_snap_pts_3d(out);
}

void Sketch::get_originating_face_snp_pts_3d_(std::vector<gp_Pnt>& out)
{
  if (!m_originating_face)
    return;

  std::vector<gp_Pnt> snp_pts;
  const TopoDS_Shape& shape = m_originating_face->Shape();
  EZY_ASSERT(shape.ShapeType() == TopAbs_WIRE);
  const TopoDS_Wire& wire = TopoDS::Wire(shape);
  TopExp_Explorer    edgeExplorer(wire, TopAbs_EDGE);
  while (edgeExplorer.More())
  {
    const TopoDS_Edge& edge = TopoDS::Edge(edgeExplorer.Current());

    if (BRep_Tool::IsClosed(edge))
    {
      int hi = 0;
    }

    double         first, last;
    Geom_Curve_ptr curve = BRep_Tool::Curve(edge, first, last);
    EZY_ASSERT(curve);

    TopoDS_Vertex v1, v2;
    TopExp::Vertices(edge, v1, v2);
    gp_Pnt p1 = BRep_Tool::Pnt(v1);
    gp_Pnt pc = curve->Value((first + last) * 0.5);
    gp_Pnt p2 = BRep_Tool::Pnt(v2);

    if (snp_pts.empty() || !snp_pts.back().IsEqual(p1, Precision::Confusion()))
      snp_pts.push_back(p1);

    snp_pts.push_back(pc);

    if (!snp_pts.front().IsEqual(p2, Precision::Confusion()))
      snp_pts.push_back(p2);

    edgeExplorer.Next();
  }

  append(out, snp_pts);
}

void Sketch::set_visible(bool state)
{
  m_visible = state;

  if (state)
  {
    if (m_show_faces)
      for (AIS_Shape_ptr& face : m_faces)
        m_ctx.Display(face, AIS_Shaded, 0, false);

    for (Edge& e : m_edges)
    {
      m_ctx.Display(e.shp, AIS_WireFrame, 0, false);
      if (e.dim)
        m_ctx.Display(e.dim, false);
    }

    if (m_originating_face)
      m_ctx.Display(m_originating_face, AIS_WireFrame, 0, false);
  }
  else
  {
    for (AIS_Shape_ptr& face : m_faces)
      m_ctx.Erase(face, false);

    for (Edge& e : m_edges)
    {
      m_ctx.Erase(e.shp, false);
      if (e.dim)
        m_ctx.Erase(e.dim, false);
    }

    if (m_originating_face)
      m_ctx.Erase(m_originating_face, false);

    m_nodes.hide_snap_annos();
    cancel_elm();
  }

  m_ctx.UpdateCurrentViewer();

  // Call to update outside sketch snap points.
  m_view.curr_sketch().set_current();
}

bool Sketch::is_visible() const
{
  return m_visible;
}

void Sketch::set_show_faces(bool show)
{
  if (show && m_visible)
    for (AIS_Shape_ptr& face : m_faces)
      m_ctx.Display(face, AIS_Shaded, 0, false);
  else
    for (AIS_Shape_ptr& face : m_faces)
      m_ctx.Erase(face, false);

  m_show_faces = show;
}

void Sketch::set_show_edges(bool show)
{
  if (show && m_visible)
    for (Edge& e : m_edges)
      m_ctx.Display(e.shp, AIS_WireFrame, 0, false);
  else
    for (Edge& e : m_edges)
      m_ctx.Erase(e.shp, false);
}

void Sketch::set_show_dims(bool show)
{
  if (show && m_visible)
  {
    for (Edge& e : m_edges)
      if (e.dim)
        m_ctx.Display(e.dim, false);
  }
  else
  {
    for (Edge& e : m_edges)
      if (e.dim)
        m_ctx.Erase(e.dim, false);
  }
}

bool Sketch::is_current() const
{
  return this == &m_view.curr_sketch();
}

void Sketch::set_current()
{
  m_ctx.CurrentViewer()->SetPrivilegedPlane(m_pln.Position());
  set_edge_style(Edge_style::Full);
  m_nodes.clear_outside_snap_pnts();

  m_tmp_pts_3d.clear();
  get_originating_face_snp_pts_3d_(m_tmp_pts_3d);

  for (const gp_Pnt& pt3d : m_tmp_pts_3d)
    // Insert 2D points on this `m_pln`, points will be considered the same using `Precision::Confusion()`
    m_nodes.add_outside_snap_pnt(pt3d);

  for (Sketch_ptr& sketch : m_view.get_sketches())
    if (sketch.get() != this)
    {
      sketch->set_edge_style(Edge_style::Background);

      if (sketch->is_visible())
      {
        // Add snap points from other sketches, but only if they are visible.
        sketch->get_snap_pts_3d_(m_tmp_pts_3d);
        for (const gp_Pnt& pt3d : m_tmp_pts_3d)
          // Insert 2D points on this `m_pln`, points will be considered the same using `Precision::Confusion()`
          m_nodes.add_outside_snap_pnt(pt3d);
      }
    }

  // DBG_MSG("m_outside_snap_pts " << m_nodes.m_outside_snap_pts.size());
}

void Sketch::set_edge_style(Edge_style style)
{
  m_edge_style = style;

  for (Edge& e : m_edges)
    update_edge_style_(e.shp);

  update_originating_face_style();
}

gp_Pnt Sketch::to_3d_(size_t node_idx) const
{
  return to_3d(m_pln, m_nodes[node_idx]);
}

gp_Pnt Sketch::to_3d_(const std::optional<size_t>& node_idx) const
{
  EZY_ASSERT(node_idx.has_value());
  return to_3d_(*node_idx);
}

const std::string& Sketch::get_name() const
{
  return m_name;
}

void Sketch::set_name(const std::string& name)
{
  m_name = name;
}

gp_Vec2d Sketch::edge_outgoing_dir_(size_t idx_a, size_t idx_b, const Edge& edge) const
{
#if 0
  boost_geom::linestring_2d ls;
  boost_geom::linestring_2d ls2;
  ls.push_back(to_boost(m_nodes[idx_a]));
  ls.push_back(to_boost(m_nodes[idx_b]));
  if (false && edge.circle_arc)
  {
    // TODO this will work for some cases and fail for others.
    // to_boost(*edge.circle_arc, m_pln);
    auto [od, p_end] = get_out_dir_and_end_pt(edge.circle_arc);
    auto p_end_2d    = to_2d(m_pln, {p_end.X(), p_end.Y(), p_end.Z()});

    auto e = m_nodes[idx_b];
    if (!p_end_2d.IsEqual(e, Precision::Confusion()))
    {
      std::tie(od, p_end) = get_out_dir_and_end_pt(Geom_TrimmedCurve_ptr::DownCast(edge.circle_arc->Reversed()));
      int hi = 0;
    }
    auto p = to_2d(m_pln, {od.X(), od.Y(), od.Z()});
    ls2.push_back(to_boost(m_nodes[idx_b]));
    auto pp = m_nodes[idx_b].XY() + p.XY();
    ls2.push_back({pp.X(), pp.Y()});
    return gp_Vec2d(p.X(), p.Y());
    int i = 0;
  }
#endif

  gp_Vec2d ret(m_nodes[idx_a], m_nodes[idx_b]);
  ret.Normalize();
  return ret;
}

bool Sketch::Edge::reversed(size_t idx_a, size_t idx_b) const
{
  if (node_idx_a == idx_a && node_idx_b == idx_b)
    return false;

  if (node_idx_a == idx_b && node_idx_b == idx_a)
    return true;

  EZY_ASSERT(false);
  return false;
}

Sketch_AIS_edge::Sketch_AIS_edge(Sketch& owner, const TopoDS_Shape& shp)
    : AIS_Shape(shp), owner_sketch(owner)
{
  int hi = 0;
}

Sketch_face_shp::Sketch_face_shp(Sketch& owner, const TopoDS_Shape& face)
    : owner_sketch(owner), AIS_Shape(face) {}