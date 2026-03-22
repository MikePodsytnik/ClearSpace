#pragma once

#include <variant>

#include "drawing.hpp"
#include "geometry.hpp"
#include "types.hpp"

namespace clearspace::core {

struct StrokeBegin { Color color{}; Thickness thickness{}; Point start{}; };
struct StrokePoint { Point p{}; };
struct StrokeEnd {};
struct CursorMove { Point p{}; };
struct Clear {};

using EventPayload = std::variant<StrokeBegin, StrokePoint, StrokeEnd, CursorMove, Clear>;

struct StrokeBeginOut { ClientId author{}; StrokeId id{}; Seq strokeOrder{}; Color color{}; Thickness thickness{}; Point start{}; };
struct StrokePointOut { ClientId author{}; StrokeId id{}; Point p{}; };
struct StrokeEndOut { ClientId author{}; StrokeId id{}; };
struct CursorMoveOut { ClientId author{}; Point p{}; };
struct ClearOut { ClientId author{}; };
struct MemberLeftOut { ClientId client{}; };

using ServerEventPayload = std::variant<StrokeBeginOut, StrokePointOut, StrokeEndOut, CursorMoveOut, ClearOut, MemberLeftOut>;

struct OutgoingEvent { RoomId room{}; ClientId to{}; Seq seq{}; ServerEventPayload payload{}; };

}
