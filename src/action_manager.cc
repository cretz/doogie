#include "action_manager.h"

namespace doogie {

ActionManager* ActionManager::instance_ = nullptr;

ActionManager* ActionManager::Instance() {
  return instance_;
}

QAction* ActionManager::Action(int type) {
  return instance_->actions_[type];
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
  registerAction(NewTopLevelPage, "New Top-Level Page");
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
  registerAction(ExpandTree, "Expand Current Tree");
  registerAction(CollapseTree, "Collapse Current Tree");
  registerAction(DuplicateTree, "Duplicate Current Tree");
  registerAction(SelectSameHostPages, "Select Current Same-Host Pages");
  registerAction(ClosePage, "Close Current Page");
  registerAction(CloseTree, "Close Current Tree");
  registerAction(CloseSameHostPages, "Close Current Same-Host Pages");
  registerAction(CloseOtherTrees, "Close Non-Current Trees");

  registerAction(ReloadSelectedPages, "Reload Selected Pages");
  registerAction(ExpandSelectedTrees, "Expand Selected Trees");
  registerAction(CollapseSelectedTrees, "Collapse Selected Trees");
  registerAction(DuplicateSelectedTrees, "Duplicate Selected Trees");
  registerAction(CloseSelectedPages, "Close Selected Pages");
  registerAction(CloseSelectedTrees, "Close Selected Trees");
  registerAction(CloseNonSelectedPages, "Close Non-Selected Pages");
  registerAction(CloseNonSelectedTrees, "Close Non-Selected Trees");

  registerAction(ReloadAllPages, "Reload All Pages");
  registerAction(ExpandAllTrees, "Expand All Trees");
  registerAction(CollapseAllTrees, "Collapse All Trees");
  registerAction(CloseAllPages, "Close All Pages");

  registerAction(ToggleDevTools, "Toggle Dev Tools");
  registerAction(LogsWindow, "Logs");
  registerAction(FocusPageTree, "Focus Page Tree");
  registerAction(FocusAddressBar, "Focus Address Bar");
  registerAction(FocusBrowser, "Focus Browser");
}

}  // namespace doogie
