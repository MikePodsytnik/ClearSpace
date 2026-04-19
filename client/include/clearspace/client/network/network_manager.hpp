#pragma once

#include <atomic>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>

#include <QObject>
#include <QString>

#include <boost/asio.hpp>

#include "clearspace/client/network/client_protocol.hpp"

namespace clearspace::client {

class NetworkManager : public QObject {
  Q_OBJECT

 public:
  explicit NetworkManager(QObject* parent = nullptr);
  ~NetworkManager() override;

  void connectToServer(const QString& host, quint16 port);
  void disconnectFromServer();

  void ping();
  void createRoom();
  void joinRoom(RoomId room);
  void leaveRoom(RoomId room);
  void sendStrokeBegin(RoomId room, std::uint8_t color, std::uint8_t thickness, const QPointF& start);
  void sendStrokePoint(RoomId room, const QPointF& point);
  void sendStrokeEnd(RoomId room);
  void sendCursorMove(RoomId room, const QPointF& point);
  void sendClear(RoomId room);

 signals:
  void connected();
  void disconnected(const QString& reason);
  void connectionFailed(const QString& reason);
  void roomCreated(const clearspace::client::RoomCreatedData& data);
  void okReceived(const clearspace::client::OkData& data);
  void errorReceived(const clearspace::client::ErrorData& data);
  void snapshotReceived(const clearspace::client::SnapshotData& data);
  void remoteStrokeBeginReceived(const clearspace::client::StrokeBeginData& data);
  void remoteStrokePointReceived(const clearspace::client::StrokePointData& data);
  void remoteStrokeEndReceived(const clearspace::client::StrokeEndData& data);
  void remoteCursorMoveReceived(const clearspace::client::CursorMoveData& data);
  void clearReceived(const clearspace::client::ClearData& data);
  void memberLeftReceived(const clearspace::client::MemberLeftData& data);

 private:
  using tcp = boost::asio::ip::tcp;

  void postWrite(std::string line);
  void doWrite();
  void startRead();
  void handleParsedMessage(const ServerMessage& message);
  void reportDisconnect(const QString& reason);
  void closeSocket();

  boost::asio::io_context ioContext_;
  boost::asio::executor_work_guard<boost::asio::io_context::executor_type> workGuard_;
  tcp::resolver resolver_;
  tcp::socket socket_;
  boost::asio::streambuf readBuffer_;
  std::deque<std::string> writeQueue_;
  std::thread ioThread_;
  std::atomic_bool connected_{false};
  std::atomic_bool disconnectReported_{false};
};

}
