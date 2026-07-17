#include "doc_delta.h"

#include "gui_occt_view.h"
#include "skt.h"
#include "skt_json.h"
#include "utl.h"

Sketch_struct_delta::Sketch_struct_delta(Kind kind, nlohmann::json sketch_json, bool was_current,
                                         std::optional<nlohmann::json> auto_created_default_json)
    : m_kind(kind)
    , m_sketch_json(std::move(sketch_json))
    , m_was_current(was_current)
    , m_auto_created_default_json(std::move(auto_created_default_json))
{
}

void Sketch_struct_delta::add_sketch_(Occt_view& view, const nlohmann::json& sketch_json, bool make_current) const
{
  view.undo_insert_sketch(sketch_json, make_current);
}

void Sketch_struct_delta::remove_sketch_by_id_(Occt_view& view, size_t sketch_id) const
{
  view.undo_remove_sketch(sketch_id);
}

void Sketch_struct_delta::apply_forward(Occt_view& view)
{
  if (m_kind == Kind::Add)
  {
    add_sketch_(view, m_sketch_json, m_was_current);
    return;
  }

  const size_t id = m_sketch_json["id"].get<size_t>();
  remove_sketch_by_id_(view, id);
  if (m_auto_created_default_json)
    add_sketch_(view, *m_auto_created_default_json, true);
}

void Sketch_struct_delta::apply_reverse(Occt_view& view)
{
  if (m_kind == Kind::Add)
  {
    const size_t id = m_sketch_json["id"].get<size_t>();
    remove_sketch_by_id_(view, id);
    if (view.get_sketches().empty())
      view.ensure_current_sketch_for_undo();

    return;
  }

  if (m_auto_created_default_json)
    remove_sketch_by_id_(view, (*m_auto_created_default_json)["id"].get<size_t>());

  add_sketch_(view, m_sketch_json, m_was_current);
}

std::unique_ptr<Delta> Sketch_struct_delta::clone() const
{
  return std::make_unique<Sketch_struct_delta>(m_kind, m_sketch_json, m_was_current, m_auto_created_default_json);
}

Underlay_delta::Underlay_delta(size_t sketch_id, nlohmann::json before, nlohmann::json after)
    : m_sketch_id(sketch_id)
    , m_before(std::move(before))
    , m_after(std::move(after))
{
}

void Underlay_delta::apply_json_(Occt_view& view, const nlohmann::json& j) const
{
  for (const auto& sketch : view.get_sketches())
  {
    if (sketch->get_id() != m_sketch_id)
      continue;

    Sketch_underlay& ul = sketch->underlay();
    if (j.is_null() || (j.is_object() && j.empty()))
    {
      ul.clear_and_update();
      return;
    }

    if (!ul.from_json(j, view.asset_store()))
      ul.clear_and_update();
    else
      ul.rebuild_display(sketch->get_plane(), sketch->is_visible());

    return;
  }
}

void Underlay_delta::apply_forward(Occt_view& view) { apply_json_(view, m_after); }

void Underlay_delta::apply_reverse(Occt_view& view) { apply_json_(view, m_before); }

std::unique_ptr<Delta> Underlay_delta::clone() const
{
  return std::make_unique<Underlay_delta>(m_sketch_id, m_before, m_after);
}

Composite_delta::Composite_delta(std::vector<std::unique_ptr<Delta>> parts)
    : m_parts(std::move(parts))
{
}

void Composite_delta::apply_forward(Occt_view& view)
{
  for (std::unique_ptr<Delta>& part : m_parts)
    part->apply_forward(view);
}

void Composite_delta::apply_reverse(Occt_view& view)
{
  for (auto it = m_parts.rbegin(); it != m_parts.rend(); ++it)
    (*it)->apply_reverse(view);
}

std::unique_ptr<Delta> Composite_delta::clone() const
{
  std::vector<std::unique_ptr<Delta>> copies;
  copies.reserve(m_parts.size());
  for (const std::unique_ptr<Delta>& part : m_parts)
    copies.push_back(part->clone());

  return std::make_unique<Composite_delta>(std::move(copies));
}
