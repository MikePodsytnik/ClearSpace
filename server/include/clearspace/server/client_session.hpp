#pragma once

#include <atomic>
#include <deque>
#include <memory>
#include <optional>
#include <string>

#include <boost/asio.hpp>

#include "clearspace/core/server_core.hpp"
#include "clearspace/transport/messages.hpp"
#include "clearspace/transport/protocol_error.hpp"

namespace clearspace::server {

class SessionRegistry;

class ClientSession final : public std::enable_shared_from_this<ClientSession> {
 public:
  using tcp = boost::asio::ip::tcp;

  ClientSession(tcp::socket socket,
                clearspace::core::ClientId clientId,
                std::shared_ptr<clearspace::core::ServerCore> core,
                std::shared_ptr<SessionRegistry> registry);

  void start();
  void deliver(const clearspace::transport::ServerMessage& msg);
  void stop();

  clearspace::core::ClientId clientId() const noexcept;

 private:
  void doReadLine();
  void onReadLine(const boost::system::error_code& ec, std::size_t bytesTransferred);
  void handleLine(const std::string& line);
  void handleMessage(const clearspace::transport::ClientMessage& msg);
  void doWrite();
  void onWrite(const boost::system::error_code& ec, std::size_t bytesTransferred);
  void stopImpl();
  void sendError(const clearspace::core::Error& error);
  void sendProtocolError(const clearspace::transport::ProtocolError& error);
  void dispatchOutgoing(const std::vector<clearspace::core::OutgoingEvent>& events);

  tcp::socket socket_;
  boost::asio::strand<boost::asio::any_io_executor> strand_;
  boost::asio::streambuf readBuffer_;
  std::deque<std::string> writeQueue_;

  clearspace::core::ClientId clientId_{};
  std::shared_ptr<clearspace::core::ServerCore> core_;
  std::shared_ptr<SessionRegistry> registry_;
  std::optional<clearspace::core::RoomId> currentRoom_;
  std::atomic_bool stopped_{false};
};

}
