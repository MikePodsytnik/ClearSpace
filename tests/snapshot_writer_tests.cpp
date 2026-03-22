#include <gtest/gtest.h>

#include <filesystem>
#include <memory>

#include "clearspace/persistence/snapshot_writer.hpp"
#include "clearspace/persistence/sqlite_snapshot_store.hpp"

namespace clearspace::persistence {

static std::filesystem::path tempDbPath(const char* name) {
  auto p = std::filesystem::temp_directory_path() / name;
  std::error_code ec;
  std::filesystem::remove(p, ec);
  return p;
}

TEST(SnapshotWriter, EnqueueFlushAndRetention) {
  auto db = tempDbPath("clearspace_writer_test.sqlite");
  auto store = std::make_shared<SqliteSnapshotStore>(db.string());

  SnapshotWriter writer(store, 10);

  clearspace::core::RoomId room = 1;

  clearspace::core::Stroke s;
  s.id = 1;
  s.author = 1;
  s.color = clearspace::core::Color::Black;
  s.thickness = clearspace::core::Thickness::Thin;
  s.finished = true;
  s.points.push_back(clearspace::core::Point{0, 0});

  for (int i = 1; i <= 12; ++i) {
    s.strokeOrder = static_cast<clearspace::core::Seq>(i);
    clearspace::core::PersistedBoardSnapshot snap;
    snap.room = room;
    snap.lastSeq = static_cast<clearspace::core::Seq>(i);
    snap.strokes = {s};
    writer.enqueue(std::move(snap));
  }

  writer.flush();

  auto list = store->list(room);
  ASSERT_EQ(list.size(), 10u);

  auto latest = store->loadLatest(room);
  ASSERT_TRUE(latest.has_value());
  ASSERT_EQ(latest->strokes.size(), 1u);

  std::error_code ec;
  std::filesystem::remove(db, ec);
}

}
