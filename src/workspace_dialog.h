#ifndef DOOGIE_WORKSPACE_DIALOG_H_
#define DOOGIE_WORKSPACE_DIALOG_H_

#include <QtWidgets>
#include "workspace.h"

namespace doogie {

// Dialog handlers for selecting and managing workspaces.
class WorkspaceDialog : public QDialog {
  Q_OBJECT
 public:
  explicit WorkspaceDialog(QWidget* parent = nullptr);

  Workspace SelectedWorkspace() const { return selected_workspace_; }

  int execOpen(QList<Workspace> open_workspaces);
  int execManage(QList<Workspace> open_workspaces);

 private:
  Workspace selected_workspace_;
};

}  // namespace doogie

#endif  // DOOGIE_WORKSPACE_DIALOG_H_
