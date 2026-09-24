#include "shp_delta.h"

#include <unordered_set>

#include "gui_occt_view.h"

namespace
{
void remove_recs_(Occt_view& view, const std::vector<Shape_rec>& recs, const std::unordered_set<Shape_id>& keep_links);
void insert_recs_(Occt_view& view, const std::vector<Shape_rec>& recs);
void apply_links_(Occt_view& view, const std::vector<Shape_tree_delta::Link_change>& links, bool forward);

std::unordered_set<Shape_id> design_ids_(const std::vector<Shape_rec>& recs)
{
  std::unordered_set<Shape_id> ids;
  for (const Shape_rec& rec : recs)
    if (!rec.is_workbench)
      ids.insert(rec.id);

  return ids;
}
} // namespace

Shape_rec capture_shape_rec(const Shp& shp)
{
  Shape_rec rec;
  rec.id            = shp.get_id();
  rec.name          = shp.get_name();
  rec.material      = shp.Material();
  rec.geom          = shp.Shape();
  rec.frame         = shp.get_frame();
  rec.local_frame   = shp.get_local_frame();
  rec.parent_id     = shp.get_parent_id();
  rec.sibling_order = shp.get_sibling_order();
  rec.is_group         = shp.is_group();
  rec.is_workbench     = shp.is_workbench();
  rec.source_id        = shp.get_source_id();
  rec.visible          = shp.get_visible();
  rec.show_frame_axes  = shp.show_frame_axes();
  rec.show_frame_plane = shp.show_frame_plane();
  rec.show_frame_up    = shp.show_frame_up();
  return rec;
}

Shape_add_delta::Shape_add_delta(std::vector<Shape_rec> added)
    : m_added(std::move(added))
{
}

void Shape_add_delta::apply_forward(Occt_view& view) { insert_recs_(view, m_added); }

void Shape_add_delta::apply_reverse(Occt_view& view) { remove_recs_(view, m_added, {}); }

std::unique_ptr<Delta> Shape_add_delta::clone() const { return std::make_unique<Shape_add_delta>(m_added); }

Shape_remove_delta::Shape_remove_delta(std::vector<Shape_rec> removed)
    : m_removed(std::move(removed))
{
}

void Shape_remove_delta::apply_forward(Occt_view& view) { remove_recs_(view, m_removed, {}); }

void Shape_remove_delta::apply_reverse(Occt_view& view) { insert_recs_(view, m_removed); }

std::unique_ptr<Delta> Shape_remove_delta::clone() const { return std::make_unique<Shape_remove_delta>(m_removed); }

Shape_geom_delta::Shape_geom_delta(std::vector<Geom_change> changes)
    : m_changes(std::move(changes))
{
}

void Shape_geom_delta::apply_forward(Occt_view& view)
{
  for (const Geom_change& ch : m_changes)
  {
    view.set_shape_geom_by_id(ch.id, ch.after_geom, ch.after_frame);
    if (ch.has_local_frame)
      view.set_shape_local_frame_by_id(ch.id, ch.after_local_frame);
    if (ch.has_source_id)
      view.set_shape_source_id(ch.id, ch.after_source_id);
  }
}

void Shape_geom_delta::apply_reverse(Occt_view& view)
{
  for (const Geom_change& ch : m_changes)
  {
    view.set_shape_geom_by_id(ch.id, ch.before_geom, ch.before_frame);
    if (ch.has_local_frame)
      view.set_shape_local_frame_by_id(ch.id, ch.before_local_frame);
    if (ch.has_source_id)
      view.set_shape_source_id(ch.id, ch.before_source_id);
  }
}

std::unique_ptr<Delta> Shape_geom_delta::clone() const { return std::make_unique<Shape_geom_delta>(m_changes); }

Shape_replace_delta::Shape_replace_delta(std::vector<Shape_rec> removed, std::vector<Shape_rec> added)
    : m_removed(std::move(removed))
    , m_added(std::move(added))
{
}

void Shape_replace_delta::apply_forward(Occt_view& view)
{
  // Ids that return in m_added are the same part (cut/fuse/common/fillet). Do not drop their workbench links
  // in the gap before insert; insert_shape_rec syncs geometry.
  remove_recs_(view, m_removed, design_ids_(m_added));
  insert_recs_(view, m_added);
}

void Shape_replace_delta::apply_reverse(Occt_view& view)
{
  remove_recs_(view, m_added, design_ids_(m_removed));
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
  remove_recs_(view, m_removed, {});
}

void Shape_tree_delta::apply_reverse(Occt_view& view)
{
  insert_recs_(view, m_removed);
  apply_links_(view, m_links, false);
  remove_recs_(view, m_added, {});
}

std::unique_ptr<Delta> Shape_tree_delta::clone() const
{
  return std::make_unique<Shape_tree_delta>(m_added, m_removed, m_links);
}

namespace
{
void remove_recs_(Occt_view& view, const std::vector<Shape_rec>& recs, const std::unordered_set<Shape_id>& keep_links)
{
  for (const Shape_rec& rec : recs)
    view.remove_shape_by_id(rec.id, keep_links.find(rec.id) == keep_links.end());
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
