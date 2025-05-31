#include "shp_cut.h"

#include <TopTools_ListOfShape.hxx>
#include <BRepAlgoAPI_Cut.hxx>

Shp_cut::Shp_cut(Occt_view& view)
    : Shp_operation_base(view) {}

Status Shp_cut::selected_cut()
{
  std::vector<AIS_Shape_ptr> selected = get_selected();
  if (selected.size() < 2)
    return Status::user_error("Select two or more shapes.");
  
  std::vector<AIS_Shape_ptr>::iterator itr = selected.begin();

  TopTools_ListOfShape tool_list;
  TopTools_ListOfShape arguments;
  arguments.Append((*itr)->Shape());
  for (++itr; itr != selected.end(); ++itr)
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

  ExtrudedShp_ptr shp = new ExtrudedShp(ctx(), result_shape);
  shp->set_name("Cut");
  delete_selected_();
  add_shp_(shp);

  return Status::ok();
}