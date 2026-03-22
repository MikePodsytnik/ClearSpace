#include <gtest/gtest.h>
#include "clearspace/core/server_core.hpp"
#include "fake_time.hpp"

namespace clearspace::core {

TEST(RoomOrder, SeqMonotonicNoGapsOnAppliedEvents) {
  auto tp = std::make_shared<test::FakeTimeProvider>();
  ServerCore core(tp);
  RoomId room = core.createRoom();
  ASSERT_TRUE(core.joinRoom(1, room));
  ASSERT_TRUE(core.joinRoom(2, room));

  auto a = core.handleEvent(1, room, EventPayload{StrokeBegin{Color::Black, Thickness::Thin, Point{0, 0}}});
  ASSERT_TRUE(a);
  ASSERT_EQ(a.value()[0].seq, 1u);

  auto b = core.handleEvent(1, room, EventPayload{StrokePoint{Point{1, 1}}});
  ASSERT_TRUE(b);
  ASSERT_EQ(b.value()[0].seq, 2u);

  auto c = core.handleEvent(1, room, EventPayload{StrokeEnd{}});
  ASSERT_TRUE(c);
  ASSERT_EQ(c.value()[0].seq, 3u);
}

TEST(RoomOrder, SeqIndependentAcrossRooms) {
  auto tp = std::make_shared<test::FakeTimeProvider>();
  ServerCore core(tp);
  RoomId r1 = core.createRoom();
  RoomId r2 = core.createRoom();

  ASSERT_TRUE(core.joinRoom(1, r1));
  ASSERT_TRUE(core.joinRoom(2, r1));
  ASSERT_TRUE(core.joinRoom(10, r2));
  ASSERT_TRUE(core.joinRoom(11, r2));

  auto a = core.handleEvent(1, r1, EventPayload{StrokeBegin{Color::Black, Thickness::Thin, Point{0, 0}}});
  ASSERT_TRUE(a);
  ASSERT_EQ(a.value()[0].seq, 1u);

  auto b = core.handleEvent(10, r2, EventPayload{StrokeBegin{Color::Black, Thickness::Thin, Point{0, 0}}});
  ASSERT_TRUE(b);
  ASSERT_EQ(b.value()[0].seq, 1u);
}

}
