#include <gtest/gtest.h>

#include "clearspace/transport/mappers.hpp"

using namespace clearspace;

TEST(TransportMapperTests, ConvertsStrokeBeginRequestToCoreEvent) {
  transport::StrokeBeginRequest request{.room = 10, .color = 1, .thickness = 2, .start = {.x = 3.0f, .y = 4.0f}};
  auto event = transport::toCoreEvent(request);
  ASSERT_TRUE(event.has_value());
  ASSERT_TRUE(std::holds_alternative<core::StrokeBegin>(event.value()));
  const auto& payload = std::get<core::StrokeBegin>(event.value());
  EXPECT_EQ(payload.color, core::Color::Red);
  EXPECT_EQ(payload.thickness, core::Thickness::Thick);
  EXPECT_FLOAT_EQ(payload.start.x, 3.0f);
  EXPECT_FLOAT_EQ(payload.start.y, 4.0f);
}

TEST(TransportMapperTests, RejectsInvalidColor) {
  transport::StrokeBeginRequest request{.room = 10, .color = 100, .thickness = 1, .start = {.x = 3.0f, .y = 4.0f}};
  auto event = transport::toCoreEvent(request);
  ASSERT_FALSE(event.has_value());
  EXPECT_EQ(event.error().code, core::ErrorCode::InvalidParams);
}

TEST(TransportMapperTests, ConvertsSnapshotToServerMessage) {
  core::RoomSnapshot snapshot;
  snapshot.room = 9;
  snapshot.lastSeq = 3;
  snapshot.members = {2, 1};
  snapshot.state.cursors[5] = core::Point{.x = 1.0f, .y = 2.0f};

  core::Stroke stroke;
  stroke.id = 11;
  stroke.author = 2;
  stroke.color = core::Color::Blue;
  stroke.thickness = core::Thickness::Medium;
  stroke.strokeOrder = 7;
  stroke.points = {core::Point{.x = 1.0f, .y = 1.5f}};
  stroke.finished = true;
  snapshot.state.strokes.push_back(stroke);

  auto message = transport::toSnapshotMessage(snapshot);
  ASSERT_TRUE(std::holds_alternative<transport::RoomSnapshotResponse>(message));
  const auto& response = std::get<transport::RoomSnapshotResponse>(message);
  EXPECT_EQ(response.room, 9);
  EXPECT_EQ(response.lastSeq, 3);
  ASSERT_EQ(response.members.size(), 2);
  EXPECT_EQ(response.members[0], 1);
  EXPECT_EQ(response.members[1], 2);
  ASSERT_EQ(response.strokes.size(), 1);
  EXPECT_EQ(response.strokes[0].id, 11);
  ASSERT_EQ(response.cursors.size(), 1);
  EXPECT_EQ(response.cursors[0].client, 5);
}
