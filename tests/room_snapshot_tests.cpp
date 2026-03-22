#include <gtest/gtest.h>
#include "clearspace/core/server_core.hpp"
#include "fake_time.hpp"

namespace clearspace::core {

TEST(RoomSnapshot, JoinAfterDrawingGetsFullState) {
  auto tp = std::make_shared<test::FakeTimeProvider>();
  ServerCore core(tp);
  RoomId room = core.createRoom();
  ASSERT_TRUE(core.joinRoom(1, room));
  ASSERT_TRUE(core.joinRoom(2, room));

  ASSERT_TRUE(core.handleEvent(1, room, EventPayload{StrokeBegin{Color::Red, Thickness::Thin, Point{1, 1}}}));
  ASSERT_TRUE(core.handleEvent(1, room, EventPayload{StrokePoint{Point{2, 2}}}));
  ASSERT_TRUE(core.handleEvent(1, room, EventPayload{StrokeEnd{}}));

  auto snap = core.joinRoom(3, room);
  ASSERT_TRUE(snap);
  ASSERT_EQ(snap.value().lastSeq, 3u);
  ASSERT_EQ(snap.value().state.strokes.size(), 1u);
  ASSERT_TRUE(snap.value().state.strokes[0].finished);
}

TEST(RoomSnapshot, JoinAfterClearGetsEmptyBoard) {
  auto tp = std::make_shared<test::FakeTimeProvider>();
  ServerCore core(tp);
  RoomId room = core.createRoom();
  ASSERT_TRUE(core.joinRoom(1, room));
  ASSERT_TRUE(core.joinRoom(2, room));

  ASSERT_TRUE(core.handleEvent(1, room, EventPayload{StrokeBegin{Color::Red, Thickness::Thin, Point{1, 1}}}));
  ASSERT_TRUE(core.handleEvent(1, room, EventPayload{StrokeEnd{}}));
  ASSERT_TRUE(core.handleEvent(2, room, EventPayload{Clear{}}));

  auto snap = core.joinRoom(3, room);
  ASSERT_TRUE(snap);
  ASSERT_TRUE(snap.value().state.strokes.empty());
}

}
