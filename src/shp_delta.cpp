#include "shp_delta.h"

#include "gui_occt_view.h"

Shape_rec capture_shape_rec(const Shp& shp)
{
  Shape_rec rec;
  rec.id            = shp.get_id();
  rec.name          = shp.get_name();
  rec.material      = shp.Material();
  rec.geom          = shp.Shape();
  rec.frame         = shp.get_frame();
  rec.parent_id     = shp.get_parent_id();
  rec.sibling_order = shp.get_sibling_order();
  rec.is_group      = shp.is_group();
  rec.visible       = shp.get_visible();
  return rec;
}

namespace
{

void remove_recs_(Occt_view& view, const std::vector<Shape_rec>& recs)
{
  for (const Shape_rec& rec : recs)
    view.remove_shape_by_id(rec.id);
}

void insert_recs_(Occt_view& view, const std::vector<Shape_rec>& recs)
{
  for (const Shape_rec& rec : recs)
    view.insert_shape_rec(rec);
}

void apply_links_(Occt_view& view, const std::vector<Shape_tree_delta::Link_change>& links, bool forward)
{
  for (const Shape_tree_delta::Link_change& ch : links)
  {
    Shp_ptr shp = view.find_shape_by_id(ch.id);
    if (shp.IsNull())
      continue;

    if (forward)
    {
      shp->set_parent_id(ch.new_parent);
      shp->set_sibling_order(ch.new_order);
    }
    else
    {
      shp->set_parent_id(ch.old_parent);
      shp->set_sibling_order(ch.old_order);
    }
  }
  view.sync_sketch_shape_faint_style();
}

} // namespace

Shape_add_delta::Shape_add_delta(std::vector<Shape_rec> added)
    : m_added(std::move(added))
{
}

void Shape_add_delta::apply_forward(Occt_view& view) { insert_recs_(view, m_added); }

void Shape_add_delta::apply_reverse(Occt_view& view) { remove_recs_(view, m_added); }

std::unique_ptr<Delta> Shape_add_delta::clone() const { return std::make_unique<Shape_add_delta>(m_added); }

Shape_remove_delta::Shape_remove_delta(std::vector<Shape_rec> removed)
    : m_removed(std::move(removed))
{
}

void Shape_remove_delta::apply_forward(Occt_view& view) { remove_recs_(view, m_removed); }

void Shape_remove_delta::apply_reverse(Occt_view& view) { insert_recs_(view, m_removed); }

std::unique_ptr<Delta> Shape_remove_delta::clone() const { return std::make_unique<Shape_remove_delta>(m_removed); }

Shape_geom_delta::Shape_geom_delta(std::vector<Geom_change> changes)
    : m_changes(std::move(changes))
{
}

void Shape_geom_delta::apply_forward(Occt_view& view)
{
  for (const Geom_change& ch : m_changes)
    view.set_shape_geom_by_id(ch.id, ch.after_geom, ch.after_frame);
}

void Shape_geom_delta::apply_reverse(Occt_view& view)
{
  for (const Geom_change& ch : m_changes)
    view.set_shape_geom_by_id(ch.id, ch.before_geom, ch.before_frame);
}

std::unique_ptr<Delta> Shape_geom_delta::clone() const { return std::make_unique<Shape_geom_delta>(m_changes); }

Shape_replace_delta::Shape_replace_delta(std::vector<Shape_rec> removed, std::vector<Shape_rec> added)
    : m_removed(std::move(removed))
    , m_added(std::move(added))
{
}

void Shape_replace_delta::apply_forward(Occt_view& view)
{
  remove_recs_(view, m_removed);
  insert_recs_(view, m_added);
}

void Shape_replace_delta::apply_reverse(Occt_view& view)
{
  remove_recs_(view, m_added);
  insert_recs_(view, m_removed);
}

std::unique_ptr<Delta> Shape_replace_delta::clone() const { return std::make_unique<Shape_replace_delta>(m_removed, m_added); }

Shape_tree_delta::Shape_tree_delta(std::vector<Shape_rec> added, std::vector<Shape_rec> removed, std::vector<Link_change> links)
    : m_added(std::move(added))
    , m_removed(std::move(removed))
    , m_links(std::move(links))
{
}

void Shape_tree_delta::apply_forward(Occt_view& view)
{
  insert_recs_(view, m_added);
  apply_links_(view, m_links, true);
  remove_recs_(view, m_removed);
}

void Shape_tree_delta::apply_reverse(Occt_view& view)
{
  insert_recs_(view, m_removed);
  apply_links_(view, m_links, false);
  remove_recs_(view, m_added);
}

std::unique_ptr<Delta> Shape_tree_delta::clone() const
{
  return std::make_unique<Shape_tree_delta>(m_added, m_removed, m_links);
}
