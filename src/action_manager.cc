#include "action_manager.h"

namespace doogie {

ActionManager* ActionManager::instance_ = nullptr;

ActionManager* ActionManager::Instance() {
  return instance_;
}

void ActionManager::CreateInstance(QApplication* app) {
  new ActionManager(app);
}

const QMap<int, QAction*> ActionManager::Actions() {
  return instance_->actions_;
}

QAction* ActionManager::Action(int type) {
  return instance_->actions_[type];
}

QList<QKeySequence> ActionManager::DefaultShortcuts(int type) {
  return instance_->default_shortcuts_.value(type);
}

void ActionManager::RegisterAction(int type, QAction* action) {
  action->setParent(instance_);
  instance_->actions_[type] = action;
}

void ActionManager::RegisterAction(int type, const QString& text) {
  ActionManager::RegisterAction(type, new QAction(text));
}

QString ActionManager::TypeToString(int type) {
  const auto meta = QMetaEnum::fromType<Type>();
  auto str = meta.valueToKey(type);
  return str ? QString(str) : QString::number(type);
}

int ActionManager::StringToType(const QString& str) {
  const auto meta = QMetaEnum::fromType<Type>();
  auto type = meta.keyToValue(str.toUtf8().constData());
  if (type == -1) {
    bool ok;
    type = str.toInt(&ok);
    if (!ok) type = -1;
  }
  return type;
}

ActionManager::ActionManager(QObject* parent) : QObject(parent) {
  instance_ = this;
  CreateActions();
}

void ActionManager::CreateActions() {
  RegisterAction(NewWindow, "New Window");
  RegisterAction(NewTopLevelPage, "New Top-Level Page");
  RegisterAction(ProfileSettings, "Profile Settings");
  RegisterAction(ChangeProfile, "Change Profile");
  RegisterAction(NewChildForegroundPage, "New Child Foreground Page");
  RegisterAction(NewChildBackgroundPage, "New Child Background Page");
  RegisterAction(Reload, "Reload");
  RegisterAction(Stop, "Stop");
  RegisterAction(Back, "Back");
  RegisterAction(Forward, "Forward");
  RegisterAction(Print, "Print");
  RegisterAction(Fullscreen, "Fullscreen");
  RegisterAction(ZoomIn, "Zoom In");
  RegisterAction(ZoomOut, "Zoom Out");
  RegisterAction(ResetZoom, "Reset Zoom");
  RegisterAction(FindInPage, "Find in Page");
  RegisterAction(SuspendPage, "Suspend Current Page");
  RegisterAction(UnsuspendPage, "Unsuspend Current Page");
  RegisterAction(ExpandTree, "Expand Current Tree");
  RegisterAction(CollapseTree, "Collapse Current Tree");
  RegisterAction(DuplicateTree, "Duplicate Current Tree");
  RegisterAction(SelectSameHostPages, "Select Current Same-Host Pages");
  RegisterAction(ClosePage, "Close Current Page");
  RegisterAction(CloseTree, "Close Current Tree");
  RegisterAction(CloseSameHostPages, "Close Current Same-Host Pages");
  RegisterAction(CloseOtherTrees, "Close Non-Current Trees");

  RegisterAction(ReloadSelectedPages, "Reload Selected Pages");
  RegisterAction(SuspendSelectedPages, "Suspend Selected Pages");
  RegisterAction(UnsuspendSelectedPages, "Unsuspend Selected Pages");
  RegisterAction(ExpandSelectedTrees, "Expand Selected Trees");
  RegisterAction(CollapseSelectedTrees, "Collapse Selected Trees");
  RegisterAction(DuplicateSelectedTrees, "Duplicate Selected Trees");
  RegisterAction(CloseSelectedPages, "Close Selected Pages");
  RegisterAction(CloseSelectedTrees, "Close Selected Trees");
  RegisterAction(CloseNonSelectedPages, "Close Non-Selected Pages");
  RegisterAction(CloseNonSelectedTrees, "Close Non-Selected Trees");

  RegisterAction(ReloadAllPages, "Reload All Pages");
  RegisterAction(SuspendAllPages, "Suspend All Pages");
  RegisterAction(UnsuspendAllPages, "Unsuspend All Pages");
  RegisterAction(ExpandAllTrees, "Expand All Trees");
  RegisterAction(CollapseAllTrees, "Collapse All Trees");
  RegisterAction(CloseAllPages, "Close All Pages");

  RegisterAction(NextPage, "Next Page");
  RegisterAction(PreviousPage, "Previous Page");

  RegisterAction(ToggleDevTools, "Toggle Dev Tools");
  RegisterAction(LogsWindow, "Logs");
  RegisterAction(DownloadsWindow, "Downloads");
  RegisterAction(BlockerWindow, "Request Blocker");
  RegisterAction(FocusPageTree, "Focus Page Tree");
  RegisterAction(FocusAddressBar, "Focus Address Bar");
  RegisterAction(FocusBrowser, "Focus Browser");

  RegisterAction(NewWorkspace, "New Workspace");
  RegisterAction(ManageWorkspaces, "Manage Workspaces");
  RegisterAction(OpenWorkspace, "Open Workspace");

  // Default shortcuts
  auto shortcuts = [=](ActionManager::Type type, const QString& shortcuts) {
    default_shortcuts_[type] = QKeySequence::listFromString(shortcuts);
  };
  shortcuts(ActionManager::NewWindow, "Ctrl+N");
  shortcuts(ActionManager::NewTopLevelPage, "Ctrl+T");
  shortcuts(ActionManager::NewChildForegroundPage, "Ctrl+Shift+T");
  shortcuts(ActionManager::ClosePage, "Ctrl+F4; Ctrl+W");
  shortcuts(ActionManager::CloseAllPages, "Ctrl+Shift+F4; Ctrl+Shift+W");
  shortcuts(ActionManager::NextPage, "Ctrl+Down; Ctrl+Tab");
  shortcuts(ActionManager::PreviousPage, "Ctrl+Up; Ctrl+Shift+Tab");
  shortcuts(ActionManager::ToggleDevTools, "F12; Ctrl+F12");
  shortcuts(ActionManager::Reload, "F5; Ctrl+F5; Ctrl+R");
  shortcuts(ActionManager::Stop, "Esc");
  shortcuts(ActionManager::Back,
            "Ctrl+Page Up; Ctrl+Shift+Page Down; Backspace");
  shortcuts(ActionManager::Forward,
            "Ctrl+Page Down; Ctrl+Shift+Page Up; Shift+Backspace");
  shortcuts(ActionManager::Print, "Ctrl+P");
  shortcuts(ActionManager::Fullscreen, "F11");
  shortcuts(ActionManager::ZoomIn, "Ctrl++; Ctrl+=");
  shortcuts(ActionManager::ZoomOut, "Ctrl+-");
  shortcuts(ActionManager::ResetZoom, "Ctrl+0");
  shortcuts(ActionManager::FindInPage, "Ctrl+F");
  shortcuts(ActionManager::FocusPageTree, "Alt+1");
  shortcuts(ActionManager::FocusAddressBar, "Alt+2; Alt+D");
  shortcuts(ActionManager::FocusBrowser, "Alt+3");
}

}  // namespace doogie
