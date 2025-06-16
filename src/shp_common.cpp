
#include "shp_common.h"
#include "utl.h"

#include <BRepAlgoAPI_Common.hxx>

Shp_common::Shp_common(Occt_view& view)
    : Shp_operation_base(view) {}

Status Shp_common::selected_common()
{
  CHK_RET(ensure_operation_multi_shps_());

  std::vector<ShapeBase_ptr>::iterator itr = m_shps.begin();

  // Start with the first shape
  TopoDS_Shape result = (*itr)->Shape();

  // Intersect with each subsequent shape
  for (++itr; itr != m_shps.end(); ++itr)
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
  delete_operation_shps_();
  add_shp_(shp);
  return Status::ok();
}