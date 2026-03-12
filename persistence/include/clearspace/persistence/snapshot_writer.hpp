#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

#include "clearspace/core/snapshot_sink.hpp"
#include "snapshot_store.hpp"

namespace clearspace::persistence {

class SnapshotWriter final : public clearspace::core::ISnapshotSink {
 public:
  SnapshotWriter(std::shared_ptr<ISnapshotStore> store, std::size_t keepLatest = 10);
  ~SnapshotWriter() override;

  SnapshotWriter(const SnapshotWriter&) = delete;
  SnapshotWriter& operator=(const SnapshotWriter&) = delete;

  void enqueue(clearspace::core::PersistedBoardSnapshot snapshot) override;

  void flush();

 private:
  void run_();

  std::shared_ptr<ISnapshotStore> store_;
  std::size_t keepLatest_{10};

  std::mutex m_;
  std::condition_variable cv_;
  std::queue<clearspace::core::PersistedBoardSnapshot> q_;
  bool stop_{false};

  std::thread worker_;

  std::atomic<std::uint64_t> pending_{0};

  std::mutex flushM_;
  std::condition_variable flushCv_;
};

}
