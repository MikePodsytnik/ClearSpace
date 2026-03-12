#include "clearspace/persistence/snapshot_writer.hpp"

#include <stdexcept>

namespace clearspace::persistence {

SnapshotWriter::SnapshotWriter(std::shared_ptr<ISnapshotStore> store, std::size_t keepLatest)
    : store_(std::move(store)), keepLatest_(keepLatest) {
  if (!store_) throw std::invalid_argument("SnapshotWriter requires a store");
  store_->init();
  worker_ = std::thread([this] { run_(); });
}

SnapshotWriter::~SnapshotWriter() {
  {
    std::lock_guard lk(m_);
    stop_ = true;
  }
  cv_.notify_all();
  if (worker_.joinable()) worker_.join();
}

void SnapshotWriter::enqueue(clearspace::core::PersistedBoardSnapshot snapshot) {
  pending_.fetch_add(1, std::memory_order_relaxed);
  {
    std::lock_guard lk(m_);
    q_.push(std::move(snapshot));
  }
  cv_.notify_one();
}

void SnapshotWriter::flush() {
  std::unique_lock lk(flushM_);
  flushCv_.wait(lk, [&] { return pending_.load(std::memory_order_relaxed) == 0; });
}

void SnapshotWriter::run_() {
  while (true) {
    clearspace::core::PersistedBoardSnapshot task;
    {
      std::unique_lock lk(m_);
      cv_.wait(lk, [&] { return stop_ || !q_.empty(); });
      if (stop_ && q_.empty()) break;
      task = std::move(q_.front());
      q_.pop();
    }

    try {
      store_->save(task.room, task.lastSeq, task.strokes);
      store_->pruneKeepLatest(task.room, keepLatest_);
    } catch (...) {
      // MVP: swallow errors (would be logged in production)
    }

    pending_.fetch_sub(1, std::memory_order_relaxed);
    flushCv_.notify_all();
  }
}

}
