#pragma once

#include <cstdint>

#include <QMainWindow>

#include "clearspace/client/model/board_model.hpp"

class QAction;
class QLabel;
class QComboBox;

namespace clearspace::client {

class CanvasWidget;
class NetworkManager;
class ClientContext;

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  MainWindow(NetworkManager* networkManager, ClientContext* clientContext, QWidget* parent = nullptr);

  BoardModel* boardModel();
  void setRoom(std::uint64_t roomId);

 signals:
  void leaveRequested();

 private slots:
  void onClearRequested();
  void updateStatus();

 protected:
  void closeEvent(QCloseEvent* event) override;

 private:
  void setupToolbar();
  std::uint8_t currentColorValue() const;
  std::uint8_t currentThicknessValue() const;

  NetworkManager* networkManager_;
  ClientContext* clientContext_;
  BoardModel boardModel_;
  CanvasWidget* canvas_;
  QLabel* statusLabel_;
  QComboBox* colorCombo_;
  QComboBox* thicknessCombo_;
};

}
