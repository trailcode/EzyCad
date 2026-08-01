#pragma once

#include <atomic>
#include <mutex>
#include <string>

#include <Message_ProgressIndicator.hxx>
#include <Message_ProgressScope.hxx>

#include "utl_types.h"

using Message_ProgressIndicator_ptr = opencascade::handle<Message_ProgressIndicator>;

/// Thread-safe OCCT progress sink for background STEP transfer (poll from the UI thread).
class Atomic_progress_indicator : public Message_ProgressIndicator
{
  DEFINE_STANDARD_RTTI_INLINE(Atomic_progress_indicator, Message_ProgressIndicator)

public:
  Atomic_progress_indicator() = default;

  void               request_cancel() { m_cancel.store(true, std::memory_order_relaxed); }
  [[nodiscard]] bool cancelled() const { return m_cancel.load(std::memory_order_relaxed); }

  /// Overall position in [0, 1] from the last OCCT Show() (0 if transfer has not started).
  [[nodiscard]] float position() const { return m_pos.load(std::memory_order_relaxed); }

  void                      set_stage(std::string stage);
  [[nodiscard]] std::string stage() const;
  [[nodiscard]] std::string scope_name() const;

protected:
  void Show(const Message_ProgressScope& theScope, const bool /*isForce*/) override;
  bool UserBreak() override { return m_cancel.load(std::memory_order_relaxed); }
  void Reset() override;

private:
  std::atomic<float> m_pos{0.f};
  std::atomic<bool>  m_cancel{false};
  mutable std::mutex m_mu;
  std::string        m_stage;
  std::string        m_scope;
};

using Atomic_progress_indicator_ptr = opencascade::handle<Atomic_progress_indicator>;
