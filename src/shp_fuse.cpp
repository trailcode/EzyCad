#include "shp_fuse.h"
#include "gui.h"

#include <BRepAlgoAPI_Fuse.hxx>

Shp_fuse::Shp_fuse(Occt_view& view)
    : Shp_operation_base(view) {}

Status Shp_fuse::selected_fuse()
{
  std::vector<AIS_Shape_ptr> selected = get_selected();
  if (selected.size() < 2)
    return Status::user_error("Select two or more shapes.");
  
  std::vector<AIS_Shape_ptr>::iterator itr = selected.begin();

  // Start with the first shape
  TopoDS_Shape result = (*itr)->Shape();

  // Union with each subsequent shape
  for (++itr; itr != selected.end(); ++itr)
  {
    BRepAlgoAPI_Fuse fuse_op(result, (*itr)->Shape());

    // Check if the operation was successful
    if (!fuse_op.IsDone())
      return Status::user_error("Error: Union operation failed");

    // Get the result of this union operation
    result = fuse_op.Shape();

    if (result.IsNull())
      return Status::user_error("Error: Resulting shape is null");
  }

  // Create a new shape from the union result
  ExtrudedShp_ptr shp = new ExtrudedShp(ctx(), result);
  shp->set_name("Fused");
  delete_selected_();
  add_shp_(shp);
  return Status::ok();
}