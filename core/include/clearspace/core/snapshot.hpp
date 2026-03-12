#pragma once

#include <vector>

#include "drawing.hpp"
#include "types.hpp"

namespace clearspace::core {

struct RoomSnapshot {
  RoomId room{};
  Seq lastSeq{};
  BoardState state{};
  std::vector<ClientId> members;
};

}
