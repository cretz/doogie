#include "workspace_dialog.h"

namespace doogie {

WorkspaceDialog::WorkspaceDialog(QWidget* parent) : QDialog(parent) {

}

int WorkspaceDialog::execOpen() {
  return Rejected;
}

int WorkspaceDialog::execManage() {
  return Rejected;
}

}  // namespace doogie
