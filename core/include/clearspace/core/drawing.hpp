#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "geometry.hpp"
#include "types.hpp"

namespace clearspace::core {

enum class Color : std::uint8_t { Black = 0, Red = 1, Green = 2, Blue = 3 };
enum class Thickness : std::uint8_t { Thin = 0, Medium = 1, Thick = 2 };

struct Stroke {
  StrokeId id{};
  ClientId author{};
  Color color{};
  Thickness thickness{};
  Seq strokeOrder{};
  std::vector<Point> points;
  bool finished{false};
};

struct BoardState {
  std::vector<Stroke> strokes;
  std::unordered_map<ClientId, Point> cursors;
};

}
