#pragma once

namespace clearspace::core {
struct Point {
  float x{};
  float y{};
  friend bool operator==(const Point& a, const Point& b) noexcept {
    return a.x == b.x && a.y == b.y;
  }
};
}
