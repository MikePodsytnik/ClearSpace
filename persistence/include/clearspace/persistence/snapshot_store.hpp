#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "clearspace/core/drawing.hpp"
#include "clearspace/core/types.hpp"

namespace clearspace::persistence {

struct SnapshotMeta {
  std::int64_t id{};
  clearspace::core::RoomId room{};
  std::int64_t savedAtMs{};
  clearspace::core::Seq lastSeq{};
};

struct StoredSnapshot {
  SnapshotMeta meta{};
  std::vector<clearspace::core::Stroke> strokes;
};

class ISnapshotStore {
 public:
  virtual ~ISnapshotStore() = default;

  virtual void init() = 0;

  virtual void save(clearspace::core::RoomId room,
                    clearspace::core::Seq lastSeq,
                    const std::vector<clearspace::core::Stroke>& strokes) = 0;

  virtual std::vector<SnapshotMeta> list(clearspace::core::RoomId room) = 0;

  virtual std::optional<StoredSnapshot> loadById(std::int64_t id) = 0;

  virtual std::optional<StoredSnapshot> loadLatest(clearspace::core::RoomId room) = 0;

  virtual void pruneKeepLatest(clearspace::core::RoomId room, std::size_t keep) = 0;
};

}
