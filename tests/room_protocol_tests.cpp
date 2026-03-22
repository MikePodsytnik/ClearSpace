#include <gtest/gtest.h>
#include "clearspace/core/server_core.hpp"
#include "fake_time.hpp"

namespace clearspace::core {

TEST(RoomProtocol, PointWithoutBeginIsProtocolViolation) {
  auto tp = std::make_shared<test::FakeTimeProvider>();
  ServerCore core(tp);
  RoomId room = core.createRoom();
  ASSERT_TRUE(core.joinRoom(1, room));
  ASSERT_TRUE(core.joinRoom(2, room));

  auto r = core.handleEvent(1, room, EventPayload{StrokePoint{Point{1, 1}}});
  ASSERT_FALSE(r);
  ASSERT_EQ(r.error().code, ErrorCode::ProtocolViolation);
}

TEST(RoomProtocol, EndWithoutBeginIsProtocolViolation) {
  auto tp = std::make_shared<test::FakeTimeProvider>();
  ServerCore core(tp);
  RoomId room = core.createRoom();
  ASSERT_TRUE(core.joinRoom(1, room));
  ASSERT_TRUE(core.joinRoom(2, room));

  auto r = core.handleEvent(1, room, EventPayload{StrokeEnd{}});
  ASSERT_FALSE(r);
  ASSERT_EQ(r.error().code, ErrorCode::ProtocolViolation);
}

TEST(RoomProtocol, BeginWhileActiveIsProtocolViolation) {
  auto tp = std::make_shared<test::FakeTimeProvider>();
  ServerCore core(tp);
  RoomId room = core.createRoom();
  ASSERT_TRUE(core.joinRoom(1, room));
  ASSERT_TRUE(core.joinRoom(2, room));

  ASSERT_TRUE(core.handleEvent(1, room, EventPayload{StrokeBegin{Color::Black, Thickness::Thin, Point{0, 0}}}));
  auto r = core.handleEvent(1, room, EventPayload{StrokeBegin{Color::Black, Thickness::Thin, Point{1, 1}}});
  ASSERT_FALSE(r);
  ASSERT_EQ(r.error().code, ErrorCode::ProtocolViolation);
}

}
