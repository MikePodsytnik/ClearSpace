#include "clearspace/client/ui/login_window.hpp"

#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace clearspace::client {

LoginWindow::LoginWindow(QWidget* parent) : QWidget(parent) {
  setWindowTitle(QStringLiteral("ClearSpace — Подключение"));
  resize(360, 180);

  auto* layout = new QVBoxLayout(this);
  auto* form = new QFormLayout();

  hostEdit_ = new QLineEdit(QStringLiteral("127.0.0.1"), this);
  portEdit_ = new QLineEdit(QStringLiteral("5555"), this);
  statusLabel_ = new QLabel(this);
  connectButton_ = new QPushButton(QStringLiteral("Подключиться"), this);

  form->addRow(QStringLiteral("IP-адрес"), hostEdit_);
  form->addRow(QStringLiteral("Порт"), portEdit_);
  layout->addLayout(form);
  layout->addWidget(connectButton_);
  layout->addWidget(statusLabel_);
  layout->addStretch();

  connect(connectButton_, &QPushButton::clicked, this, &LoginWindow::onConnectClicked);
}

void LoginWindow::setBusy(bool busy) {
  hostEdit_->setEnabled(!busy);
  portEdit_->setEnabled(!busy);
  connectButton_->setEnabled(!busy);
}

void LoginWindow::setStatus(const QString& text, bool isError) {
  statusLabel_->setText(text);
  statusLabel_->setStyleSheet(isError ? QStringLiteral("color: #b00020;") : QStringLiteral("color: #205020;"));
}

QString LoginWindow::host() const {
  return hostEdit_->text().trimmed();
}

quint16 LoginWindow::port() const {
  return portEdit_->text().toUShort();
}

void LoginWindow::onConnectClicked() {
  emit connectRequested(host(), port());
}

}
