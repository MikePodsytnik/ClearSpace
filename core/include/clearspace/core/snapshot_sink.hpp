#pragma once

#include <vector>

#include "drawing.hpp"
#include "types.hpp"

namespace clearspace::core {

struct PersistedBoardSnapshot {
  RoomId room{};
  Seq lastSeq{};
  std::vector<Stroke> strokes;
};

class ISnapshotSink {
 public:
  virtual ~ISnapshotSink() = default;
  virtual void enqueue(PersistedBoardSnapshot snapshot) = 0;
};

}
