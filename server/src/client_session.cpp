#include "clearspace/server/client_session.hpp"

#include <utility>

#include "clearspace/server/session_registry.hpp"
#include "clearspace/transport/json_protocol.hpp"
#include "clearspace/transport/mappers.hpp"
#include "clearspace/transport/protocol_error.hpp"

namespace clearspace::server {

clearspace::core::Error protocolToCoreError(const clearspace::transport::ProtocolError& error) {
  switch (error.code) {
    case clearspace::transport::ProtocolErrorCode::InvalidJson:
    case clearspace::transport::ProtocolErrorCode::MissingField:
    case clearspace::transport::ProtocolErrorCode::InvalidFieldType:
    case clearspace::transport::ProtocolErrorCode::UnknownMessageType:
    case clearspace::transport::ProtocolErrorCode::InvalidMessage:
      return clearspace::core::Error::invalidParams(error.message);
  }
  return clearspace::core::Error::invalidParams(error.message);
}

ClientSession::ClientSession(tcp::socket socket,
                             clearspace::core::ClientId clientId,
                             std::shared_ptr<clearspace::core::ServerCore> core,
                             std::shared_ptr<SessionRegistry> registry)
    : socket_(std::move(socket)),
      strand_(boost::asio::make_strand(socket_.get_executor())),
      clientId_(clientId),
      core_(std::move(core)),
      registry_(std::move(registry)) {}

void ClientSession::start() {
  boost::asio::dispatch(strand_, [self = shared_from_this()] { self->doReadLine(); });
}

void ClientSession::deliver(const clearspace::transport::ServerMessage& msg) {
  auto line = clearspace::transport::serializeServerMessage(msg) + "\n";
  boost::asio::post(strand_, [self = shared_from_this(), line = std::move(line)]() mutable {
    const bool wasIdle = self->writeQueue_.empty();
    self->writeQueue_.push_back(std::move(line));
    if (wasIdle) {
      self->doWrite();
    }
  });
}

void ClientSession::stop() {
  boost::asio::post(strand_, [self = shared_from_this()] { self->stopImpl(); });
}

clearspace::core::ClientId ClientSession::clientId() const noexcept {
  return clientId_;
}

void ClientSession::doReadLine() {
  if (stopped_.load(std::memory_order_relaxed)) {
    return;
  }
  boost::asio::async_read_until(
      socket_,
      readBuffer_,
      '\n',
      boost::asio::bind_executor(
          strand_,
          [self = shared_from_this()](const boost::system::error_code& ec, std::size_t bytesTransferred) {
            self->onReadLine(ec, bytesTransferred);
          }));
}

void ClientSession::onReadLine(const boost::system::error_code& ec, std::size_t) {
  if (ec) {
    stopImpl();
    return;
  }

  std::istream input(&readBuffer_);
  std::string line;
  std::getline(input, line);
  handleLine(line);

  if (!stopped_.load(std::memory_order_relaxed)) {
    doReadLine();
  }
}

void ClientSession::handleLine(const std::string& line) {
  auto msg = clearspace::transport::parseClientMessage(line);
  if (!msg) {
    sendProtocolError(msg.error());
    return;
  }
  handleMessage(msg.value());
}

void ClientSession::handleMessage(const clearspace::transport::ClientMessage& msg) {
  std::visit(
      [this](const auto& request) {
        using T = std::decay_t<decltype(request)>;
        if constexpr (std::is_same_v<T, clearspace::transport::PingRequest>) {
          deliver(clearspace::transport::PongResponse{});
        } else if constexpr (std::is_same_v<T, clearspace::transport::CreateRoomRequest>) {
          auto room = core_->createRoom();
          deliver(clearspace::transport::RoomCreatedResponse{.room = room});
        } else if constexpr (std::is_same_v<T, clearspace::transport::JoinRoomRequest>) {
          if (currentRoom_ && *currentRoom_ != request.room) {
            sendError(clearspace::core::Error::invalidParams("session already joined to another room"));
            return;
          }
          auto result = core_->joinRoom(clientId_, request.room);
          if (!result) {
            sendError(result.error());
            return;
          }
          currentRoom_ = request.room;
          deliver(clearspace::transport::toSnapshotMessage(result.value()));
        } else if constexpr (std::is_same_v<T, clearspace::transport::LeaveRoomRequest>) {
          if (!currentRoom_ || *currentRoom_ != request.room) {
            sendError(clearspace::core::Error::notInRoom());
            return;
          }
          auto result = core_->leaveRoom(clientId_, request.room);
          if (!result) {
            sendError(result.error());
            return;
          }
          currentRoom_.reset();
          dispatchOutgoing(result.value());
          deliver(clearspace::transport::OkResponse{.request = "leave_room", .room = request.room});
        } else {
          if (!currentRoom_ || *currentRoom_ != request.room) {
            sendError(clearspace::core::Error::notInRoom());
            return;
          }
          auto event = clearspace::transport::toCoreEvent(request);
          if (!event) {
            sendError(event.error());
            return;
          }
          auto result = core_->handleEvent(clientId_, request.room, event.value());
          if (!result) {
            sendError(result.error());
            return;
          }
          dispatchOutgoing(result.value());
        }
      },
      msg);
}

void ClientSession::doWrite() {
  if (writeQueue_.empty() || stopped_.load(std::memory_order_relaxed)) {
    return;
  }
  boost::asio::async_write(
      socket_,
      boost::asio::buffer(writeQueue_.front()),
      boost::asio::bind_executor(
          strand_,
          [self = shared_from_this()](const boost::system::error_code& ec, std::size_t bytesTransferred) {
            self->onWrite(ec, bytesTransferred);
          }));
}

void ClientSession::onWrite(const boost::system::error_code& ec, std::size_t) {
  if (ec) {
    stopImpl();
    return;
  }
  writeQueue_.pop_front();
  if (!writeQueue_.empty()) {
    doWrite();
  }
}

void ClientSession::stopImpl() {
  if (stopped_.exchange(true, std::memory_order_relaxed)) {
    return;
  }

  if (currentRoom_) {
    auto result = core_->leaveRoom(clientId_, *currentRoom_);
    if (result) {
      dispatchOutgoing(result.value());
    }
    currentRoom_.reset();
  }

  boost::system::error_code ignored;
  socket_.shutdown(tcp::socket::shutdown_both, ignored);
  socket_.close(ignored);
  registry_->remove(clientId_);
}

void ClientSession::sendError(const clearspace::core::Error& error) {
  deliver(clearspace::transport::toErrorMessage(error));
}

void ClientSession::sendProtocolError(const clearspace::transport::ProtocolError& error) {
  sendError(protocolToCoreError(error));
}

void ClientSession::dispatchOutgoing(const std::vector<clearspace::core::OutgoingEvent>& events) {
  for (const auto& event : events) {
    registry_->deliver(event.to, clearspace::transport::toServerMessage(event));
  }
}

}
