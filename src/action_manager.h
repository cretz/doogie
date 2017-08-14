#ifndef DOOGIE_ACTION_MANAGER_H_
#define DOOGIE_ACTION_MANAGER_H_

#include <QtWidgets>

namespace doogie {

class MainWindow;

class ActionManager : public QObject {
  Q_OBJECT

 public:
  enum Type {
    NewWindow = 0,
    NewTopLevelPage,
    ProfileSettings,
    ChangeProfile,

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
    SuspendPage,
    UnsuspendPage,
    ExpandTree,
    CollapseTree,
    DuplicateTree,
    SelectSameHostPages,
    ClosePage,
    CloseTree,
    CloseSameHostPages,
    CloseOtherTrees,

    ReloadSelectedPages,
    SuspendSelectedPages,
    UnsuspendSelectedPages,
    ExpandSelectedTrees,
    CollapseSelectedTrees,
    DuplicateSelectedTrees,
    CloseSelectedPages,
    CloseSelectedTrees,
    CloseNonSelectedPages,
    CloseNonSelectedTrees,

    ReloadAllPages,
    SuspendAllPages,
    UnsuspendAllPages,
    ExpandAllTrees,
    CollapseAllTrees,
    CloseAllPages,

    ToggleDevTools,
    LogsWindow,
    DownloadsWindow,
    FocusPageTree,
    FocusAddressBar,
    FocusBrowser,

    NewWorkspace,
    ManageWorkspaces,
    OpenWorkspace,

    UserAction = 10000
  };
  Q_ENUM(Type)

  static ActionManager* Instance();
  static void CreateInstance(QApplication* app);
  static const QMap<int, QAction*> Actions();
  static QAction* Action(int type);
  static QList<QKeySequence> DefaultShortcuts(int type);
  static void RegisterAction(int type, QAction* action);
  static void RegisterAction(int type, const QString& text);
  // If unknown, just converts int to string
  static QString TypeToString(int type);
  // If not found, will attempt to convert to int. If unsuccessful, return -1.
  static int StringToType(const QString& str);

 private:
  explicit ActionManager(QObject* parent = nullptr);
  void CreateActions();

  static ActionManager* instance_;
  QMap<int, QAction*> actions_;
  QHash<int, QList<QKeySequence>> default_shortcuts_;
};

}  // namespace doogie

#endif  // DOOGIE_ACTION_MANAGER_H_
