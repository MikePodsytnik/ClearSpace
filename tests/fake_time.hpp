#pragma once
#include "clearspace/core/time.hpp"

namespace clearspace::core::test {

class FakeTimeProvider final : public ITimeProvider {
 public:
  TimePoint now() const override { return now_; }
  void set(TimePoint tp) { now_ = tp; }
  void advance(Duration d) { now_ += d; }

 private:
  TimePoint now_{};
};

}
