#pragma once

#include <atomic>
#include <memory>

#include <boost/asio.hpp>

#include "clearspace/core/server_core.hpp"
#include "session_registry.hpp"

namespace clearspace::server {

class TcpServer final {
 public:
  using tcp = boost::asio::ip::tcp;

  TcpServer(boost::asio::io_context& ioContext,
            const tcp::endpoint& endpoint,
            std::shared_ptr<clearspace::core::ServerCore> core,
            std::shared_ptr<SessionRegistry> registry = std::make_shared<SessionRegistry>());

  void start();
  unsigned short port() const;

 private:
  void doAccept();
  void onAccept(const boost::system::error_code& ec, tcp::socket socket);

  boost::asio::io_context& ioContext_;
  tcp::acceptor acceptor_;
  std::shared_ptr<clearspace::core::ServerCore> core_;
  std::shared_ptr<SessionRegistry> registry_;
  std::atomic<clearspace::core::ClientId> nextClientId_{1};
};

}
