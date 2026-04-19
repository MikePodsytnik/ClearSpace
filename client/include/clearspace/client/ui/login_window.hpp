#pragma once

#include <QWidget>

class QLineEdit;
class QLabel;
class QPushButton;

namespace clearspace::client {

class LoginWindow : public QWidget {
  Q_OBJECT

 public:
  explicit LoginWindow(QWidget* parent = nullptr);

  void setBusy(bool busy);
  void setStatus(const QString& text, bool isError = false);
  QString host() const;
  quint16 port() const;

 signals:
  void connectRequested(const QString& host, quint16 port);

 private slots:
  void onConnectClicked();

 private:
  QLineEdit* hostEdit_;
  QLineEdit* portEdit_;
  QLabel* statusLabel_;
  QPushButton* connectButton_;
};

}
