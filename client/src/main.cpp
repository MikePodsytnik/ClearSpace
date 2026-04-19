#include <QApplication>
#include <QMessageBox>

#include "clearspace/client/model/client_context.hpp"
#include "clearspace/client/network/client_protocol.hpp"
#include "clearspace/client/network/network_manager.hpp"
#include "clearspace/client/ui/login_window.hpp"
#include "clearspace/client/ui/main_window.hpp"
#include "clearspace/client/ui/room_selection_window.hpp"

using namespace clearspace::client;

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  app.setQuitOnLastWindowClosed(false);

  qRegisterMetaType<RoomCreatedData>();
  qRegisterMetaType<OkData>();
  qRegisterMetaType<ErrorData>();
  qRegisterMetaType<SnapshotData>();
  qRegisterMetaType<StrokeBeginData>();
  qRegisterMetaType<StrokePointData>();
  qRegisterMetaType<StrokeEndData>();
  qRegisterMetaType<CursorMoveData>();
  qRegisterMetaType<ClearData>();
  qRegisterMetaType<MemberLeftData>();

  NetworkManager networkManager;
  ClientContext clientContext;
  LoginWindow loginWindow;
  RoomSelectionWindow roomWindow;
  MainWindow mainWindow(&networkManager, &clientContext);

  loginWindow.show();

  QObject::connect(&loginWindow, &LoginWindow::connectRequested, &networkManager, [&](const QString& host, quint16 port) {
    loginWindow.setBusy(true);
    loginWindow.setStatus(QStringLiteral("Подключение..."));
    networkManager.connectToServer(host, port);
  });

  QObject::connect(&networkManager, &NetworkManager::connected, &loginWindow, [&] {
    clientContext.setConnection(true, loginWindow.host(), loginWindow.port());
    loginWindow.setBusy(false);
    loginWindow.setStatus(QStringLiteral("Подключено"));
    loginWindow.hide();
    roomWindow.setBusy(false);
    roomWindow.setStatus(QStringLiteral("Выберите действие"));
    roomWindow.show();
  });

  QObject::connect(&networkManager, &NetworkManager::connectionFailed, &loginWindow, [&](const QString& reason) {
    loginWindow.setBusy(false);
    loginWindow.setStatus(reason, true);
  });

  QObject::connect(&networkManager, &NetworkManager::disconnected, &app, [&](const QString& reason) {
    clientContext.resetConnection();
    roomWindow.hide();
    mainWindow.hide();
    loginWindow.setBusy(false);
    loginWindow.setStatus(reason, true);
    loginWindow.show();
    QMessageBox::warning(nullptr, QStringLiteral("ClearSpace"), reason);
  });

  QObject::connect(&roomWindow, &RoomSelectionWindow::createRoomRequested, &networkManager, [&] {
    roomWindow.setBusy(true);
    roomWindow.setStatus(QStringLiteral("Создание комнаты..."));
    networkManager.createRoom();
  });

  QObject::connect(&roomWindow, &RoomSelectionWindow::joinRoomRequested, &networkManager, [&](std::uint64_t roomId) {
    roomWindow.setBusy(true);
    roomWindow.setStatus(QStringLiteral("Подключение к комнате %1...").arg(roomId));
    networkManager.joinRoom(roomId);
  });

  QObject::connect(&networkManager, &NetworkManager::roomCreated, &roomWindow, [&](const RoomCreatedData& data) {
    roomWindow.setStatus(QStringLiteral("Комната %1 создана, выполняется вход...").arg(data.room));
    networkManager.joinRoom(data.room);
  });

  QObject::connect(&networkManager, &NetworkManager::snapshotReceived, &app, [&](const SnapshotData& data) {
    clientContext.setRoom(data.room);
    clientContext.setMembers(data.members);
    mainWindow.boardModel()->resetFromSnapshot(data);
    mainWindow.setRoom(data.room);
    roomWindow.setBusy(false);
    roomWindow.hide();
    mainWindow.show();
    mainWindow.raise();
    mainWindow.activateWindow();
  });

  QObject::connect(&networkManager, &NetworkManager::okReceived, &app, [&](const OkData& data) {
    if (data.request == QStringLiteral("leave_room")) {
      clientContext.clearRoom();
      mainWindow.boardModel()->clearBoard();
      mainWindow.hide();
      roomWindow.setBusy(false);
      roomWindow.setStatus(QStringLiteral("Выберите действие"));
      roomWindow.show();
      roomWindow.raise();
      roomWindow.activateWindow();
    }
  });

  QObject::connect(&networkManager, &NetworkManager::errorReceived, &app, [&](const ErrorData& data) {
    roomWindow.setBusy(false);
    loginWindow.setBusy(false);
    const QString text = QStringLiteral("%1: %2").arg(data.code, data.message);
    if (mainWindow.isVisible()) {
      QMessageBox::critical(&mainWindow, QStringLiteral("Ошибка сервера"), text);
    } else if (roomWindow.isVisible()) {
      roomWindow.setStatus(text, true);
    } else {
      loginWindow.setStatus(text, true);
    }
  });

  QObject::connect(&networkManager, &NetworkManager::remoteStrokeBeginReceived, &app, [&](const StrokeBeginData& data) {
    if (clientContext.hasRoom() && data.room == clientContext.roomId()) {
      clientContext.ensureMember(data.author);
      mainWindow.boardModel()->applyRemoteStrokeBegin(data);
    }
  });
  QObject::connect(&networkManager, &NetworkManager::remoteStrokePointReceived, &app, [&](const StrokePointData& data) {
    if (clientContext.hasRoom() && data.room == clientContext.roomId()) {
      clientContext.ensureMember(data.author);
      mainWindow.boardModel()->applyRemoteStrokePoint(data);
    }
  });
  QObject::connect(&networkManager, &NetworkManager::remoteStrokeEndReceived, &app, [&](const StrokeEndData& data) {
    if (clientContext.hasRoom() && data.room == clientContext.roomId()) {
      clientContext.ensureMember(data.author);
      mainWindow.boardModel()->applyRemoteStrokeEnd(data);
    }
  });
  QObject::connect(&networkManager, &NetworkManager::remoteCursorMoveReceived, &app, [&](const CursorMoveData& data) {
    if (clientContext.hasRoom() && data.room == clientContext.roomId()) {
      clientContext.ensureMember(data.author);
      mainWindow.boardModel()->applyRemoteCursorMove(data);
    }
  });
  QObject::connect(&networkManager, &NetworkManager::clearReceived, &app, [&](const ClearData& data) {
    if (clientContext.hasRoom() && data.room == clientContext.roomId()) {
      mainWindow.boardModel()->clearBoard();
    }
  });
  QObject::connect(&networkManager, &NetworkManager::memberLeftReceived, &app, [&](const MemberLeftData& data) {
    if (clientContext.hasRoom() && data.room == clientContext.roomId()) {
      clientContext.removeMember(data.client);
      mainWindow.boardModel()->removeCursor(data.client);
    }
  });

  QObject::connect(&mainWindow, &MainWindow::leaveRequested, &app, [&] {
    if (clientContext.hasRoom()) {
      networkManager.leaveRoom(clientContext.roomId());
    }
  });

  return app.exec();
}
