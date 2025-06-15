#if 0
std::optional<gp_Pnt> Occt_view::get_selected_hit_point(const ScreenCoords& screen_coords) const
{
  Ray hit_ray                = get_hit_test_ray(screen_coords);
  Handle(Geom_Line) ray_line = new Geom_Line(hit_ray.origin, hit_ray.direction);
  for (AIS_Shape_ptr& shape : get_selected())
  {
    TopExp_Explorer explorer(shape->Shape(), TopAbs_FACE);
    while (explorer.More())
    {
      TopoDS_Face face             = TopoDS::Face(explorer.Current());
      Handle(Geom_Surface) surface = BRep_Tool::Surface(face);
      GeomAPI_IntCS intersection(ray_line, surface);
      if (intersection.NbPoints() > 0)
      {
        return intersection.Point(1);
      }
      explorer.Next();
    }
  }

  // No hit points found.
  return std::nullopt;
}

void Occt_view::errorCallback(int theError, const char* theDescription)
{
  Message::DefaultMessenger()->Send(
      TCollection_AsciiString("Error") + theError + ": " + theDescription,
      Message_Fail);
}

// Function to find the coordinates of the vertex closest to the camera
std::optional<gp_Pnt> find_closest_vertex_to_camera(const V3d_View_ptr& view, const AIS_Shape_ptr& shp)
{
  std::optional<gp_Pnt> closest_pnt  = std::nullopt;
  Standard_Real         min_distance = std::numeric_limits<Standard_Real>::max();

  // Get the camera's eye position (world coordinates)
  gp_Pnt camera_pos = view->Camera()->Eye();

  // Get the TopoDS_Shape from the AIS_Shape
  const TopoDS_Shape& shape = shp->Shape();

  // Get the transformation from the AIS_Shape
  const gp_Trsf& trsf = shp->Transformation();

  // Explore all vertices in the shape
  TopExp_Explorer explorer(shape, TopAbs_VERTEX);
  while (explorer.More())
  {
    // Get the vertex
    TopoDS_Vertex vertex = TopoDS::Vertex(explorer.Current());

    // Get the vertex's 3D point
    gp_Pnt vertex_pnt = BRep_Tool::Pnt(vertex);

    // Apply the AIS_Shape's transformation
    vertex_pnt.Transform(trsf);

    // Compute Euclidean distance to camera
    Standard_Real distance = vertex_pnt.Distance(camera_pos);

    // Update closest point if this distance is smaller
    if (distance < min_distance)
    {
      min_distance = distance;
      closest_pnt  = vertex_pnt;
    }

    explorer.Next();
  }

  return closest_pnt;
}

enum class Order
{
  Ascending,
  Descending
};

// Templated function to sort a container of AIS_Shape-derived handles by area
template <typename Container>
void sort_faces_by_area(Container& faces, Order order)
{
  // Extract the element type from the container
  using ElementType = typename Container::value_type;

  // Ensure ElementType is a Handle(T) where T derives from AIS_Shape
  static_assert(
      std::is_base_of_v<AIS_Shape, typename ElementType::element_type>,
      "Container elements must be handles to classes derived from AIS_Shape");

  // Sort the container using std::sort
  std::sort(faces.begin(), faces.end(),
            [order](const ElementType& shape_a, const ElementType& shape_b)
            {
              try
              {
                Standard_Real area_a = compute_face_area(shape_a);
                Standard_Real area_b = compute_face_area(shape_b);
                return order == Order::Ascending ? (area_a < area_b) : (area_a > area_b);
              }
              catch (const std::exception&)
              {
                // If area computation fails, treat as equal
                return false;
              }
            });
}

void get_wire_verts(AIS_Shape_ptr& shp, std::vector<gp_Pnt>& verts_out)
{
  verts_out.clear();
  const TopoDS_Shape& shape = shp->Shape();
  EZY_ASSERT(shape.ShapeType() == TopAbs_WIRE);
  const TopoDS_Wire& wire = TopoDS::Wire(shape);
  TopExp_Explorer    edgeExplorer(wire, TopAbs_EDGE);
  while (edgeExplorer.More())
  {
    const TopoDS_Edge& edge = TopoDS::Edge(edgeExplorer.Current());
    TopoDS_Vertex      v1, v2;
    TopExp::Vertices(edge, v1, v2);
    gp_Pnt p1 = BRep_Tool::Pnt(v1);
    gp_Pnt p2 = BRep_Tool::Pnt(v2);
    
    verts_out.push_back(p1);
    if (!v1.IsSame(v2))
      // Check if the edge isn't degenerate
      // TODO This should not happen, use EZY_ASSERT?
      verts_out.push_back(p2);

    edgeExplorer.Next();
  }
}

#endif