# Basic startup helpers for EzyCad Python console.

def dump_view():
    ezy.log(f"mode={ezy.get_mode()} sketches={view.sketch_count()} shapes={view.shape_count()}")

