# Basic startup helpers for EzyCad Python console.

def dump_view():
    ezy.log(
        f"mode={ezy.get_mode()} sketches={ezy.view.sketch_count()} shapes={ezy.view.shape_count()}"
    )
    # Alias still works: view == ezy.view
    ezy.log(f"(alias) sketches={view.sketch_count()}")

def print_current_sketch_nodes():
    """Print all nodes (with their 2D coordinates) of the current sketch."""
    try:
        n = view.curr_sketch.node_count()
    except Exception as ex:
        ezy.log(f"print_current_sketch_nodes: {ex}")
        return
    ezy.log(f"current sketch nodes: {n}")
    for i in range(n):
        try:
            x, y = view.curr_sketch.node(i)
            ezy.log(f"  [{i}] ({x}, {y})")
        except Exception as ex:
            ezy.log(f"  [{i}] error: {ex}")

def print_current_sketch_dims():
    """Print all length dimensions (name, distance, node pair, visibility, offset) of the current sketch."""
    try:
        n = view.curr_sketch.dim_count()
    except Exception as ex:
        ezy.log(f"print_current_sketch_dims: {ex}")
        return
    ezy.log(f"current sketch dimensions: {n}")
    for i in range(n):
        try:
            lo, hi, visible, offset, name, dist = view.curr_sketch.dim(i)
            label = name if name else f"D{i}"
            ezy.log(f"  [{i}] {label} {dist:g}: nodes ({lo}, {hi}) visible={visible} offset={offset}")
        except Exception as ex:
            ezy.log(f"  [{i}] error: {ex}")
