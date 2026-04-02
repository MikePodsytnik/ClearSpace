#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace clearspace::transport {

using RoomId = std::uint64_t;
using ClientId = std::uint64_t;
using StrokeId = std::uint64_t;
using Seq = std::uint64_t;

struct PointDto {
  float x{};
  float y{};
};

struct CursorDto {
  ClientId client{};
  PointDto point{};
};

struct StrokeDto {
  StrokeId id{};
  ClientId author{};
  std::uint8_t color{};
  std::uint8_t thickness{};
  Seq strokeOrder{};
  std::vector<PointDto> points;
  bool finished{false};
};

struct PingRequest {};
struct CreateRoomRequest {};
struct JoinRoomRequest {
  RoomId room{};
};
struct LeaveRoomRequest {
  RoomId room{};
};
struct StrokeBeginRequest {
  RoomId room{};
  std::uint8_t color{};
  std::uint8_t thickness{};
  PointDto start{};
};
struct StrokePointRequest {
  RoomId room{};
  PointDto point{};
};
struct StrokeEndRequest {
  RoomId room{};
};
struct CursorMoveRequest {
  RoomId room{};
  PointDto point{};
};
struct ClearRequest {
  RoomId room{};
};

using ClientMessage = std::variant<
    PingRequest,
    CreateRoomRequest,
    JoinRoomRequest,
    LeaveRoomRequest,
    StrokeBeginRequest,
    StrokePointRequest,
    StrokeEndRequest,
    CursorMoveRequest,
    ClearRequest>;

struct PongResponse {};
struct RoomCreatedResponse {
  RoomId room{};
};
struct OkResponse {
  std::string request;
  RoomId room{};
};
struct ErrorResponse {
  std::string code;
  std::string message;
};
struct RoomSnapshotResponse {
  RoomId room{};
  Seq lastSeq{};
  std::vector<ClientId> members;
  std::vector<StrokeDto> strokes;
  std::vector<CursorDto> cursors;
};
struct StrokeBeginEvent {
  RoomId room{};
  Seq seq{};
  ClientId author{};
  StrokeId id{};
  Seq strokeOrder{};
  std::uint8_t color{};
  std::uint8_t thickness{};
  PointDto start{};
};
struct StrokePointEvent {
  RoomId room{};
  Seq seq{};
  ClientId author{};
  StrokeId id{};
  PointDto point{};
};
struct StrokeEndEvent {
  RoomId room{};
  Seq seq{};
  ClientId author{};
  StrokeId id{};
};
struct CursorMoveEvent {
  RoomId room{};
  Seq seq{};
  ClientId author{};
  PointDto point{};
};
struct ClearEvent {
  RoomId room{};
  Seq seq{};
  ClientId author{};
};
struct MemberLeftEvent {
  RoomId room{};
  Seq seq{};
  ClientId client{};
};

using ServerMessage = std::variant<
    PongResponse,
    RoomCreatedResponse,
    OkResponse,
    ErrorResponse,
    RoomSnapshotResponse,
    StrokeBeginEvent,
    StrokePointEvent,
    StrokeEndEvent,
    CursorMoveEvent,
    ClearEvent,
    MemberLeftEvent>;

}
