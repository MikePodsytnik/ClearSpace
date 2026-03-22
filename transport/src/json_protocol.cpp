#include "clearspace/transport/json_protocol.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>

#include <nlohmann/json.hpp>

namespace clearspace::transport {
namespace {

using json = nlohmann::json;

ProtocolError invalidFieldType(const std::string& key) {
  return ProtocolError::invalidFieldType("invalid field type: " + key);
}

template <class T>
Expected<T, ProtocolError> getRequired(const json& object, const char* key) {
  if (!object.contains(key)) {
    return ProtocolError::missingField("missing field: " + std::string(key));
  }

  try {
    return object.at(key).get<T>();
  } catch (const json::exception&) {
    return invalidFieldType(key);
  }
}

Expected<PointDto, ProtocolError> getRequiredPoint(const json& object, const char* key) {
  if (!object.contains(key)) {
    return ProtocolError::missingField("missing field: " + std::string(key));
  }
  if (!object.at(key).is_object()) {
    return invalidFieldType(key);
  }

  const auto& pointObject = object.at(key);
  auto x = getRequired<float>(pointObject, "x");
  if (!x) {
    return x.error();
  }
  auto y = getRequired<float>(pointObject, "y");
  if (!y) {
    return y.error();
  }
  return PointDto{.x = x.value(), .y = y.value()};
}

json makePointJson(const PointDto& point) {
  return json{{"x", point.x}, {"y", point.y}};
}

json makeStrokeJson(const StrokeDto& stroke) {
  json points = json::array();
  for (const auto& point : stroke.points) {
    points.push_back(makePointJson(point));
  }

  return json{
      {"id", stroke.id},
      {"author", stroke.author},
      {"color", stroke.color},
      {"thickness", stroke.thickness},
      {"stroke_order", stroke.strokeOrder},
      {"points", std::move(points)},
      {"finished", stroke.finished},
  };
}

json makeCursorJson(const CursorDto& cursor) {
  return json{{"client", cursor.client}, {"point", makePointJson(cursor.point)}};
}

}  // namespace

Expected<ClientMessage, ProtocolError> parseClientMessage(std::string_view line) {
  json object;
  try {
    object = json::parse(line.begin(), line.end());
  } catch (const json::exception& ex) {
    return ProtocolError::invalidJson(ex.what());
  }

  if (!object.is_object()) {
    return ProtocolError::invalidJson("root json value must be an object");
  }

  auto type = getRequired<std::string>(object, "type");
  if (!type) {
    return type.error();
  }

  if (type.value() == "ping") {
    return ClientMessage{PingRequest{}};
  }
  if (type.value() == "create_room") {
    return ClientMessage{CreateRoomRequest{}};
  }
  if (type.value() == "join_room") {
    auto room = getRequired<RoomId>(object, "room");
    if (!room) {
      return room.error();
    }
    return ClientMessage{JoinRoomRequest{.room = room.value()}};
  }
  if (type.value() == "leave_room") {
    auto room = getRequired<RoomId>(object, "room");
    if (!room) {
      return room.error();
    }
    return ClientMessage{LeaveRoomRequest{.room = room.value()}};
  }
  if (type.value() == "stroke_begin") {
    auto room = getRequired<RoomId>(object, "room");
    if (!room) {
      return room.error();
    }
    auto color = getRequired<unsigned int>(object, "color");
    if (!color) {
      return color.error();
    }
    auto thickness = getRequired<unsigned int>(object, "thickness");
    if (!thickness) {
      return thickness.error();
    }
    auto start = getRequiredPoint(object, "start");
    if (!start) {
      return start.error();
    }
    if (color.value() > 255 || thickness.value() > 255) {
      return ProtocolError::invalidFieldType("color or thickness is out of range");
    }
    return ClientMessage{StrokeBeginRequest{
        .room = room.value(),
        .color = static_cast<std::uint8_t>(color.value()),
        .thickness = static_cast<std::uint8_t>(thickness.value()),
        .start = start.value(),
    }};
  }
  if (type.value() == "stroke_point") {
    auto room = getRequired<RoomId>(object, "room");
    if (!room) {
      return room.error();
    }
    auto point = getRequiredPoint(object, "point");
    if (!point) {
      return point.error();
    }
    return ClientMessage{StrokePointRequest{.room = room.value(), .point = point.value()}};
  }
  if (type.value() == "stroke_end") {
    auto room = getRequired<RoomId>(object, "room");
    if (!room) {
      return room.error();
    }
    return ClientMessage{StrokeEndRequest{.room = room.value()}};
  }
  if (type.value() == "cursor_move") {
    auto room = getRequired<RoomId>(object, "room");
    if (!room) {
      return room.error();
    }
    auto point = getRequiredPoint(object, "point");
    if (!point) {
      return point.error();
    }
    return ClientMessage{CursorMoveRequest{.room = room.value(), .point = point.value()}};
  }
  if (type.value() == "clear") {
    auto room = getRequired<RoomId>(object, "room");
    if (!room) {
      return room.error();
    }
    return ClientMessage{ClearRequest{.room = room.value()}};
  }

  return ProtocolError::unknownMessageType("unknown message type: " + type.value());
}

std::string serializeServerMessage(const ServerMessage& msg) {
  json object;

  std::visit(
      [&](const auto& payload) {
        using T = std::decay_t<decltype(payload)>;
        if constexpr (std::is_same_v<T, PongResponse>) {
          object = json{{"type", "pong"}};
        } else if constexpr (std::is_same_v<T, RoomCreatedResponse>) {
          object = json{{"type", "room_created"}, {"room", payload.room}};
        } else if constexpr (std::is_same_v<T, OkResponse>) {
          object = json{{"type", "ok"}, {"request", payload.request}, {"room", payload.room}};
        } else if constexpr (std::is_same_v<T, ErrorResponse>) {
          object = json{{"type", "error"}, {"code", payload.code}, {"message", payload.message}};
        } else if constexpr (std::is_same_v<T, RoomSnapshotResponse>) {
          json members = json::array();
          for (ClientId client : payload.members) {
            members.push_back(client);
          }

          json strokes = json::array();
          for (const auto& stroke : payload.strokes) {
            strokes.push_back(makeStrokeJson(stroke));
          }

          json cursors = json::array();
          for (const auto& cursor : payload.cursors) {
            cursors.push_back(makeCursorJson(cursor));
          }

          object = json{
              {"type", "room_snapshot"},
              {"room", payload.room},
              {"last_seq", payload.lastSeq},
              {"members", std::move(members)},
              {"strokes", std::move(strokes)},
              {"cursors", std::move(cursors)},
          };
        } else if constexpr (std::is_same_v<T, StrokeBeginEvent>) {
          object = json{{"type", "stroke_begin"},
                        {"room", payload.room},
                        {"seq", payload.seq},
                        {"author", payload.author},
                        {"id", payload.id},
                        {"stroke_order", payload.strokeOrder},
                        {"color", payload.color},
                        {"thickness", payload.thickness},
                        {"start", makePointJson(payload.start)}};
        } else if constexpr (std::is_same_v<T, StrokePointEvent>) {
          object = json{{"type", "stroke_point"},
                        {"room", payload.room},
                        {"seq", payload.seq},
                        {"author", payload.author},
                        {"id", payload.id},
                        {"point", makePointJson(payload.point)}};
        } else if constexpr (std::is_same_v<T, StrokeEndEvent>) {
          object = json{{"type", "stroke_end"},
                        {"room", payload.room},
                        {"seq", payload.seq},
                        {"author", payload.author},
                        {"id", payload.id}};
        } else if constexpr (std::is_same_v<T, CursorMoveEvent>) {
          object = json{{"type", "cursor_move"},
                        {"room", payload.room},
                        {"seq", payload.seq},
                        {"author", payload.author},
                        {"point", makePointJson(payload.point)}};
        } else if constexpr (std::is_same_v<T, ClearEvent>) {
          object = json{{"type", "clear"}, {"room", payload.room}, {"seq", payload.seq}, {"author", payload.author}};
        } else if constexpr (std::is_same_v<T, MemberLeftEvent>) {
          object = json{{"type", "member_left"},
                        {"room", payload.room},
                        {"seq", payload.seq},
                        {"client", payload.client}};
        }
      },
      msg);

  return object.dump();
}

}  // namespace clearspace::transport
