#ifndef DOOGIE_ACTION_MANAGER_H_
#define DOOGIE_ACTION_MANAGER_H_

#include <QtWidgets>

namespace doogie {

class MainWindow;

class ActionManager : public QObject {
  Q_OBJECT

 public:
  enum Type {
    NewTopLevelPage = 0,

    NewChildForegroundPage,
    NewChildBackgroundPage,
    Reload,
    Stop,
    Back,
    Forward,
    Print,
    ZoomIn,
    ZoomOut,
    ResetZoom,
    FindInPage,
    ExpandTree,
    CollapseTree,
    DuplicateTree,
    SelectSameHostPages,
    ClosePage,
    CloseTree,
    CloseSameHostPages,
    CloseOtherTrees,

    ReloadSelectedPages,
    ExpandSelectedTrees,
    CollapseSelectedTrees,
    DuplicateSelectedTrees,
    CloseSelectedPages,
    CloseSelectedTrees,
    CloseNonSelectedPages,
    CloseNonSelectedTrees,

    ReloadAllPages,
    ExpandAllTrees,
    CollapseAllTrees,
    CloseAllPages,

    ToggleDevTools,
    LogsWindow,
    FocusPageTree,
    FocusAddressBar,
    FocusBrowser,

    UserAction = 10000
  };
  Q_ENUM(Type)

  static ActionManager* Instance();
  static QAction* Action(int type);
  static void registerAction(int type, QAction* action);
  static void registerAction(int type, const QString& text);

 private:
  explicit ActionManager(QObject *parent = nullptr);
  void CreateActions();

  QHash<int, QAction*> actions_;
  static ActionManager* instance_;

  friend class MainWindow;
};

}  // namespace doogie

#endif  // DOOGIE_ACTION_MANAGER_H_
