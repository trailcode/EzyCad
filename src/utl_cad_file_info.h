#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <Message_ProgressRange.hxx>
#include <TopoDS_Shape.hxx>

#include "utl.h"
#include "utl_occt_progress.h"

/// Read-only metadata for CAD/mesh files EzyCad can import and/or export.
/// Does not modify the document.
namespace utl_cad_file_info
{
enum class Format : uint8_t
{
  Unknown,
  Step,
  Iges,
  Stl,
  Ply
};

struct Line
{
  std::string label;
  std::string value;
};

/// One importable body from STEP (geometry may still need project unit scaling).
struct Named_body
{
  TopoDS_Shape shape;
  std::string  name; // empty if the file had no usable name
};

/// STEP XCAF node: assemblies become groups (`is_group`); leaves carry geometry.
struct Named_node
{
  TopoDS_Shape shape; // null/empty when is_group
  std::string  name;
  bool         is_group{false};
  int          parent_index{-1}; // index in the output vector, or -1 for root
};

[[nodiscard]] Format detect(const std::string& file_path, const std::string& file_bytes);

/// True for formats File -> Import can load (STEP, PLY).
[[nodiscard]] bool can_import(Format fmt);

[[nodiscard]] const char* format_label(Format fmt);

/// Collect label/value rows for the Import dialog.
/// Optional \a progress receives stage text and Transfer percent (STEP/IGES).
[[nodiscard]] std::vector<Line> collect(const std::string& file_path, const std::string& file_bytes,
                                        const Atomic_progress_indicator_ptr& progress = {});

/// Read STEP bodies with XCAF product/part names when present (flat; assemblies expanded to leaves).
[[nodiscard]] Status read_step_named_bodies(const std::string& file_bytes, std::vector<Named_body>& out,
                                            const Message_ProgressRange& progress = Message_ProgressRange());

/// Read STEP as a group/leaf tree (XCAF assemblies preserved). Falls back to flat bodies as root leaves.
[[nodiscard]] Status read_step_named_tree(const std::string& file_bytes, std::vector<Named_node>& out,
                                          const Message_ProgressRange& progress = Message_ProgressRange());
} // namespace utl_cad_file_info
