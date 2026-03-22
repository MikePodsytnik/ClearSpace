#pragma once

#include <string>
#include <string_view>

#include "expected.hpp"
#include "messages.hpp"
#include "protocol_error.hpp"

namespace clearspace::transport {

Expected<ClientMessage, ProtocolError> parseClientMessage(std::string_view line);
std::string serializeServerMessage(const ServerMessage& msg);

}  // namespace clearspace::transport
