#include "shp_cut.h"
#include "utl.h"

#include <TopTools_ListOfShape.hxx>
#include <BRepAlgoAPI_Cut.hxx>

Shp_cut::Shp_cut(Occt_view& view)
    : Shp_operation_base(view) {}

Status Shp_cut::selected_cut()
{
  CHK_RET(ensure_operation_multi_shps_());
  
  std::vector<ShapeBase_ptr>::iterator itr = m_shps.begin();

  TopTools_ListOfShape tool_list;
  TopTools_ListOfShape arguments;
  arguments.Append((*itr)->Shape());
  for (++itr; itr != m_shps.end(); ++itr)
    tool_list.Append((*itr)->Shape());

  BRepAlgoAPI_Cut cut_op;
  cut_op.SetArguments(arguments);
  cut_op.SetTools(tool_list);  // Set tool shapes

  // Perform difference operation
  cut_op.Build();
  if (!cut_op.IsDone())
    return Status::user_error("Error: Difference operation failed");

  const TopoDS_Shape& result_shape = cut_op.Shape();
  if (result_shape.IsNull())
    return Status::user_error("Error: Resulting shape is null");

  extruded_shp_ptr shp = new Extruded_shp(ctx(), result_shape);
  shp->set_name("Cut");
  delete_operation_shps_();
  add_shp_(shp);

  return Status::ok();
}