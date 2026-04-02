#pragma once

#include <string>
#include <utility>

namespace clearspace::transport {

enum class ProtocolErrorCode {
  InvalidJson,
  MissingField,
  InvalidFieldType,
  UnknownMessageType,
  InvalidMessage,
};

struct ProtocolError {
  ProtocolErrorCode code{ProtocolErrorCode::InvalidMessage};
  std::string message;

  static ProtocolError invalidJson(std::string msg = "invalid json") {
    return ProtocolError{ProtocolErrorCode::InvalidJson, std::move(msg)};
  }

  static ProtocolError missingField(std::string msg) {
    return ProtocolError{ProtocolErrorCode::MissingField, std::move(msg)};
  }

  static ProtocolError invalidFieldType(std::string msg) {
    return ProtocolError{ProtocolErrorCode::InvalidFieldType, std::move(msg)};
  }

  static ProtocolError unknownMessageType(std::string msg) {
    return ProtocolError{ProtocolErrorCode::UnknownMessageType, std::move(msg)};
  }

  static ProtocolError invalidMessage(std::string msg) {
    return ProtocolError{ProtocolErrorCode::InvalidMessage, std::move(msg)};
  }
};

}
