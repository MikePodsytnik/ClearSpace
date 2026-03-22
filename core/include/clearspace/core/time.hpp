#pragma once

#include <chrono>
#include <memory>

namespace clearspace::core {

using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
using Duration = Clock::duration;

class ITimeProvider {
 public:
  virtual ~ITimeProvider() = default;
  virtual TimePoint now() const = 0;
};

class SystemTimeProvider final : public ITimeProvider {
 public:
  TimePoint now() const override;
};

std::shared_ptr<ITimeProvider> makeSystemTimeProvider();

}
