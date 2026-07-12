#include "shp_delta.h"

#include "gui_occt_view.h"

Shape_rec capture_shape_rec(const Shp& shp)
{
  Shape_rec rec;
  rec.id       = shp.get_id();
  rec.name     = shp.get_name();
  rec.material = shp.Material();
  rec.geom     = shp.Shape();
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
    view.set_shape_geom_by_id(ch.id, ch.after_geom);
}

void Shape_geom_delta::apply_reverse(Occt_view& view)
{
  for (const Geom_change& ch : m_changes)
    view.set_shape_geom_by_id(ch.id, ch.before_geom);
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

std::unique_ptr<Delta> Shape_replace_delta::clone() const
{
  return std::make_unique<Shape_replace_delta>(m_removed, m_added);
}
