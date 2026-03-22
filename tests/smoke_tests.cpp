#include <gtest/gtest.h>
#include "clearspace/core/server_core.hpp"
#include "fake_time.hpp"

namespace clearspace::core {

TEST(Smoke, CreateJoinAndCursorMove) {
  auto tp = std::make_shared<test::FakeTimeProvider>();
  ServerCore core(tp);

  RoomId room = core.createRoom();
  ASSERT_TRUE(core.joinRoom(1, room));
  ASSERT_TRUE(core.joinRoom(2, room));

  CursorMove mv{.p = Point{10.0f, 20.0f}};
  auto out = core.handleEvent(1, room, EventPayload{mv});
  ASSERT_TRUE(out);
}

}
