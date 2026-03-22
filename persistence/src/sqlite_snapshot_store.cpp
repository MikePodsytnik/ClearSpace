#include "clearspace/persistence/sqlite_snapshot_store.hpp"

#include <chrono>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include <sqlite3.h>

namespace {

std::int64_t nowMs() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

void throwIf(int rc, sqlite3* db) {
  if (rc == SQLITE_OK || rc == SQLITE_DONE || rc == SQLITE_ROW) {
    return;
  }
  const char* msg = db ? sqlite3_errmsg(db) : "sqlite error";
  throw std::runtime_error(msg);
}

struct Stmt {
  sqlite3_stmt* s{nullptr};
  Stmt(sqlite3* db, const char* sql) {
    throwIf(sqlite3_prepare_v2(db, sql, -1, &s, nullptr), db);
  }
  ~Stmt() {
    if (s) sqlite3_finalize(s);
  }
};

template <class T>
void appendLE(std::vector<std::uint8_t>& buf, const T& v) {
  static_assert(std::is_trivially_copyable_v<T>);
  const std::uint8_t* p = reinterpret_cast<const std::uint8_t*>(&v);
  buf.insert(buf.end(), p, p + sizeof(T));
}

template <class T>
T readLE(const std::uint8_t*& p, const std::uint8_t* end) {
  static_assert(std::is_trivially_copyable_v<T>);
  if (static_cast<std::size_t>(end - p) < sizeof(T)) {
    throw std::runtime_error("corrupted snapshot blob");
  }
  T v{};
  std::memcpy(&v, p, sizeof(T));
  p += sizeof(T);
  return v;
}

std::vector<std::uint8_t> serializeStrokes(const std::vector<clearspace::core::Stroke>& strokes) {
  std::vector<std::uint8_t> buf;
  const std::uint32_t version = 1;
  appendLE(buf, version);
  appendLE(buf, static_cast<std::uint32_t>(strokes.size()));

  for (const auto& s : strokes) {
    appendLE(buf, static_cast<std::uint64_t>(s.id));
    appendLE(buf, static_cast<std::uint64_t>(s.author));
    appendLE(buf, static_cast<std::uint8_t>(s.color));
    appendLE(buf, static_cast<std::uint8_t>(s.thickness));
    appendLE(buf, static_cast<std::uint64_t>(s.strokeOrder));
    appendLE(buf, static_cast<std::uint8_t>(s.finished ? 1 : 0));
    appendLE(buf, static_cast<std::uint32_t>(s.points.size()));
    for (const auto& pt : s.points) {
      appendLE(buf, pt.x);
      appendLE(buf, pt.y);
    }
  }
  return buf;
}

std::vector<clearspace::core::Stroke> deserializeStrokes(const void* data, int size) {
  const auto* p = reinterpret_cast<const std::uint8_t*>(data);
  const auto* end = p + size;

  const std::uint32_t version = readLE<std::uint32_t>(p, end);
  if (version != 1) throw std::runtime_error("unsupported snapshot version");

  const std::uint32_t strokesCount = readLE<std::uint32_t>(p, end);
  std::vector<clearspace::core::Stroke> strokes;
  strokes.reserve(strokesCount);

  for (std::uint32_t i = 0; i < strokesCount; ++i) {
    clearspace::core::Stroke s;
    s.id = static_cast<clearspace::core::StrokeId>(readLE<std::uint64_t>(p, end));
    s.author = static_cast<clearspace::core::ClientId>(readLE<std::uint64_t>(p, end));
    s.color = static_cast<clearspace::core::Color>(readLE<std::uint8_t>(p, end));
    s.thickness = static_cast<clearspace::core::Thickness>(readLE<std::uint8_t>(p, end));
    s.strokeOrder = static_cast<clearspace::core::Seq>(readLE<std::uint64_t>(p, end));
    s.finished = readLE<std::uint8_t>(p, end) != 0;

    const std::uint32_t pointsCount = readLE<std::uint32_t>(p, end);
    s.points.reserve(pointsCount);
    for (std::uint32_t j = 0; j < pointsCount; ++j) {
      float x = readLE<float>(p, end);
      float y = readLE<float>(p, end);
      s.points.push_back(clearspace::core::Point{x, y});
    }
    strokes.push_back(std::move(s));
  }

  if (p != end) throw std::runtime_error("corrupted snapshot blob (trailing bytes)");
  return strokes;
}

}

namespace clearspace::persistence {

SqliteSnapshotStore::SqliteSnapshotStore(std::string dbPath) : dbPath_(std::move(dbPath)) {}

SqliteSnapshotStore::~SqliteSnapshotStore() {
  if (db_) {
    sqlite3_close(db_);
    db_ = nullptr;
  }
}

void SqliteSnapshotStore::init() {
  if (db_) return;

  throwIf(sqlite3_open(dbPath_.c_str(), &db_), db_);
  throwIf(sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr), db_);

  const char* ddl =
      "CREATE TABLE IF NOT EXISTS snapshots ("
      "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "  room_id INTEGER NOT NULL,"
      "  saved_at_ms INTEGER NOT NULL,"
      "  last_seq INTEGER NOT NULL,"
      "  data BLOB NOT NULL"
      ");"
      "CREATE INDEX IF NOT EXISTS idx_snapshots_room_saved ON snapshots(room_id, saved_at_ms);";
  throwIf(sqlite3_exec(db_, ddl, nullptr, nullptr, nullptr), db_);
}

void SqliteSnapshotStore::save(clearspace::core::RoomId room,
                              clearspace::core::Seq lastSeq,
                              const std::vector<clearspace::core::Stroke>& strokes) {
  init();
  const auto blob = serializeStrokes(strokes);
  const std::int64_t savedAt = nowMs();

  Stmt st(db_, "INSERT INTO snapshots(room_id, saved_at_ms, last_seq, data) VALUES(?, ?, ?, ?);");
  throwIf(sqlite3_bind_int64(st.s, 1, static_cast<sqlite3_int64>(room)), db_);
  throwIf(sqlite3_bind_int64(st.s, 2, static_cast<sqlite3_int64>(savedAt)), db_);
  throwIf(sqlite3_bind_int64(st.s, 3, static_cast<sqlite3_int64>(lastSeq)), db_);
  throwIf(sqlite3_bind_blob(st.s, 4, blob.data(), static_cast<int>(blob.size()), SQLITE_TRANSIENT), db_);
  throwIf(sqlite3_step(st.s), db_);
}

std::vector<SnapshotMeta> SqliteSnapshotStore::list(clearspace::core::RoomId room) {
  init();
  std::vector<SnapshotMeta> res;

  Stmt st(db_, "SELECT id, room_id, saved_at_ms, last_seq FROM snapshots WHERE room_id=? ORDER BY saved_at_ms DESC;");
  throwIf(sqlite3_bind_int64(st.s, 1, static_cast<sqlite3_int64>(room)), db_);

  while (true) {
    int rc = sqlite3_step(st.s);
    if (rc == SQLITE_DONE) break;
    throwIf(rc, db_);

    SnapshotMeta m;
    m.id = sqlite3_column_int64(st.s, 0);
    m.room = static_cast<clearspace::core::RoomId>(sqlite3_column_int64(st.s, 1));
    m.savedAtMs = sqlite3_column_int64(st.s, 2);
    m.lastSeq = static_cast<clearspace::core::Seq>(sqlite3_column_int64(st.s, 3));
    res.push_back(m);
  }

  return res;
}

std::optional<StoredSnapshot> SqliteSnapshotStore::loadById(std::int64_t id) {
  init();
  Stmt st(db_, "SELECT id, room_id, saved_at_ms, last_seq, data FROM snapshots WHERE id=?;");
  throwIf(sqlite3_bind_int64(st.s, 1, static_cast<sqlite3_int64>(id)), db_);

  int rc = sqlite3_step(st.s);
  if (rc == SQLITE_DONE) return std::nullopt;
  throwIf(rc, db_);

  StoredSnapshot res;
  res.meta.id = sqlite3_column_int64(st.s, 0);
  res.meta.room = static_cast<clearspace::core::RoomId>(sqlite3_column_int64(st.s, 1));
  res.meta.savedAtMs = sqlite3_column_int64(st.s, 2);
  res.meta.lastSeq = static_cast<clearspace::core::Seq>(sqlite3_column_int64(st.s, 3));

  const void* data = sqlite3_column_blob(st.s, 4);
  int size = sqlite3_column_bytes(st.s, 4);
  if (!data || size <= 0) throw std::runtime_error("empty snapshot blob");
  res.strokes = deserializeStrokes(data, size);
  return res;
}

std::optional<StoredSnapshot> SqliteSnapshotStore::loadLatest(clearspace::core::RoomId room) {
  init();
  Stmt st(db_,
          "SELECT id, room_id, saved_at_ms, last_seq, data "
          "FROM snapshots WHERE room_id=? ORDER BY saved_at_ms DESC LIMIT 1;");
  throwIf(sqlite3_bind_int64(st.s, 1, static_cast<sqlite3_int64>(room)), db_);

  int rc = sqlite3_step(st.s);
  if (rc == SQLITE_DONE) return std::nullopt;
  throwIf(rc, db_);

  StoredSnapshot res;
  res.meta.id = sqlite3_column_int64(st.s, 0);
  res.meta.room = static_cast<clearspace::core::RoomId>(sqlite3_column_int64(st.s, 1));
  res.meta.savedAtMs = sqlite3_column_int64(st.s, 2);
  res.meta.lastSeq = static_cast<clearspace::core::Seq>(sqlite3_column_int64(st.s, 3));

  const void* data = sqlite3_column_blob(st.s, 4);
  int size = sqlite3_column_bytes(st.s, 4);
  if (!data || size <= 0) throw std::runtime_error("empty snapshot blob");
  res.strokes = deserializeStrokes(data, size);
  return res;
}

void SqliteSnapshotStore::pruneKeepLatest(clearspace::core::RoomId room, std::size_t keep) {
  init();
  if (keep == 0) {
    Stmt st(db_, "DELETE FROM snapshots WHERE room_id=?;");
    throwIf(sqlite3_bind_int64(st.s, 1, static_cast<sqlite3_int64>(room)), db_);
    throwIf(sqlite3_step(st.s), db_);
    return;
  }

  Stmt st(db_,
          "DELETE FROM snapshots "
          "WHERE room_id=? AND id NOT IN ("
          "  SELECT id FROM snapshots WHERE room_id=? ORDER BY saved_at_ms DESC LIMIT ?"
          ");");
  throwIf(sqlite3_bind_int64(st.s, 1, static_cast<sqlite3_int64>(room)), db_);
  throwIf(sqlite3_bind_int64(st.s, 2, static_cast<sqlite3_int64>(room)), db_);
  throwIf(sqlite3_bind_int64(st.s, 3, static_cast<sqlite3_int64>(keep)), db_);
  throwIf(sqlite3_step(st.s), db_);
}

}
