#include <gtest/gtest.h>

#include "clearspace/core/config.hpp"
#include "clearspace/core/server_core.hpp"
#include "fake_time.hpp"

namespace clearspace::core {

TEST(CursorRate, DropsTooFrequentMovesWithoutSeqIncrement) {
  auto tp = std::make_shared<test::FakeTimeProvider>();
  ServerCore core(tp);

  RoomId room = core.createRoom();
  ASSERT_TRUE(core.joinRoom(1, room));
  ASSERT_TRUE(core.joinRoom(2, room));

  tp->set(TimePoint{});

  auto a = core.handleEvent(1, room, EventPayload{CursorMove{Point{10, 10}}});
  ASSERT_TRUE(a);
  ASSERT_EQ(a.value()[0].seq, 1u);

  auto b = core.handleEvent(1, room, EventPayload{CursorMove{Point{11, 11}}});
  ASSERT_TRUE(b);
  ASSERT_TRUE(b.value().empty());

  auto c = core.handleEvent(1, room, EventPayload{StrokeBegin{Color::Black, Thickness::Thin, Point{0, 0}}});
  ASSERT_TRUE(c);
  ASSERT_EQ(c.value()[0].seq, 2u);

  tp->advance(config::kCursorMinInterval);
  auto d = core.handleEvent(1, room, EventPayload{CursorMove{Point{12, 12}}});
  ASSERT_TRUE(d);
  ASSERT_EQ(d.value()[0].seq, 3u);
}

}
