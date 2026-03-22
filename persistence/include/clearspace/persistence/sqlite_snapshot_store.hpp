#pragma once

#include <string>

#include "snapshot_store.hpp"

struct sqlite3;

namespace clearspace::persistence {

class SqliteSnapshotStore final : public ISnapshotStore {
 public:
  explicit SqliteSnapshotStore(std::string dbPath);
  ~SqliteSnapshotStore() override;

  SqliteSnapshotStore(const SqliteSnapshotStore&) = delete;
  SqliteSnapshotStore& operator=(const SqliteSnapshotStore&) = delete;

  void init() override;

  void save(clearspace::core::RoomId room,
            clearspace::core::Seq lastSeq,
            const std::vector<clearspace::core::Stroke>& strokes) override;

  std::vector<SnapshotMeta> list(clearspace::core::RoomId room) override;

  std::optional<StoredSnapshot> loadById(std::int64_t id) override;

  std::optional<StoredSnapshot> loadLatest(clearspace::core::RoomId room) override;

  void pruneKeepLatest(clearspace::core::RoomId room, std::size_t keep) override;

 private:
  std::string dbPath_;
  sqlite3* db_{nullptr};
};

}
