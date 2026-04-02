#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "clearspace/transport/json_protocol.hpp"

using namespace clearspace::transport;

TEST(TransportProtocolTests, ParsesPingMessage) {
  auto parsed = parseClientMessage(R"({"type":"ping"})");
  ASSERT_TRUE(parsed.has_value());
  EXPECT_TRUE(std::holds_alternative<PingRequest>(parsed.value()));
}

TEST(TransportProtocolTests, ParsesStrokeBeginMessage) {
  auto parsed = parseClientMessage(
      R"({"type":"stroke_begin","room":7,"color":1,"thickness":2,"start":{"x":1.5,"y":2.5}})");
  ASSERT_TRUE(parsed.has_value());
  ASSERT_TRUE(std::holds_alternative<StrokeBeginRequest>(parsed.value()));
  const auto& request = std::get<StrokeBeginRequest>(parsed.value());
  EXPECT_EQ(request.room, 7);
  EXPECT_EQ(request.color, 1);
  EXPECT_EQ(request.thickness, 2);
  EXPECT_FLOAT_EQ(request.start.x, 1.5f);
  EXPECT_FLOAT_EQ(request.start.y, 2.5f);
}

TEST(TransportProtocolTests, RejectsUnknownMessageType) {
  auto parsed = parseClientMessage(R"({"type":"oops"})");
  ASSERT_FALSE(parsed.has_value());
  EXPECT_EQ(parsed.error().code, ProtocolErrorCode::UnknownMessageType);
}

TEST(TransportProtocolTests, SerializesRoomCreatedResponse) {
  const auto jsonText = serializeServerMessage(RoomCreatedResponse{.room = 42});
  const auto json = nlohmann::json::parse(jsonText);
  EXPECT_EQ(json.at("type").get<std::string>(), "room_created");
  EXPECT_EQ(json.at("room").get<std::uint64_t>(), 42U);
}
