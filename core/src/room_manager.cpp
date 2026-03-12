#include "clearspace/core/room_manager.hpp"

namespace clearspace::core {

RoomManager::RoomManager(std::shared_ptr<ITimeProvider> timeProvider)
    : timeProvider_(std::move(timeProvider)) {}

RoomId RoomManager::createRoom() {
  std::scoped_lock lock(mutex_);
  RoomId id = nextRoomId_++;
  rooms_[id] = std::make_shared<Room>(id, timeProvider_);
  return id;
}

std::shared_ptr<Room> RoomManager::getRoom(RoomId room) {
  std::scoped_lock lock(mutex_);
  auto it = rooms_.find(room);
  if (it == rooms_.end()) {
    return {};
  }
  return it->second;
}

}
