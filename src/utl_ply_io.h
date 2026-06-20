#pragma once

#include <string>

#include <TopoDS_Shape.hxx>

#include "utl.h"

/// Parses PLY (ASCII or binary_little_endian) with vertex x/y/z and triangular faces.
[[nodiscard]] Status import_ply_shape(const std::string& file_bytes, TopoDS_Shape& out_shape);

/// Writes binary_little_endian PLY from mesh on \a shape (run `BRepMesh_IncrementalMesh` first).
[[nodiscard]] Status export_ply_binary_file(const TopoDS_Shape& shape, const std::string& file_path);
