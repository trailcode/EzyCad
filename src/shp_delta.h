#pragma once

#include "delta.h"
#include "shp.h"

#include <TopoDS_Shape.hxx>

#include <memory>
#include <string>
#include <vector>

/// In-memory snapshot of one document shape for undo/redo (shared TopoDS_Shape + attrs).
struct Shape_rec
{
  Shape_id     id{0};
  std::string  name;
  int          material{0};
  TopoDS_Shape geom;
  Shape_id     parent_id{0};
  int          sibling_order{0};
  bool         is_group{false};
  bool         visible{true};
};

Shape_rec capture_shape_rec(const Shp& shp);

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
    Shape_id     id{0};
    TopoDS_Shape before_geom;
    TopoDS_Shape after_geom;
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

/// Parent/sibling-order edits plus optional group add/remove (group, ungroup, reparent).
class Shape_tree_delta : public Delta
{
public:
  struct Link_change
  {
    Shape_id id{0};
    Shape_id old_parent{0};
    int      old_order{0};
    Shape_id new_parent{0};
    int      new_order{0};
  };

  Shape_tree_delta(std::vector<Shape_rec> added, std::vector<Shape_rec> removed, std::vector<Link_change> links);

  void                   apply_forward(Occt_view& view) override;
  void                   apply_reverse(Occt_view& view) override;
  std::unique_ptr<Delta> clone() const override;

private:
  std::vector<Shape_rec>  m_added;
  std::vector<Shape_rec>  m_removed;
  std::vector<Link_change> m_links;
};
