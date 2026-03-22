#pragma once

#include <string>
#include <utility>

namespace clearspace::core {

enum class ErrorCode { RoomNotFound, NotInRoom, ProtocolViolation, InvalidParams };

struct Error {
  ErrorCode code{ErrorCode::InvalidParams};
  std::string message;

  static Error roomNotFound(std::string msg = "room not found") {
    return Error{ErrorCode::RoomNotFound, std::move(msg)};
  }
  static Error notInRoom(std::string msg = "client is not in room") {
    return Error{ErrorCode::NotInRoom, std::move(msg)};
  }
  static Error protocolViolation(std::string msg = "protocol violation") {
    return Error{ErrorCode::ProtocolViolation, std::move(msg)};
  }
  static Error invalidParams(std::string msg = "invalid params") {
    return Error{ErrorCode::InvalidParams, std::move(msg)};
  }
};

}
