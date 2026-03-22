#pragma once

#include <memory>
#include <vector>

#include "errors.hpp"
#include "events.hpp"
#include "expected.hpp"
#include "room_manager.hpp"
#include "snapshot.hpp"
#include "snapshot_sink.hpp"
#include "time.hpp"
#include "types.hpp"

namespace clearspace::core {

class ServerCore final {
 public:
  explicit ServerCore(std::shared_ptr<ITimeProvider> timeProvider = makeSystemTimeProvider(),
                      std::shared_ptr<ISnapshotSink> snapshotSink = {});

  RoomId createRoom();
  Expected<RoomSnapshot, Error> joinRoom(ClientId client, RoomId room);
  Expected<std::vector<OutgoingEvent>, Error> leaveRoom(ClientId client, RoomId room);
  Expected<std::vector<OutgoingEvent>, Error> handleEvent(ClientId client, RoomId room, const EventPayload& ev);

 private:
  std::shared_ptr<ITimeProvider> timeProvider_;
  std::shared_ptr<ISnapshotSink> snapshotSink_;
  RoomManager rooms_;
};

}
