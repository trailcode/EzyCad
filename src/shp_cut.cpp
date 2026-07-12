#include "shp_cut.h"

#include <BRepAlgoAPI_Cut.hxx>
#include <NCollection_List.hxx>

#include "gui_occt_view.h"
#include "utl.h"

Shp_cut::Shp_cut(Occt_view& view)
    : Shp_operation_base(view)
{
}

Shp_rslt Shp_cut::cut(std::vector<Shp_ptr> shps)
{
  if (shps.size() < 2)
    return Shp_rslt(Result_status::User_error, "cut requires two or more shapes");

  for (const Shp_ptr& shp : shps)
    if (shp.IsNull())
      return Shp_rslt(Result_status::User_error, "cut: null shape");

  view().push_undo_snapshot();
  m_shps = std::move(shps);

  std::vector<Shp_ptr>::iterator itr = m_shps.begin();

  NCollection_List<TopoDS_Shape> tool_list;
  NCollection_List<TopoDS_Shape> arguments;
  arguments.Append((*itr)->Shape());
  for (++itr; itr != m_shps.end(); ++itr)
    tool_list.Append((*itr)->Shape());

  BRepAlgoAPI_Cut cut_op;
  cut_op.SetArguments(arguments);
  cut_op.SetTools(tool_list); // Set tool shapes

  // Perform difference operation
  cut_op.Build();
  if (!cut_op.IsDone())
    return Shp_rslt(Result_status::User_error, "Error: Difference operation failed");

  const TopoDS_Shape& result_shape = cut_op.Shape();
  if (result_shape.IsNull())
    return Shp_rslt(Result_status::User_error, "Error: Resulting shape is null");

  Shp_ptr shp = new Shp(ctx(), result_shape);
  shp->set_name("Cut");
  delete_operation_shps_();
  add_shp_(shp);
  return Shp_rslt(shp);
}

Status Shp_cut::selected_cut()
{
  CHK_RET(ensure_operation_multi_shps_());
  Shp_rslt r = cut(std::move(m_shps));
  if (!r.is_ok())
    return Status(r.status(), r.message());
  return Status::ok();
}
