#include "shp_fuse.h"

#include <BRepAlgoAPI_Fuse.hxx>

#include "gui_occt_view.h"
#include "shp_delta.h"
#include "utl.h"

Shp_fuse::Shp_fuse(Occt_view& view)
    : Shp_operation_base(view)
{
}

Shp_rslt Shp_fuse::fuse(std::vector<Shp_ptr> shps)
{
  if (shps.size() < 2)
    return Shp_rslt(Result_status::User_error, "fuse requires two or more shapes");

  for (const Shp_ptr& shp : shps)
    if (shp.IsNull())
      return Shp_rslt(Result_status::User_error, "fuse: null shape");

  m_shps = std::move(shps);

  std::vector<Shape_rec> removed;
  removed.reserve(m_shps.size());
  for (const Shp_ptr& shp : m_shps)
    removed.push_back(capture_shape_rec(*shp));

  std::vector<Shp_ptr>::iterator itr = m_shps.begin();

  // Start with the first shape
  TopoDS_Shape result = (*itr)->Shape();

  // Union with each subsequent shape
  for (++itr; itr != m_shps.end(); ++itr)
  {
    BRepAlgoAPI_Fuse fuse_op(result, (*itr)->Shape());

    // Check if the operation was successful
    if (!fuse_op.IsDone())
      return Shp_rslt(Result_status::User_error, "Error: Union operation failed");

    // Get the result of this union operation
    result = fuse_op.Shape();

    if (result.IsNull())
      return Shp_rslt(Result_status::User_error, "Error: Resulting shape is null");
  }

  // Create a new shape from the union result
  Shp_ptr shp = new Shp(ctx(), result);
  shp->set_name("Fused");
  assign_result_parent_(shp, m_shps);
  delete_operation_shps_();
  add_shp_(shp);
  view().push_undo_delta(std::make_unique<Shape_replace_delta>(std::move(removed), std::vector<Shape_rec>{capture_shape_rec(*shp)}));
  return Shp_rslt(shp);
}

Status Shp_fuse::selected_fuse()
{
  CHK_RET(ensure_operation_multi_shps_());
  Shp_rslt r = fuse(std::move(m_shps));
  if (!r.is_ok())
    return Status(r.status(), r.message());
  return Status::ok();
}
