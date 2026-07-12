#pragma once

#include "delta.h"
#include "shp.h"

#include <TopoDS_Shape.hxx>

#include <memory>
#include <string>
#include <vector>

/// Serializable snapshot of one document shape for undo/redo (BREP text + attrs).
struct Shape_rec
{
  Shape_id    id{0};
  std::string name;
  int         material{0};
  std::string geom;
};

Shape_rec    capture_shape_rec(const Shp& shp);
std::string  shape_brep_string(const TopoDS_Shape& shape);
TopoDS_Shape shape_from_brep_string(const std::string& geom);

/// Adds shapes on forward; removes them on reverse.
class Shape_add_delta : public Delta
{
public:
  explicit Shape_add_delta(std::vector<Shape_rec> added);

  void                   apply_forward(Occt_view& view) override;
  void                   apply_reverse(Occt_view& view) override;
  std::unique_ptr<Delta> clone() const override;

private:
  std::vector<Shape_rec> m_added;
};

/// Removes shapes on forward; restores them on reverse.
class Shape_remove_delta : public Delta
{
public:
  explicit Shape_remove_delta(std::vector<Shape_rec> removed);

  void                   apply_forward(Occt_view& view) override;
  void                   apply_reverse(Occt_view& view) override;
  std::unique_ptr<Delta> clone() const override;

private:
  std::vector<Shape_rec> m_removed;
};

/// Replaces geometry of existing shapes in place (same ids).
class Shape_geom_delta : public Delta
{
public:
  struct Geom_change
  {
    Shape_id    id{0};
    std::string before_geom;
    std::string after_geom;
  };

  explicit Shape_geom_delta(std::vector<Geom_change> changes);

  void                   apply_forward(Occt_view& view) override;
  void                   apply_reverse(Occt_view& view) override;
  std::unique_ptr<Delta> clone() const override;

private:
  std::vector<Geom_change> m_changes;
};

/// Removes inputs and adds results (booleans, fillet/chamfer replace, polar combine).
class Shape_replace_delta : public Delta
{
public:
  Shape_replace_delta(std::vector<Shape_rec> removed, std::vector<Shape_rec> added);

  void                   apply_forward(Occt_view& view) override;
  void                   apply_reverse(Occt_view& view) override;
  std::unique_ptr<Delta> clone() const override;

private:
  std::vector<Shape_rec> m_removed;
  std::vector<Shape_rec> m_added;
};
