#include "shp_common.h"

#include <BRepAlgoAPI_Common.hxx>

Shp_common::Shp_common(Occt_view& view)
    : Shp_operation_base(view) {}

Status Shp_common::selected_common()
{
  std::vector<AIS_Shape_ptr> selected = get_selected();
  if (selected.size() < 2)
    return Status::user_error("Select two or more shapes.");

  std::vector<AIS_Shape_ptr>::iterator itr = selected.begin();

  // Start with the first shape
  TopoDS_Shape result = (*itr)->Shape();

  // Intersect with each subsequent shape
  for (++itr; itr != selected.end(); ++itr)
  {
    BRepAlgoAPI_Common common_op(result, (*itr)->Shape());

    // Check if the operation was successful
    if (!common_op.IsDone())
      return Status::user_error("Error: Common operation failed");

    // Get the result of this intersection operation
    result = common_op.Shape();

    if (result.IsNull())
      return Status::user_error("Error: Resulting shape is null");
  }

  // Create a new shape from the common result
  ExtrudedShp_ptr shp = new ExtrudedShp(ctx(), result);
  shp->set_name("Common");
  delete_selected_();
  add_shp_(shp);
  return Status::ok();
}