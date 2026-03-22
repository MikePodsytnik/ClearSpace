#include "clearspace/core/time.hpp"

namespace clearspace::core {

TimePoint SystemTimeProvider::now() const {
  return Clock::now();
}

std::shared_ptr<ITimeProvider> makeSystemTimeProvider() {
  return std::make_shared<SystemTimeProvider>();
}

}
