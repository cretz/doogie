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
  connect(server, &QWebSocketServer::newConnection, [=]() {
    auto socket = server->nextPendingConnection();
    connect(socket, &QWebSocket::disconnected,
            socket, &QWebSocket::deleteLater);
    connect(server, &QWebSocketServer::destroyed,
            socket, &QWebSocket::deleteLater);
    connect(socket, &QWebSocket::textMessageReceived, [=](const QString& msg) {
      QJsonObject obj;
      if (msg == "dump") {
        obj = window->DebugDump();
      } else if (msg == "activate") {
        window->activateWindow();
        obj = window->DebugDump();
      } else if (msg.startsWith("screenshot")) {
        // After the word "screenshot" is a JSON describing what is wanted
        auto json = QJsonDocument::fromJson(
              msg.right(msg.size() - 10).toUtf8());
        if (!json.isObject()) {
          obj["error"] = "Invalid screenshot JSON";
        } else {
          obj = SaveScreenshot(json.object());
        }
      } else {
        obj["error"] = "Unrecognized message";
      }
      socket->sendTextMessage(QString::fromUtf8(
          QJsonDocument(obj).toJson(QJsonDocument::Compact)));
    });
  });
}

QJsonObject DebugMetaServer::SaveScreenshot(QJsonObject input) {
  // Extract the pieces and then do screenshot
  if (!input["x"].isDouble() || !input["y"].isDouble() ||
        !input["w"].isDouble() || !input["h"].isDouble() ||
        !input["fileName"].isString()) {
    return { { "error", "Invalid screenshot JSON" } };
  }
  auto pm = QGuiApplication::primaryScreen()->grabWindow(
        0, input["x"].toInt(), input["y"].toInt(),
        input["w"].toInt(), input["h"].toInt());
  if (!pm.save(input["fileName"].toString())) {
    return { { "error", "Failed saving screenshot" } };
  }
  return { { "success", true } };
}

}  // namespace doogie
