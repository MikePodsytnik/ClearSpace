#pragma once

#include "clearspace/core/errors.hpp"
#include "clearspace/core/events.hpp"
#include "clearspace/core/expected.hpp"
#include "clearspace/core/snapshot.hpp"
#include "messages.hpp"

namespace clearspace::transport {

clearspace::core::Expected<clearspace::core::EventPayload, clearspace::core::Error> toCoreEvent(
    const StrokeBeginRequest& request);
clearspace::core::Expected<clearspace::core::EventPayload, clearspace::core::Error> toCoreEvent(
    const StrokePointRequest& request);
clearspace::core::Expected<clearspace::core::EventPayload, clearspace::core::Error> toCoreEvent(
    const StrokeEndRequest& request);
clearspace::core::Expected<clearspace::core::EventPayload, clearspace::core::Error> toCoreEvent(
    const CursorMoveRequest& request);
clearspace::core::Expected<clearspace::core::EventPayload, clearspace::core::Error> toCoreEvent(
    const ClearRequest& request);

ServerMessage toServerMessage(const clearspace::core::OutgoingEvent& ev);
ServerMessage toSnapshotMessage(const clearspace::core::RoomSnapshot& snapshot);
ServerMessage toErrorMessage(const clearspace::core::Error& error);

std::string errorCodeToString(clearspace::core::ErrorCode code);

}  // namespace clearspace::transport
