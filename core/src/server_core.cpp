#include "clearspace/core/server_core.hpp"

namespace clearspace::core {

ServerCore::ServerCore(std::shared_ptr<ITimeProvider> timeProvider, std::shared_ptr<ISnapshotSink> snapshotSink)
    : timeProvider_(std::move(timeProvider)), snapshotSink_(std::move(snapshotSink)), rooms_(timeProvider_) {}

RoomId ServerCore::createRoom() {
  return rooms_.createRoom();
}

Expected<RoomSnapshot, Error> ServerCore::joinRoom(ClientId client, RoomId room) {
  auto r = rooms_.getRoom(room);
  if (!r) {
    return Error::roomNotFound();
  }
  return r->join(client);
}

Expected<std::vector<OutgoingEvent>, Error> ServerCore::leaveRoom(ClientId client, RoomId room) {
  auto r = rooms_.getRoom(room);
  if (!r) {
    return Error::roomNotFound();
  }

  auto res = r->leave(client);
  if (!res) {
    return res;
  }

  if (snapshotSink_) {
    if (r->memberCount() == 0) {
      snapshotSink_->enqueue(r->makePersistedSnapshot());
    }
  }

  return res;
}

Expected<std::vector<OutgoingEvent>, Error> ServerCore::handleEvent(ClientId client, RoomId room, const EventPayload& ev) {
  auto r = rooms_.getRoom(room);
  if (!r) {
    return Error::roomNotFound();
  }

  auto res = r->apply(client, ev);
  if (!res) {
    return res;
  }

  if (snapshotSink_) {
    const bool shouldSave = std::holds_alternative<StrokeEnd>(ev) || std::holds_alternative<Clear>(ev);
    if (shouldSave) {
      snapshotSink_->enqueue(r->makePersistedSnapshot());
    }
  }

  return res;
}

}
