#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <QMetaType>
#include <QPointF>
#include <QString>

#include "clearspace/transport/expected.hpp"

namespace clearspace::client {

using RoomId = std::uint64_t;
using ClientId = std::uint64_t;
using StrokeId = std::uint64_t;
using Seq = std::uint64_t;

struct CursorData {
  ClientId client{};
  QPointF point;
};

struct StrokeData {
  StrokeId id{};
  ClientId author{};
  std::uint8_t color{};
  std::uint8_t thickness{};
  Seq strokeOrder{};
  std::vector<QPointF> points;
  bool finished{false};
};

struct RoomCreatedData {
  RoomId room{};
};

struct OkData {
  QString request;
  RoomId room{};
};

struct ErrorData {
  QString code;
  QString message;
};

struct SnapshotData {
  RoomId room{};
  Seq lastSeq{};
  std::vector<ClientId> members;
  std::vector<StrokeData> strokes;
  std::vector<CursorData> cursors;
};

struct StrokeBeginData {
  RoomId room{};
  Seq seq{};
  ClientId author{};
  StrokeId id{};
  Seq strokeOrder{};
  std::uint8_t color{};
  std::uint8_t thickness{};
  QPointF start;
};

struct StrokePointData {
  RoomId room{};
  Seq seq{};
  ClientId author{};
  StrokeId id{};
  QPointF point;
};

struct StrokeEndData {
  RoomId room{};
  Seq seq{};
  ClientId author{};
  StrokeId id{};
};

struct CursorMoveData {
  RoomId room{};
  Seq seq{};
  ClientId author{};
  QPointF point;
};

struct ClearData {
  RoomId room{};
  Seq seq{};
  ClientId author{};
};

struct MemberLeftData {
  RoomId room{};
  Seq seq{};
  ClientId client{};
};

using ServerMessage = std::variant<
    std::monostate,
    RoomCreatedData,
    OkData,
    ErrorData,
    SnapshotData,
    StrokeBeginData,
    StrokePointData,
    StrokeEndData,
    CursorMoveData,
    ClearData,
    MemberLeftData>;

clearspace::transport::Expected<ServerMessage, QString> parseServerMessage(std::string_view line);

std::string makePingRequest();
std::string makeCreateRoomRequest();
std::string makeJoinRoomRequest(RoomId room);
std::string makeLeaveRoomRequest(RoomId room);
std::string makeStrokeBeginRequest(RoomId room, std::uint8_t color, std::uint8_t thickness, const QPointF& start);
std::string makeStrokePointRequest(RoomId room, const QPointF& point);
std::string makeStrokeEndRequest(RoomId room);
std::string makeCursorMoveRequest(RoomId room, const QPointF& point);
std::string makeClearRequest(RoomId room);

}

Q_DECLARE_METATYPE(clearspace::client::RoomCreatedData)
Q_DECLARE_METATYPE(clearspace::client::OkData)
Q_DECLARE_METATYPE(clearspace::client::ErrorData)
Q_DECLARE_METATYPE(clearspace::client::SnapshotData)
Q_DECLARE_METATYPE(clearspace::client::StrokeBeginData)
Q_DECLARE_METATYPE(clearspace::client::StrokePointData)
Q_DECLARE_METATYPE(clearspace::client::StrokeEndData)
Q_DECLARE_METATYPE(clearspace::client::CursorMoveData)
Q_DECLARE_METATYPE(clearspace::client::ClearData)
Q_DECLARE_METATYPE(clearspace::client::MemberLeftData)
