#include "clearspace/client/ui/room_selection_window.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace clearspace::client {

RoomSelectionWindow::RoomSelectionWindow(QWidget* parent) : QWidget(parent) {
  setWindowTitle(QStringLiteral("ClearSpace — Комнаты"));
  resize(420, 180);

  auto* layout = new QVBoxLayout(this);
  auto* row = new QHBoxLayout();

  createButton_ = new QPushButton(QStringLiteral("Создать новую комнату"), this);
  roomIdEdit_ = new QLineEdit(this);
  roomIdEdit_->setPlaceholderText(QStringLiteral("ID комнаты"));
  joinButton_ = new QPushButton(QStringLiteral("Подключиться"), this);
  statusLabel_ = new QLabel(this);

  row->addWidget(roomIdEdit_);
  row->addWidget(joinButton_);

  layout->addWidget(createButton_);
  layout->addLayout(row);
  layout->addWidget(statusLabel_);
  layout->addStretch();

  connect(createButton_, &QPushButton::clicked, this, &RoomSelectionWindow::createRoomRequested);
  connect(joinButton_, &QPushButton::clicked, this, &RoomSelectionWindow::onJoinClicked);
}

void RoomSelectionWindow::setBusy(bool busy) {
  createButton_->setEnabled(!busy);
  roomIdEdit_->setEnabled(!busy);
  joinButton_->setEnabled(!busy);
}

void RoomSelectionWindow::setStatus(const QString& text, bool isError) {
  statusLabel_->setText(text);
  statusLabel_->setStyleSheet(isError ? QStringLiteral("color: #b00020;") : QStringLiteral("color: #205020;"));
}

void RoomSelectionWindow::onJoinClicked() {
  emit joinRoomRequested(roomIdEdit_->text().trimmed().toULongLong());
}

}
