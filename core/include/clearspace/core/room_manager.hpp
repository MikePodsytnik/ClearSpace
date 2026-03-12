#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>

#include "room.hpp"
#include "time.hpp"
#include "types.hpp"

namespace clearspace::core {

class RoomManager final {
 public:
  explicit RoomManager(std::shared_ptr<ITimeProvider> timeProvider);

  RoomId createRoom();
  std::shared_ptr<Room> getRoom(RoomId room);

 private:
  std::shared_ptr<ITimeProvider> timeProvider_;
  std::mutex mutex_;
  std::unordered_map<RoomId, std::shared_ptr<Room>> rooms_;
  RoomId nextRoomId_{1};
};

}
