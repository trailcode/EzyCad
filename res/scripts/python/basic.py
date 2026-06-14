# Basic startup helpers for EzyCad Python console.

def dump_view():
    ezy.log(f"mode={ezy.get_mode()} sketches={view.sketch_count()} shapes={view.shape_count()}")

def print_current_sketch_nodes():
    """Print all nodes (with their 2D coordinates) of the current sketch."""
    try:
        n = view.curr_sketch_node_count()
    except Exception as ex:
        ezy.log(f"print_current_sketch_nodes: {ex}")
        return
    ezy.log(f"current sketch nodes: {n}")
    for i in range(n):
        try:
            x, y = view.curr_sketch_node(i)
            ezy.log(f"  [{i}] ({x}, {y})")
        except Exception as ex:
            ezy.log(f"  [{i}] error: {ex}")
