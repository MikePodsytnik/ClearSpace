#include <gtest/gtest.h>
#include "clearspace/core/server_core.hpp"
#include "fake_time.hpp"

namespace clearspace::core {

TEST(RoomLeave, LeaveBroadcastsMemberLeftAndRemovesCursor) {
  auto tp = std::make_shared<test::FakeTimeProvider>();
  ServerCore core(tp);

  RoomId room = core.createRoom();
  ASSERT_TRUE(core.joinRoom(1, room));
  ASSERT_TRUE(core.joinRoom(2, room));

  tp->set(TimePoint{});
  ASSERT_TRUE(core.handleEvent(1, room, EventPayload{CursorMove{Point{10, 10}}}));

  auto out = core.leaveRoom(1, room);
  ASSERT_TRUE(out);
  ASSERT_EQ(out.value().size(), 1u);
  ASSERT_EQ(out.value()[0].to, 2u);
  ASSERT_TRUE(std::holds_alternative<MemberLeftOut>(out.value()[0].payload));

  auto snap = core.joinRoom(3, room);
  ASSERT_TRUE(snap);
  ASSERT_EQ(snap.value().state.cursors.count(1u), 0u);
}

}
