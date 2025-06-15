#include "shp_fuse.h"
#include "gui.h"
#include "utl.h"

#include <BRepAlgoAPI_Fuse.hxx>

Shp_fuse::Shp_fuse(Occt_view& view)
    : Shp_operation_base(view) {}

Status Shp_fuse::selected_fuse()
{
  CHK_RET(ensure_operation_multi_shps_());
  
  std::vector<ShapeBase_ptr>::iterator itr = m_shps.begin();

  // Start with the first shape
  TopoDS_Shape result = (*itr)->Shape();

  // Union with each subsequent shape
  for (++itr; itr != m_shps.end(); ++itr)
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
  delete_operation_shps_();
  add_shp_(shp);
  return Status::ok();
}