# Sierpinski triangle sketch generator for EzyCad Python console.
# Just for fun!
#
# Usage (after opening Python console, or it runs on load):
#   create_sierpinski(order=3, size=20.0)           # log nodes and segments
#   create_sierpinski_sketch(order=3, size=20.0)    # build as a new sketch
#
# The recursion draws the standard Sierpinski gasket lines (connect midpoints
# recursively, resulting in 3**order small upward triangles' boundaries).

def _mid(a, b):
    return ((a[0] + b[0]) / 2.0, (a[1] + b[1]) / 2.0)

def _sierpinski_lines(order, p1, p2, p3, lines):
    """Recursively append line segments (as (pA, pB) tuples) for the gasket."""
    if order <= 0:
        lines.append((p1, p2))
        lines.append((p2, p3))
        lines.append((p3, p1))
        return
    m12 = _mid(p1, p2)
    m23 = _mid(p2, p3)
    m31 = _mid(p3, p1)
    _sierpinski_lines(order - 1, p1, m12, m31, lines)
    _sierpinski_lines(order - 1, p2, m23, m12, lines)
    _sierpinski_lines(order - 1, p3, m31, m23, lines)

def create_sierpinski(order=3, size=20.0, center=(0.0, 0.0)):
    """Generate and log a Sierpinski triangle of given fractal order and size.
    Logs all unique nodes (sorted) and all line segments.
    The triangle is equilateral pointing up, centered at the given (cx, cy).
    """
    cx, cy = center
    h = size * (3.0 ** 0.5) / 2.0
    # Base centered at (cx, cy - h/3) so centroid is at center
    p1 = (cx - size / 2.0, cy - h / 3.0)
    p2 = (cx + size / 2.0, cy - h / 3.0)
    p3 = (cx, cy + 2.0 * h / 3.0)

    lines = []
    _sierpinski_lines(order, p1, p2, p3, lines)

    # Dedup lines (though recursion shouldn't duplicate)
    seen = set()
    unique_lines = []
    for a, b in lines:
        key = (tuple(sorted([a, b])))
        if key not in seen:
            seen.add(key)
            unique_lines.append((a, b))

    # Collect unique nodes
    nodes = {}
    for a, b in unique_lines:
        nodes[a] = True
        nodes[b] = True
    node_list = sorted(nodes.keys(), key=lambda p: (p[1], p[0]))  # bottom to top, left to right

    ezy.log(f"Sierpinski triangle: order={order}, size={size}, center={center}")
    ezy.log(f"  unique nodes: {len(node_list)}")
    for i, (x, y) in enumerate(node_list):
        ezy.log(f"    [{i}] ({x:.6f}, {y:.6f})")

    ezy.log(f"  line segments: {len(unique_lines)}")
    for i, (a, b) in enumerate(unique_lines):
        ezy.log(f"    [{i}] ({a[0]:.6f},{a[1]:.6f}) -> ({b[0]:.6f},{b[1]:.6f})")

    ezy.log("To build the sketch: create_sierpinski_sketch(order, size, center)")

    # For convenience, also stash the data on the main module so you can access from console
    import __main__
    __main__.sierpinski_nodes = node_list
    __main__.sierpinski_lines = unique_lines

    return node_list, unique_lines

def create_sierpinski_sketch(order=3, size=20.0, center=(0.0, 0.0), plane="XY", offset=0.0, name="Sierpinski"):
    """Generate a Sierpinski triangle and add it as a new sketch."""
    node_list, unique_lines = create_sierpinski(order, size, center)
    ezy.sketch.add(plane, offset, name)
    for (a, b) in unique_lines:
        ezy.sketch.add_edge(a[0], a[1], b[0], b[1])
    ezy.sketch.finish_edges()
    ezy.log(f"Created sketch '{ezy.sketch.curr_name()}' with {len(unique_lines)} edges.")
    return node_list, unique_lines

# Auto-generate a small example on script load (so it's immediately visible in the console tab)
# You can call create_sierpinski(4) yourself for higher detail (gets dense quickly).
if __name__ == "__main__" or True:  # always run on exec
    try:
        create_sierpinski(order=2, size=10.0)  # small default so it doesn't spam
    except Exception as ex:
        try:
            ezy.log(f"sierpinski auto-gen error: {ex}")
        except:
            pass
