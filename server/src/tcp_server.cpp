#include "clearspace/server/tcp_server.hpp"

#include "clearspace/server/client_session.hpp"

namespace clearspace::server {

TcpServer::TcpServer(boost::asio::io_context& ioContext,
                     const tcp::endpoint& endpoint,
                     std::shared_ptr<clearspace::core::ServerCore> core,
                     std::shared_ptr<SessionRegistry> registry)
    : ioContext_(ioContext),
      acceptor_(ioContext),
      core_(std::move(core)),
      registry_(std::move(registry)) {
  acceptor_.open(endpoint.protocol());
  acceptor_.set_option(tcp::acceptor::reuse_address(true));
  acceptor_.bind(endpoint);
  acceptor_.listen(boost::asio::socket_base::max_listen_connections);
}

void TcpServer::start() {
  doAccept();
}

unsigned short TcpServer::port() const {
  return acceptor_.local_endpoint().port();
}

void TcpServer::doAccept() {
  acceptor_.async_accept(
      boost::asio::make_strand(ioContext_),
      [this](const boost::system::error_code& ec, tcp::socket socket) mutable { onAccept(ec, std::move(socket)); });
}

void TcpServer::onAccept(const boost::system::error_code& ec, tcp::socket socket) {
  if (!ec) {
    auto session = std::make_shared<ClientSession>(
        std::move(socket), nextClientId_.fetch_add(1, std::memory_order_relaxed), core_, registry_);
    registry_->add(session->clientId(), session);
    session->start();
  }

  if (acceptor_.is_open()) {
    doAccept();
  }
}

}  // namespace clearspace::server
