#include "clearspace/client/model/client_context.hpp"

#include <algorithm>

namespace clearspace::client {

ClientContext::ClientContext(QObject* parent) : QObject(parent) {}

bool ClientContext::isConnected() const {
  return connected_;
}

QString ClientContext::host() const {
  return host_;
}

quint16 ClientContext::port() const {
  return port_;
}

bool ClientContext::hasRoom() const {
  return hasRoom_;
}

std::uint64_t ClientContext::roomId() const {
  return roomId_;
}

const std::vector<std::uint64_t>& ClientContext::members() const {
  return members_;
}

void ClientContext::setConnection(bool connected, const QString& host, quint16 port) {
  connected_ = connected;
  host_ = host;
  port_ = port;
  emit connectionChanged(connected_, host_, port_);
}

void ClientContext::resetConnection() {
  connected_ = false;
  host_.clear();
  port_ = 0;
  clearRoom();
  emit connectionChanged(false, host_, port_);
}

void ClientContext::setRoom(std::uint64_t roomId) {
  hasRoom_ = true;
  roomId_ = roomId;
  emit roomChanged(true, roomId_);
}

void ClientContext::clearRoom() {
  hasRoom_ = false;
  roomId_ = 0;
  members_.clear();
  emit roomChanged(false, 0);
  emit membersChanged(0);
}

void ClientContext::setMembers(std::vector<std::uint64_t> members) {
  members_ = std::move(members);
  emit membersChanged(static_cast<int>(members_.size()));
}


void ClientContext::ensureMember(std::uint64_t clientId) {
  if (std::find(members_.begin(), members_.end(), clientId) != members_.end()) {
    return;
  }
  members_.push_back(clientId);
  emit membersChanged(static_cast<int>(members_.size()));
}

void ClientContext::removeMember(std::uint64_t clientId) {
  members_.erase(std::remove(members_.begin(), members_.end(), clientId), members_.end());
  emit membersChanged(static_cast<int>(members_.size()));
}

}
