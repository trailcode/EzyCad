// Pure C++ implementation of the polygon/ring functionality (for OCCT face to
// 2D polygon conversion, shoelace-based clockwise/area checks, and basic WKT
// output for tests). No external geometry library dependency.
namespace ezy_geom
{
struct point_2d
{
  double x_ = 0.0, y_ = 0.0;
  double x() const { return x_; }
  double y() const { return y_; }
  point_2d() = default;
  point_2d(double xx, double yy)
      : x_(xx)
      , y_(yy)
  {
  }
};

using ring_2d = std::vector<point_2d>;

struct polygon_2d
{
  ring_2d                     outer_;
  std::vector<ring_2d>        inners_;
  ring_2d&                    outer() { return outer_; }
  const ring_2d&              outer() const { return outer_; }
  std::vector<ring_2d>&       inners() { return inners_; }
  const std::vector<ring_2d>& inners() const { return inners_; }
};

// Basic validity (the OCCT construction code ensures well-formed polygons).
bool is_valid(const polygon_2d& poly);

// Shoelace area (outer minus holes).
double area(const polygon_2d& poly);
} // namespace ezy_geom
