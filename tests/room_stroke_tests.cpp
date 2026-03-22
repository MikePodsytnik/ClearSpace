#include <gtest/gtest.h>
#include <set>
#include <vector>

#include "clearspace/core/server_core.hpp"
#include "fake_time.hpp"

namespace clearspace::core {

static std::set<ClientId> toTargets(const std::vector<OutgoingEvent>& out) {
  std::set<ClientId> s;
  for (const auto& e : out) s.insert(e.to);
  return s;
}

TEST(RoomStroke, BeginPointEndHappyPath) {
  auto tp = std::make_shared<test::FakeTimeProvider>();
  ServerCore core(tp);

  RoomId room = core.createRoom();
  ASSERT_TRUE(core.joinRoom(1, room));
  ASSERT_TRUE(core.joinRoom(2, room));

  StrokeBegin b{Color::Red, Thickness::Medium, Point{1.0f, 2.0f}};
  auto out1 = core.handleEvent(1, room, EventPayload{b});
  ASSERT_TRUE(out1);
  ASSERT_EQ(out1.value().size(), 1u);
  ASSERT_EQ(toTargets(out1.value()), (std::set<ClientId>{2}));
  ASSERT_EQ(out1.value()[0].seq, 1u);

  auto out2 = core.handleEvent(1, room, EventPayload{StrokePoint{Point{3.0f, 4.0f}}});
  ASSERT_TRUE(out2);
  ASSERT_EQ(out2.value()[0].seq, 2u);

  auto out3 = core.handleEvent(1, room, EventPayload{StrokeEnd{}});
  ASSERT_TRUE(out3);
  ASSERT_EQ(out3.value()[0].seq, 3u);

  auto snap = core.joinRoom(3, room);
  ASSERT_TRUE(snap);
  ASSERT_EQ(snap.value().state.strokes.size(), 1u);
  ASSERT_EQ(snap.value().state.strokes[0].points.size(), 2u);
  ASSERT_TRUE(snap.value().state.strokes[0].finished);
}

TEST(RoomStroke, StrokeOrderFixedOnBegin) {
  auto tp = std::make_shared<test::FakeTimeProvider>();
  ServerCore core(tp);

  RoomId room = core.createRoom();
  ASSERT_TRUE(core.joinRoom(1, room));
  ASSERT_TRUE(core.joinRoom(2, room));
  ASSERT_TRUE(core.joinRoom(3, room));

  ASSERT_TRUE(core.handleEvent(1, room, EventPayload{StrokeBegin{Color::Black, Thickness::Thin, Point{0, 0}}}));
  ASSERT_TRUE(core.handleEvent(2, room, EventPayload{StrokeBegin{Color::Blue, Thickness::Thin, Point{10, 10}}}));

  ASSERT_TRUE(core.handleEvent(1, room, EventPayload{StrokePoint{Point{1, 1}}}));
  ASSERT_TRUE(core.handleEvent(2, room, EventPayload{StrokePoint{Point{11, 11}}}));

  auto snap = core.joinRoom(4, room);
  ASSERT_TRUE(snap);
  ASSERT_EQ(snap.value().state.strokes.size(), 2u);

  const auto& s0 = snap.value().state.strokes[0];
  const auto& s1 = snap.value().state.strokes[1];
  ASSERT_EQ(s0.strokeOrder, 1u);
  ASSERT_EQ(s1.strokeOrder, 2u);
}

TEST(RoomStroke, SingleEventBroadcastsSameSeqToAllRecipients) {
  auto tp = std::make_shared<test::FakeTimeProvider>();
  ServerCore core(tp);

  RoomId room = core.createRoom();
  ASSERT_TRUE(core.joinRoom(1, room));
  ASSERT_TRUE(core.joinRoom(2, room));
  ASSERT_TRUE(core.joinRoom(3, room));

  auto out = core.handleEvent(1, room, EventPayload{StrokeBegin{Color::Green, Thickness::Thick, Point{5, 6}}});
  ASSERT_TRUE(out);
  ASSERT_EQ(out.value().size(), 2u);
  ASSERT_EQ(out.value()[0].seq, out.value()[1].seq);
  ASSERT_EQ(toTargets(out.value()), (std::set<ClientId>{2, 3}));
}

}
