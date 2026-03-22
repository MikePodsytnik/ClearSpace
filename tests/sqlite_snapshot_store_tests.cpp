#include <gtest/gtest.h>

#include <filesystem>

#include "clearspace/persistence/sqlite_snapshot_store.hpp"

namespace clearspace::persistence {

static std::filesystem::path tempDbPath(const char* name) {
  auto p = std::filesystem::temp_directory_path() / name;
  std::error_code ec;
  std::filesystem::remove(p, ec);
  return p;
}

TEST(SqliteSnapshotStore, SaveListLoadLatestAndPruneKeep10) {
  auto db = tempDbPath("clearspace_snapshots_test.sqlite");
  SqliteSnapshotStore store(db.string());
  store.init();

  clearspace::core::RoomId room = 42;

  clearspace::core::Stroke s;
  s.id = 1;
  s.author = 7;
  s.color = clearspace::core::Color::Red;
  s.thickness = clearspace::core::Thickness::Medium;
  s.strokeOrder = 1;
  s.finished = true;
  s.points.push_back(clearspace::core::Point{1, 2});

  for (int i = 1; i <= 12; ++i) {
    s.strokeOrder = static_cast<clearspace::core::Seq>(i);
    store.save(room, static_cast<clearspace::core::Seq>(i), std::vector<clearspace::core::Stroke>{s});
  }

  auto all = store.list(room);
  ASSERT_EQ(all.size(), 12u);

  store.pruneKeepLatest(room, 10);
  auto pruned = store.list(room);
  ASSERT_EQ(pruned.size(), 10u);

  auto latest = store.loadLatest(room);
  ASSERT_TRUE(latest.has_value());
  ASSERT_EQ(latest->meta.room, room);
  ASSERT_EQ(latest->strokes.size(), 1u);
  ASSERT_EQ(latest->strokes[0].points.size(), 1u);

  auto byId = store.loadById(latest->meta.id);
  ASSERT_TRUE(byId.has_value());
  ASSERT_EQ(byId->meta.id, latest->meta.id);

  std::error_code ec;
  std::filesystem::remove(db, ec);
}

}
