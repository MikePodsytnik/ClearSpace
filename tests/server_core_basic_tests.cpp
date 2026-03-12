#include <gtest/gtest.h>
#include <set>
#include <vector>

#include "clearspace/core/server_core.hpp"
#include "fake_time.hpp"

namespace clearspace::core {

static std::set<ClientId> toSet(const std::vector<ClientId>& v) {
  return std::set<ClientId>(v.begin(), v.end());
}

TEST(ServerCoreBasic, CreateRoomReturnsUniqueIds) {
  auto tp = std::make_shared<test::FakeTimeProvider>();
  ServerCore core(tp);
  RoomId a = core.createRoom();
  RoomId b = core.createRoom();
  ASSERT_NE(a, b);
}

TEST(ServerCoreBasic, JoinReturnsSnapshotWithMembers) {
  auto tp = std::make_shared<test::FakeTimeProvider>();
  ServerCore core(tp);

  RoomId room = core.createRoom();

  auto s1 = core.joinRoom(1, room);
  ASSERT_TRUE(s1);
  ASSERT_EQ(s1.value().lastSeq, 0u);
  ASSERT_EQ(toSet(s1.value().members), (std::set<ClientId>{1}));

  auto s2 = core.joinRoom(2, room);
  ASSERT_TRUE(s2);
  ASSERT_EQ(toSet(s2.value().members), (std::set<ClientId>{1, 2}));
}

TEST(ServerCoreBasic, UnknownRoomReturnsError) {
  auto tp = std::make_shared<test::FakeTimeProvider>();
  ServerCore core(tp);

  auto r = core.joinRoom(1, 999);
  ASSERT_FALSE(r);
  ASSERT_EQ(r.error().code, ErrorCode::RoomNotFound);
}

}
