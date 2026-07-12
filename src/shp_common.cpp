#include "shp_common.h"

#include <BRepAlgoAPI_Common.hxx>

#include "gui_occt_view.h"
#include "utl.h"

Shp_common::Shp_common(Occt_view& view)
    : Shp_operation_base(view)
{
}

Shp_rslt Shp_common::common(std::vector<Shp_ptr> shps)
{
  if (shps.size() < 2)
    return Shp_rslt(Result_status::User_error, "common requires two or more shapes");

  for (const Shp_ptr& shp : shps)
    if (shp.IsNull())
      return Shp_rslt(Result_status::User_error, "common: null shape");

  view().push_undo_snapshot();
  m_shps = std::move(shps);

  std::vector<Shp_ptr>::iterator itr = m_shps.begin();

  // Start with the first shape
  TopoDS_Shape result = (*itr)->Shape();

  // Intersect with each subsequent shape
  for (++itr; itr != m_shps.end(); ++itr)
  {
    BRepAlgoAPI_Common common_op(result, (*itr)->Shape());

    // Check if the operation was successful
    if (!common_op.IsDone())
      return Shp_rslt(Result_status::User_error, "Error: Common operation failed");

    // Get the result of this intersection operation
    result = common_op.Shape();

    if (result.IsNull())
      return Shp_rslt(Result_status::User_error, "Error: Resulting shape is null");
  }

  // Create a new shape from the common result
  Shp_ptr shp = new Shp(ctx(), result);
  shp->set_name("Common");
  delete_operation_shps_();
  add_shp_(shp);
  return Shp_rslt(shp);
}

Status Shp_common::selected_common()
{
  CHK_RET(ensure_operation_multi_shps_());
  Shp_rslt r = common(std::move(m_shps));
  if (!r.is_ok())
    return Status(r.status(), r.message());
  return Status::ok();
}
