#include "clearspace/server/session_registry.hpp"

#include "clearspace/server/client_session.hpp"

namespace clearspace::server {

void SessionRegistry::add(clearspace::core::ClientId id, const std::shared_ptr<ClientSession>& session) {
  std::scoped_lock lock(mutex_);
  sessions_[id] = session;
}

void SessionRegistry::remove(clearspace::core::ClientId id) {
  std::scoped_lock lock(mutex_);
  sessions_.erase(id);
}

std::shared_ptr<ClientSession> SessionRegistry::find(clearspace::core::ClientId id) const {
  std::scoped_lock lock(mutex_);
  auto it = sessions_.find(id);
  if (it == sessions_.end()) {
    return {};
  }
  auto session = it->second.lock();
  if (!session) {
    sessions_.erase(it);
  }
  return session;
}

void SessionRegistry::deliver(clearspace::core::ClientId id, const clearspace::transport::ServerMessage& msg) const {
  auto session = find(id);
  if (session) {
    session->deliver(msg);
  }
}

}  // namespace clearspace::server
