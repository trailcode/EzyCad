#pragma once

#include <memory>

class Occt_view;

/// One undo/redo step. Subclasses record a forward change; `apply_reverse` undoes it and
/// `apply_forward` redoes it.
class Delta
{
public:
  virtual ~Delta() = default;

  virtual void                   apply_forward(Occt_view& view) = 0;
  virtual void                   apply_reverse(Occt_view& view) = 0;
  virtual std::unique_ptr<Delta> clone() const                  = 0;
};
