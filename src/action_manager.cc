#include "action_manager.h"

namespace doogie {

ActionManager* ActionManager::instance_ = nullptr;

ActionManager* ActionManager::Instance() {
  return instance_;
}

void ActionManager::CreateInstance(QApplication* app) {
  new ActionManager(app);
}

QAction* ActionManager::Action(int type) {
  return instance_->actions_[type];
}

QList<QKeySequence> ActionManager::DefaultShortcuts(int type) {
  return instance_->default_shortcuts_.value(type);
}

void ActionManager::registerAction(int type, QAction* action) {
  action->setParent(instance_);
  instance_->actions_[type] = action;
}

void ActionManager::registerAction(int type, const QString& text) {
  ActionManager::registerAction(type, new QAction(text));
}

ActionManager::ActionManager(QObject *parent) : QObject(parent) {
  instance_ = this;
  CreateActions();
}

void ActionManager::CreateActions() {
  registerAction(NewWindow, "New Window");
  registerAction(NewTopLevelPage, "New Top-Level Page");
  registerAction(ProfileSettings, "Profile Settings");
  registerAction(ChangeProfile, "Change Profile");
  registerAction(NewChildForegroundPage, "New Child Foreground Page");
  registerAction(NewChildBackgroundPage, "New Child Background Page");
  registerAction(Reload, "Reload");
  registerAction(Stop, "Stop");
  registerAction(Back, "Back");
  registerAction(Forward, "Forward");
  registerAction(Print, "Print");
  registerAction(ZoomIn, "Zoom In");
  registerAction(ZoomOut, "Zoom Out");
  registerAction(ResetZoom, "Reset Zoom");
  registerAction(FindInPage, "Find in Page");
  registerAction(SuspendPage, "Suspend Current Page");
  registerAction(UnsuspendPage, "Unsuspend Current Page");
  registerAction(ExpandTree, "Expand Current Tree");
  registerAction(CollapseTree, "Collapse Current Tree");
  registerAction(DuplicateTree, "Duplicate Current Tree");
  registerAction(SelectSameHostPages, "Select Current Same-Host Pages");
  registerAction(ClosePage, "Close Current Page");
  registerAction(CloseTree, "Close Current Tree");
  registerAction(CloseSameHostPages, "Close Current Same-Host Pages");
  registerAction(CloseOtherTrees, "Close Non-Current Trees");

  registerAction(ReloadSelectedPages, "Reload Selected Pages");  
  registerAction(SuspendSelectedPages, "Suspend Selected Pages");
  registerAction(UnsuspendSelectedPages, "Unsuspend Selected Pages");
  registerAction(ExpandSelectedTrees, "Expand Selected Trees");
  registerAction(CollapseSelectedTrees, "Collapse Selected Trees");
  registerAction(DuplicateSelectedTrees, "Duplicate Selected Trees");
  registerAction(CloseSelectedPages, "Close Selected Pages");
  registerAction(CloseSelectedTrees, "Close Selected Trees");
  registerAction(CloseNonSelectedPages, "Close Non-Selected Pages");
  registerAction(CloseNonSelectedTrees, "Close Non-Selected Trees");

  registerAction(ReloadAllPages, "Reload All Pages");
  registerAction(SuspendAllPages, "Suspend All Pages");
  registerAction(UnsuspendAllPages, "Unsuspend All Pages");
  registerAction(ExpandAllTrees, "Expand All Trees");
  registerAction(CollapseAllTrees, "Collapse All Trees");
  registerAction(CloseAllPages, "Close All Pages");

  registerAction(ToggleDevTools, "Toggle Dev Tools");
  registerAction(LogsWindow, "Logs");
  registerAction(FocusPageTree, "Focus Page Tree");
  registerAction(FocusAddressBar, "Focus Address Bar");
  registerAction(FocusBrowser, "Focus Browser");

  // Default shortcuts
  auto shortcuts = [this](ActionManager::Type type, const QString& shortcuts) {
    default_shortcuts_[type] = QKeySequence::listFromString(shortcuts);
  };
  shortcuts(ActionManager::NewWindow, "Ctrl+N");
  shortcuts(ActionManager::NewTopLevelPage, "Ctrl+T");
  shortcuts(ActionManager::NewChildForegroundPage, "Ctrl+Shift+T");
  shortcuts(ActionManager::ClosePage, "Ctrl+F4; Ctrl+W");
  shortcuts(ActionManager::CloseAllPages, "Ctrl+Shift+F4; Ctrl+Shift+W");
  shortcuts(ActionManager::ToggleDevTools, "F12; Ctrl+F12");
  shortcuts(ActionManager::Reload, "F5; Ctrl+F5; Ctrl+R");
  shortcuts(ActionManager::Stop, "Esc");
  shortcuts(ActionManager::Back,
            "Ctrl+Page Up; Ctrl+Shift+Page Down; Backspace");
  shortcuts(ActionManager::Forward,
            "Ctrl+Page Down; Ctrl+Shift+Page Up; Shift+Backspace");
  shortcuts(ActionManager::Print, "Ctrl+P");
  shortcuts(ActionManager::ZoomIn, "Ctrl++; Ctrl+=");
  shortcuts(ActionManager::ZoomOut, "Ctrl+-");
  shortcuts(ActionManager::ResetZoom, "Ctrl+0");
  shortcuts(ActionManager::FindInPage, "Ctrl+F");
  shortcuts(ActionManager::FocusPageTree, "Alt+1");
  shortcuts(ActionManager::FocusAddressBar, "Alt+2; Ctrl+D");
  shortcuts(ActionManager::FocusBrowser, "Alt+3");
}

}  // namespace doogie
