#include "logging_dock.h"

#include "browser_widget.h"

namespace doogie {

LoggingDock::LoggingDock(QWidget* parent) : QDockWidget("Logs", parent) {
  text_edit_ = new QPlainTextEdit(this);
  text_edit_->setReadOnly(true);
  setWidget(text_edit_);
}

void LoggingDock::Log(const QString &str) {
  text_edit_->appendPlainText(str);
  text_edit_->verticalScrollBar()->setValue(
        text_edit_->verticalScrollBar()->maximum());
}

void LoggingDock::LogQtMessage(QtMsgType,
                               const QMessageLogContext&,
                               const QString& str) {
  Log(str);
}

void LoggingDock::LogWidgetMessage(const QString& str, QWidget* source) {
  // TODO(cretz): waiting on https://bitbucket.org/chromiumembedded/cef/issues/2154
  auto browser = qobject_cast<BrowserWidget*>(source);
  if (browser) {
    Log(browser->CurrentUrl() + ": " + str);
  } else {
    Log(str);
  }
}

}  // namespace doogie
