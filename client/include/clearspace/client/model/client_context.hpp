#pragma once

#include <cstdint>
#include <vector>

#include <QObject>
#include <QString>

namespace clearspace::client {

class ClientContext : public QObject {
  Q_OBJECT

 public:
  explicit ClientContext(QObject* parent = nullptr);

  bool isConnected() const;
  QString host() const;
  quint16 port() const;
  bool hasRoom() const;
  std::uint64_t roomId() const;
  const std::vector<std::uint64_t>& members() const;

  void setConnection(bool connected, const QString& host, quint16 port);
  void resetConnection();
  void setRoom(std::uint64_t roomId);
  void clearRoom();
  void setMembers(std::vector<std::uint64_t> members);
  void ensureMember(std::uint64_t clientId);
  void removeMember(std::uint64_t clientId);

 signals:
  void connectionChanged(bool connected, const QString& host, quint16 port);
  void roomChanged(bool hasRoom, std::uint64_t roomId);
  void membersChanged(int count);

 private:
  bool connected_{false};
  QString host_;
  quint16 port_{0};
  bool hasRoom_{false};
  std::uint64_t roomId_{0};
  std::vector<std::uint64_t> members_;
};

}
