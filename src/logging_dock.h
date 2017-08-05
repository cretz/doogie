#ifndef DOOGIE_LOGGING_DOCK_H_
#define DOOGIE_LOGGING_DOCK_H_

#include <QtWidgets>

namespace doogie {

class LoggingDock : public QDockWidget {
  Q_OBJECT

 public:
  explicit LoggingDock(QWidget* parent = nullptr);
  void Log(const QString& str);
  void LogQtMessage(QtMsgType type,
                    const QMessageLogContext& ctx,
                    const QString& str);
  void LogWidgetMessage(const QString& str, QWidget* source);

 private:
  QPlainTextEdit* text_edit_;
};

}  // namespace doogie

#endif  // DOOGIE_LOGGING_DOCK_H_
