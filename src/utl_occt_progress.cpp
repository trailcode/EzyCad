#include "utl_occt_progress.h"

void Atomic_progress_indicator::set_stage(std::string stage)
{
  std::lock_guard<std::mutex> lock(m_mu);
  m_stage = std::move(stage);
}

std::string Atomic_progress_indicator::stage() const
{
  std::lock_guard<std::mutex> lock(m_mu);
  return m_stage;
}

std::string Atomic_progress_indicator::scope_name() const
{
  std::lock_guard<std::mutex> lock(m_mu);
  return m_scope;
}

void Atomic_progress_indicator::Show(const Message_ProgressScope& theScope, const bool /*isForce*/)
{
  m_pos.store(static_cast<float>(GetPosition()), std::memory_order_relaxed);
  const char*                 name = theScope.Name();
  std::lock_guard<std::mutex> lock(m_mu);
  if (name && name[0] != '\0')
    m_scope = name;
}

void Atomic_progress_indicator::Reset()
{
  m_pos.store(0.f, std::memory_order_relaxed);
  std::lock_guard<std::mutex> lock(m_mu);
  m_scope.clear();
}
