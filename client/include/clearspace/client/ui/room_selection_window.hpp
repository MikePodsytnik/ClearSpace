#pragma once

#include <cstdint>

#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;

namespace clearspace::client {

class RoomSelectionWindow : public QWidget {
  Q_OBJECT

 public:
  explicit RoomSelectionWindow(QWidget* parent = nullptr);

  void setBusy(bool busy);
  void setStatus(const QString& text, bool isError = false);

 signals:
  void createRoomRequested();
  void joinRoomRequested(std::uint64_t roomId);

 private slots:
  void onJoinClicked();

 private:
  QLineEdit* roomIdEdit_;
  QLabel* statusLabel_;
  QPushButton* createButton_;
  QPushButton* joinButton_;
};

}
