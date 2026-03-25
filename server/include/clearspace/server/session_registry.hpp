#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>

#include "clearspace/core/types.hpp"
#include "clearspace/transport/messages.hpp"

namespace clearspace::server {

class ClientSession;

class SessionRegistry final {
 public:
  void add(clearspace::core::ClientId id, const std::shared_ptr<ClientSession>& session);
  void remove(clearspace::core::ClientId id);
  std::shared_ptr<ClientSession> find(clearspace::core::ClientId id) const;
  void deliver(clearspace::core::ClientId id, const clearspace::transport::ServerMessage& msg) const;

 private:
  mutable std::mutex mutex_;
  mutable std::unordered_map<clearspace::core::ClientId, std::weak_ptr<ClientSession>> sessions_;
};

}
