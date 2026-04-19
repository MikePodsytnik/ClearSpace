#include "clearspace/client/network/network_manager.hpp"

#include <type_traits>
#include <utility>

#include <QMetaObject>

namespace clearspace::client {

NetworkManager::NetworkManager(QObject* parent)
    : QObject(parent),
      workGuard_(boost::asio::make_work_guard(ioContext_)),
      resolver_(ioContext_),
      socket_(ioContext_) {
  ioThread_ = std::thread([this] { ioContext_.run(); });
}

NetworkManager::~NetworkManager() {
  disconnectFromServer();
  workGuard_.reset();
  ioContext_.stop();
  if (ioThread_.joinable()) {
    ioThread_.join();
  }
}

void NetworkManager::connectToServer(const QString& host, quint16 port) {
  const auto hostStd = host.toStdString();
  const auto portStd = std::to_string(port);

  boost::asio::post(ioContext_, [this, hostStd, portStd] {
    disconnectReported_.store(false);
    closeSocket();
    writeQueue_.clear();
    resolver_.async_resolve(
        hostStd,
        portStd,
        [this](const boost::system::error_code& ec, const tcp::resolver::results_type& results) {
          if (ec) {
            const QString message = QString::fromStdString(ec.message());
            QMetaObject::invokeMethod(this, [this, message] { emit connectionFailed(message); }, Qt::QueuedConnection);
            return;
          }
          boost::asio::async_connect(
              socket_,
              results,
              [this](const boost::system::error_code& connectEc, const tcp::endpoint&) {
                if (connectEc) {
                  const QString message = QString::fromStdString(connectEc.message());
                  QMetaObject::invokeMethod(this, [this, message] { emit connectionFailed(message); }, Qt::QueuedConnection);
                  return;
                }
                connected_.store(true);
                disconnectReported_.store(false);
                QMetaObject::invokeMethod(this, [this] { emit connected(); }, Qt::QueuedConnection);
                startRead();
              });
        });
  });
}

void NetworkManager::disconnectFromServer() {
  boost::asio::post(ioContext_, [this] {
    const bool wasConnected = connected_.exchange(false);
    closeSocket();
    writeQueue_.clear();
    if (wasConnected && !disconnectReported_.exchange(true)) {
      const QString reason = QStringLiteral("Соединение закрыто");
      QMetaObject::invokeMethod(this, [this, reason] { emit disconnected(reason); }, Qt::QueuedConnection);
    }
  });
}

void NetworkManager::ping() {
  postWrite(makePingRequest());
}

void NetworkManager::createRoom() {
  postWrite(makeCreateRoomRequest());
}

void NetworkManager::joinRoom(RoomId room) {
  postWrite(makeJoinRoomRequest(room));
}

void NetworkManager::leaveRoom(RoomId room) {
  postWrite(makeLeaveRoomRequest(room));
}

void NetworkManager::sendStrokeBegin(RoomId room, std::uint8_t color, std::uint8_t thickness, const QPointF& start) {
  postWrite(makeStrokeBeginRequest(room, color, thickness, start));
}

void NetworkManager::sendStrokePoint(RoomId room, const QPointF& point) {
  postWrite(makeStrokePointRequest(room, point));
}

void NetworkManager::sendStrokeEnd(RoomId room) {
  postWrite(makeStrokeEndRequest(room));
}

void NetworkManager::sendCursorMove(RoomId room, const QPointF& point) {
  postWrite(makeCursorMoveRequest(room, point));
}

void NetworkManager::sendClear(RoomId room) {
  postWrite(makeClearRequest(room));
}

void NetworkManager::postWrite(std::string line) {
  boost::asio::post(ioContext_, [this, line = std::move(line)]() mutable {
    if (!socket_.is_open()) {
      return;
    }
    const bool writing = !writeQueue_.empty();
    writeQueue_.push_back(std::move(line));
    if (!writing) {
      doWrite();
    }
  });
}

void NetworkManager::doWrite() {
  if (writeQueue_.empty() || !socket_.is_open()) {
    return;
  }

  boost::asio::async_write(
      socket_,
      boost::asio::buffer(writeQueue_.front()),
      [this](const boost::system::error_code& ec, std::size_t) {
        if (ec) {
          reportDisconnect(QString::fromStdString(ec.message()));
          return;
        }
        writeQueue_.pop_front();
        if (!writeQueue_.empty()) {
          doWrite();
        }
      });
}

void NetworkManager::startRead() {
  boost::asio::async_read_until(
      socket_,
      readBuffer_,
      '\n',
      [this](const boost::system::error_code& ec, std::size_t) {
        if (ec) {
          reportDisconnect(QString::fromStdString(ec.message()));
          return;
        }

        std::istream input(&readBuffer_);
        std::string line;
        std::getline(input, line);

        auto parsed = parseServerMessage(line);
        if (!parsed) {
          const ErrorData error{QStringLiteral("protocol_error"), parsed.error()};
          QMetaObject::invokeMethod(this, [this, error] { emit errorReceived(error); }, Qt::QueuedConnection);
        } else {
          handleParsedMessage(parsed.value());
        }

        startRead();
      });
}

void NetworkManager::handleParsedMessage(const ServerMessage& message) {
  std::visit(
      [this](const auto& payload) {
        using T = std::decay_t<decltype(payload)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
        } else if constexpr (std::is_same_v<T, RoomCreatedData>) {
          QMetaObject::invokeMethod(this, [this, payload] { emit roomCreated(payload); }, Qt::QueuedConnection);
        } else if constexpr (std::is_same_v<T, OkData>) {
          QMetaObject::invokeMethod(this, [this, payload] { emit okReceived(payload); }, Qt::QueuedConnection);
        } else if constexpr (std::is_same_v<T, ErrorData>) {
          QMetaObject::invokeMethod(this, [this, payload] { emit errorReceived(payload); }, Qt::QueuedConnection);
        } else if constexpr (std::is_same_v<T, SnapshotData>) {
          QMetaObject::invokeMethod(this, [this, payload] { emit snapshotReceived(payload); }, Qt::QueuedConnection);
        } else if constexpr (std::is_same_v<T, StrokeBeginData>) {
          QMetaObject::invokeMethod(this, [this, payload] { emit remoteStrokeBeginReceived(payload); }, Qt::QueuedConnection);
        } else if constexpr (std::is_same_v<T, StrokePointData>) {
          QMetaObject::invokeMethod(this, [this, payload] { emit remoteStrokePointReceived(payload); }, Qt::QueuedConnection);
        } else if constexpr (std::is_same_v<T, StrokeEndData>) {
          QMetaObject::invokeMethod(this, [this, payload] { emit remoteStrokeEndReceived(payload); }, Qt::QueuedConnection);
        } else if constexpr (std::is_same_v<T, CursorMoveData>) {
          QMetaObject::invokeMethod(this, [this, payload] { emit remoteCursorMoveReceived(payload); }, Qt::QueuedConnection);
        } else if constexpr (std::is_same_v<T, ClearData>) {
          QMetaObject::invokeMethod(this, [this, payload] { emit clearReceived(payload); }, Qt::QueuedConnection);
        } else if constexpr (std::is_same_v<T, MemberLeftData>) {
          QMetaObject::invokeMethod(this, [this, payload] { emit memberLeftReceived(payload); }, Qt::QueuedConnection);
        }
      },
      message);
}

void NetworkManager::reportDisconnect(const QString& reason) {
  const bool wasConnected = connected_.exchange(false);
  closeSocket();
  writeQueue_.clear();
  if ((wasConnected || socket_.is_open()) && !disconnectReported_.exchange(true)) {
    QMetaObject::invokeMethod(this, [this, reason] { emit disconnected(reason); }, Qt::QueuedConnection);
  }
}

void NetworkManager::closeSocket() {
  boost::system::error_code ignored;
  resolver_.cancel();
  if (socket_.is_open()) {
    socket_.shutdown(tcp::socket::shutdown_both, ignored);
    socket_.close(ignored);
  }
}

}
