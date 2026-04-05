#pragma once

#include <TopAbs_ShapeEnum.hxx>
#include <array>
#include <cstddef>
#include <string_view>

// Parallel to Open CASCADE TopAbs_ShapeEnum (TopAbs_ShapeEnum.hxx). Order must stay in sync with OCCT.
#define EZY_TOPABS_SHAPE_ENUM_LIST(X)                                                                  \
  X(Compound)                                                                                          \
  X(CompSolid)                                                                                         \
  X(Solid)                                                                                             \
  X(Shell)                                                                                             \
  X(Face)                                                                                              \
  X(Wire)                                                                                              \
  X(Edge)                                                                                              \
  X(Vertex)                                                                                            \
  X(Shape)

constexpr std::array<std::string_view, static_cast<std::size_t>(TopAbs_SHAPE) + 1u> c_names_TopAbs_ShapeEnum {
#define EZY_TOPABS_SHAPE_STR(name) #name,
    EZY_TOPABS_SHAPE_ENUM_LIST(EZY_TOPABS_SHAPE_STR)
#undef EZY_TOPABS_SHAPE_STR
};

static_assert(c_names_TopAbs_ShapeEnum.size() == static_cast<std::size_t>(TopAbs_SHAPE) + 1u);

#undef EZY_TOPABS_SHAPE_ENUM_LIST
