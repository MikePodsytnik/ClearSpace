#include "clearspace/transport/mappers.hpp"

#include <algorithm>

#include "clearspace/core/drawing.hpp"

namespace clearspace::transport {
clearspace::core::Expected<clearspace::core::Color, clearspace::core::Error> toColor(std::uint8_t color) {
  if (color > static_cast<std::uint8_t>(clearspace::core::Color::Blue)) {
    return clearspace::core::Error::invalidParams("invalid color");
  }
  return static_cast<clearspace::core::Color>(color);
}

clearspace::core::Expected<clearspace::core::Thickness, clearspace::core::Error> toThickness(std::uint8_t thickness) {
  if (thickness > static_cast<std::uint8_t>(clearspace::core::Thickness::Thick)) {
    return clearspace::core::Error::invalidParams("invalid thickness");
  }
  return static_cast<clearspace::core::Thickness>(thickness);
}

PointDto fromCorePoint(const clearspace::core::Point& point) {
  return PointDto{.x = point.x, .y = point.y};
}

StrokeDto fromCoreStroke(const clearspace::core::Stroke& stroke) {
  StrokeDto dto;
  dto.id = stroke.id;
  dto.author = stroke.author;
  dto.color = static_cast<std::uint8_t>(stroke.color);
  dto.thickness = static_cast<std::uint8_t>(stroke.thickness);
  dto.strokeOrder = stroke.strokeOrder;
  dto.finished = stroke.finished;
  dto.points.reserve(stroke.points.size());
  for (const auto& point : stroke.points) {
    dto.points.push_back(fromCorePoint(point));
  }
  return dto;
}


clearspace::core::Expected<clearspace::core::EventPayload, clearspace::core::Error> toCoreEvent(
    const StrokeBeginRequest& request) {
  auto color = toColor(request.color);
  if (!color) {
    return color.error();
  }
  auto thickness = toThickness(request.thickness);
  if (!thickness) {
    return thickness.error();
  }
  return clearspace::core::EventPayload{clearspace::core::StrokeBegin{
      .color = color.value(),
      .thickness = thickness.value(),
      .start = clearspace::core::Point{.x = request.start.x, .y = request.start.y},
  }};
}

clearspace::core::Expected<clearspace::core::EventPayload, clearspace::core::Error> toCoreEvent(
    const StrokePointRequest& request) {
  return clearspace::core::EventPayload{clearspace::core::StrokePoint{.p = clearspace::core::Point{.x = request.point.x, .y = request.point.y}}};
}

clearspace::core::Expected<clearspace::core::EventPayload, clearspace::core::Error> toCoreEvent(
    const StrokeEndRequest&) {
  return clearspace::core::EventPayload{clearspace::core::StrokeEnd{}};
}

clearspace::core::Expected<clearspace::core::EventPayload, clearspace::core::Error> toCoreEvent(
    const CursorMoveRequest& request) {
  return clearspace::core::EventPayload{clearspace::core::CursorMove{.p = clearspace::core::Point{.x = request.point.x, .y = request.point.y}}};
}

clearspace::core::Expected<clearspace::core::EventPayload, clearspace::core::Error> toCoreEvent(
    const ClearRequest&) {
  return clearspace::core::EventPayload{clearspace::core::Clear{}};
}

std::string errorCodeToString(clearspace::core::ErrorCode code) {
  switch (code) {
    case clearspace::core::ErrorCode::RoomNotFound:
      return "room_not_found";
    case clearspace::core::ErrorCode::NotInRoom:
      return "not_in_room";
    case clearspace::core::ErrorCode::ProtocolViolation:
      return "protocol_violation";
    case clearspace::core::ErrorCode::InvalidParams:
      return "invalid_params";
  }
  return "invalid_params";
}

ServerMessage toErrorMessage(const clearspace::core::Error& error) {
  return ErrorResponse{.code = errorCodeToString(error.code), .message = error.message};
}

ServerMessage toSnapshotMessage(const clearspace::core::RoomSnapshot& snapshot) {
  RoomSnapshotResponse response;
  response.room = snapshot.room;
  response.lastSeq = snapshot.lastSeq;
  response.members = snapshot.members;
  response.strokes.reserve(snapshot.state.strokes.size());
  for (const auto& stroke : snapshot.state.strokes) {
    response.strokes.push_back(fromCoreStroke(stroke));
  }
  response.cursors.reserve(snapshot.state.cursors.size());
  for (const auto& [client, point] : snapshot.state.cursors) {
    response.cursors.push_back(CursorDto{.client = client, .point = fromCorePoint(point)});
  }
  std::sort(response.members.begin(), response.members.end());
  std::sort(response.cursors.begin(), response.cursors.end(), [](const CursorDto& lhs, const CursorDto& rhs) {
    return lhs.client < rhs.client;
  });
  return response;
}

ServerMessage toServerMessage(const clearspace::core::OutgoingEvent& ev) {
  return std::visit(
      [&](const auto& payload) -> ServerMessage {
        using T = std::decay_t<decltype(payload)>;
        if constexpr (std::is_same_v<T, clearspace::core::StrokeBeginOut>) {
          return StrokeBeginEvent{
              .room = ev.room,
              .seq = ev.seq,
              .author = payload.author,
              .id = payload.id,
              .strokeOrder = payload.strokeOrder,
              .color = static_cast<std::uint8_t>(payload.color),
              .thickness = static_cast<std::uint8_t>(payload.thickness),
              .start = fromCorePoint(payload.start),
          };
        } else if constexpr (std::is_same_v<T, clearspace::core::StrokePointOut>) {
          return StrokePointEvent{
              .room = ev.room,
              .seq = ev.seq,
              .author = payload.author,
              .id = payload.id,
              .point = fromCorePoint(payload.p),
          };
        } else if constexpr (std::is_same_v<T, clearspace::core::StrokeEndOut>) {
          return StrokeEndEvent{.room = ev.room, .seq = ev.seq, .author = payload.author, .id = payload.id};
        } else if constexpr (std::is_same_v<T, clearspace::core::CursorMoveOut>) {
          return CursorMoveEvent{
              .room = ev.room,
              .seq = ev.seq,
              .author = payload.author,
              .point = fromCorePoint(payload.p),
          };
        } else if constexpr (std::is_same_v<T, clearspace::core::ClearOut>) {
          return ClearEvent{.room = ev.room, .seq = ev.seq, .author = payload.author};
        } else {
          return MemberLeftEvent{.room = ev.room, .seq = ev.seq, .client = payload.client};
        }
      },
      ev.payload);
}

}
