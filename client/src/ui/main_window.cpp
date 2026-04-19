#include "clearspace/client/ui/main_window.hpp"

#include <QAction>
#include <QCloseEvent>
#include <QComboBox>
#include <QLabel>
#include <QStatusBar>
#include <QToolBar>

#include "clearspace/client/model/client_context.hpp"
#include "clearspace/client/network/network_manager.hpp"
#include "clearspace/client/ui/canvas_widget.hpp"

namespace clearspace::client {

MainWindow::MainWindow(NetworkManager* networkManager, ClientContext* clientContext, QWidget* parent)
    : QMainWindow(parent), networkManager_(networkManager), clientContext_(clientContext), boardModel_(this) {
  setWindowTitle(QStringLiteral("ClearSpace — Доска"));
  resize(1100, 700);

  canvas_ = new CanvasWidget(this);
  canvas_->setBoardModel(&boardModel_);
  setCentralWidget(canvas_);

  setupToolbar();
  statusLabel_ = new QLabel(this);
  statusBar()->addPermanentWidget(statusLabel_);
  updateStatus();

  connect(canvas_, &CanvasWidget::localStrokeBegan, networkManager_, &NetworkManager::sendStrokeBegin);
  connect(canvas_, &CanvasWidget::localStrokePoint, networkManager_, &NetworkManager::sendStrokePoint);
  connect(canvas_, &CanvasWidget::localStrokeEnded, networkManager_, &NetworkManager::sendStrokeEnd);
  connect(canvas_, &CanvasWidget::cursorMoved, networkManager_, &NetworkManager::sendCursorMove);

  connect(clientContext_, &ClientContext::roomChanged, this, [this](bool hasRoom, std::uint64_t roomId) {
    canvas_->setRoomId(hasRoom ? roomId : 0);
    updateStatus();
  });
  connect(clientContext_, &ClientContext::membersChanged, this, &MainWindow::updateStatus);
  connect(colorCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this] {
    canvas_->setTool(currentColorValue(), currentThicknessValue());
  });
  connect(thicknessCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this] {
    canvas_->setTool(currentColorValue(), currentThicknessValue());
  });

  canvas_->setTool(currentColorValue(), currentThicknessValue());
}

BoardModel* MainWindow::boardModel() {
  return &boardModel_;
}

void MainWindow::setRoom(std::uint64_t roomId) {
  canvas_->setRoomId(roomId);
  updateStatus();
}

void MainWindow::onClearRequested() {
  if (!clientContext_->hasRoom()) {
    return;
  }
  boardModel_.clearBoard();
  networkManager_->sendClear(clientContext_->roomId());
}

void MainWindow::updateStatus() {
  const QString roomText = clientContext_->hasRoom() ? QString::number(clientContext_->roomId()) : QStringLiteral("—");
  statusLabel_->setText(QStringLiteral("Комната: %1 | Участников: %2")
                            .arg(roomText)
                            .arg(static_cast<int>(clientContext_->members().size())));
}

void MainWindow::closeEvent(QCloseEvent* event) {
  event->ignore();
  emit leaveRequested();
}

void MainWindow::setupToolbar() {
  auto* toolbar = addToolBar(QStringLiteral("Инструменты"));
  toolbar->setMovable(false);

  colorCombo_ = new QComboBox(toolbar);
  colorCombo_->addItem(QStringLiteral("Чёрный"), 0);
  colorCombo_->addItem(QStringLiteral("Красный"), 1);
  colorCombo_->addItem(QStringLiteral("Зелёный"), 2);
  colorCombo_->addItem(QStringLiteral("Синий"), 3);

  thicknessCombo_ = new QComboBox(toolbar);
  thicknessCombo_->addItem(QStringLiteral("Тонкое"), 0);
  thicknessCombo_->addItem(QStringLiteral("Среднее"), 1);
  thicknessCombo_->addItem(QStringLiteral("Толстое"), 2);

  auto* clearAction = toolbar->addAction(QStringLiteral("Очистить доску"));

  toolbar->addWidget(new QLabel(QStringLiteral("Цвет: "), toolbar));
  toolbar->addWidget(colorCombo_);
  toolbar->addSeparator();
  toolbar->addWidget(new QLabel(QStringLiteral("Толщина: "), toolbar));
  toolbar->addWidget(thicknessCombo_);

  connect(clearAction, &QAction::triggered, this, &MainWindow::onClearRequested);
}

std::uint8_t MainWindow::currentColorValue() const {
  return static_cast<std::uint8_t>(colorCombo_->currentData().toUInt());
}

std::uint8_t MainWindow::currentThicknessValue() const {
  return static_cast<std::uint8_t>(thicknessCombo_->currentData().toUInt());
}

}
