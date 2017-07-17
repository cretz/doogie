#include "debug_meta_server.h"

#include <QtWebSockets/QtWebSockets>

namespace doogie {

DebugMetaServer::DebugMetaServer(MainWindow* window) : QObject(window) {
  auto server = new QWebSocketServer("Doogie Debug Meta Server",
                                     QWebSocketServer::NonSecureMode,
                                     this);
  if (!server->listen(QHostAddress::LocalHost, 1983)) {
    qFatal("Unable to open web socket server");
    return;
  }
  connect(server, &QWebSocketServer::newConnection, [this, server, window]() {
    auto socket = server->nextPendingConnection();
    connect(socket, &QWebSocket::disconnected,
            socket, &QWebSocket::deleteLater);
    connect(server, &QWebSocketServer::destroyed,
            socket, &QWebSocket::deleteLater);
    connect(socket, &QWebSocket::textMessageReceived,
            [this, socket, window](const QString& msg) {
      QJsonObject obj;
      if (msg == "dump") {
        obj = window->DebugDump();
      } else if (msg == "activate") {
        window->activateWindow();
        obj = window->DebugDump();
      } else {
        obj["error"] = "Unrecognized message";
      }
      socket->sendTextMessage(QString::fromUtf8(
          QJsonDocument(obj).toJson(QJsonDocument::Compact)));
    });
  });
}

}  // namespace doogie
