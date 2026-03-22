#include <gtest/gtest.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "clearspace/core/server_core.hpp"
#include "fake_time.hpp"

namespace clearspace::core {

TEST(Concurrency, ConcurrentCursorMovesFromManyClients) {
  auto tp = std::make_shared<test::FakeTimeProvider>();
  ServerCore core(tp);

  RoomId room = core.createRoom();
  constexpr int kClients = 8;
  for (int i = 1; i <= kClients; ++i) {
    ASSERT_TRUE(core.joinRoom(static_cast<ClientId>(i), room));
  }

  tp->set(TimePoint{});

  std::mutex m;
  std::condition_variable cv;
  bool start = false;

  std::atomic<int> okCount{0};
  std::vector<std::thread> threads;
  threads.reserve(kClients);

  for (int i = 1; i <= kClients; ++i) {
    threads.emplace_back([&, i] {
      {
        std::unique_lock lk(m);
        cv.wait(lk, [&] { return start; });
      }
      CursorMove mv{.p = Point{static_cast<float>(i), static_cast<float>(i * 10)}};
      auto r = core.handleEvent(static_cast<ClientId>(i), room, EventPayload{mv});
      if (r) okCount.fetch_add(1, std::memory_order_relaxed);
    });
  }

  {
    std::lock_guard lk(m);
    start = true;
  }
  cv.notify_all();

  for (auto& t : threads) t.join();

  ASSERT_EQ(okCount.load(), kClients);

  auto snap = core.joinRoom(100, room);
  ASSERT_TRUE(snap);
  ASSERT_EQ(static_cast<int>(snap.value().state.cursors.size()), kClients);
}

TEST(Concurrency, TwoClientsDrawConcurrently) {
  auto tp = std::make_shared<test::FakeTimeProvider>();
  ServerCore core(tp);

  RoomId room = core.createRoom();
  ASSERT_TRUE(core.joinRoom(1, room));
  ASSERT_TRUE(core.joinRoom(2, room));

  tp->set(TimePoint{});

  std::mutex m;
  std::condition_variable cv;
  bool start = false;

  constexpr int kExtraPoints = 200;

  auto draw = [&](ClientId client, float base) {
    {
      std::unique_lock lk(m);
      cv.wait(lk, [&] { return start; });
    }
    ASSERT_TRUE(core.handleEvent(client, room, EventPayload{StrokeBegin{Color::Black, Thickness::Thin, Point{base, base}}}));
    for (int i = 0; i < kExtraPoints; ++i) {
      ASSERT_TRUE(core.handleEvent(client, room, EventPayload{StrokePoint{Point{base + i + 1.0f, base}}}));
    }
    ASSERT_TRUE(core.handleEvent(client, room, EventPayload{StrokeEnd{}}));
  };

  std::thread t1(draw, 1, 0.0f);
  std::thread t2(draw, 2, 1000.0f);

  {
    std::lock_guard lk(m);
    start = true;
  }
  cv.notify_all();

  t1.join();
  t2.join();

  auto snap = core.joinRoom(3, room);
  ASSERT_TRUE(snap);
  ASSERT_EQ(snap.value().state.strokes.size(), 2u);

  std::unordered_map<ClientId, const Stroke*> byAuthor;
  for (const auto& s : snap.value().state.strokes) byAuthor[s.author] = &s;

  ASSERT_EQ(byAuthor.size(), 2u);
  ASSERT_TRUE(byAuthor.contains(1));
  ASSERT_TRUE(byAuthor.contains(2));

  for (ClientId c : {ClientId{1}, ClientId{2}}) {
    const Stroke* s = byAuthor[c];
    ASSERT_TRUE(s->finished);
    ASSERT_EQ(s->points.size(), static_cast<std::size_t>(1 + kExtraPoints));
  }

  const Seq expectedLastSeq = static_cast<Seq>(2 * (kExtraPoints + 2));
  ASSERT_EQ(snap.value().lastSeq, expectedLastSeq);
}

}
