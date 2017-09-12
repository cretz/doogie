#ifndef DOOGIE_WORKSPACE_H_
#define DOOGIE_WORKSPACE_H_

#include <QtSql>
#include <QtWidgets>

namespace doogie {

// DB model for a workspace.
class Workspace {
 public:
  class WorkspacePage {
   public:
    static bool BubbleInUse(qlonglong bubble_id);
    static bool BubbleDeleted(qlonglong bubble_id);

    explicit WorkspacePage(qlonglong id = -1);
    explicit WorkspacePage(const QSqlRecord& record);

    qlonglong WorkspaceId() const { return workspace_id_; }
    void SetWorkspaceId(qlonglong workspace_id) {
      workspace_id_ = workspace_id;
    }

    bool Exists() const { return id_ >= 0; }
    qlonglong Id() const { return id_; }

    qlonglong ParentId() const { return parent_id_; }
    void SetParentId(qlonglong parent_id) { parent_id_ = parent_id; }

    int Pos() const { return pos_; }
    void SetPos(int pos) { pos_ = pos; }

    // This is not const because it is created lazily as needed
    QIcon Icon();
    void SetIcon(const QIcon& icon);

    QString Title() const { return title_; }
    void SetTitle(const QString& title) { title_ = title; }
    QString Url() const { return url_; }
    void SetUrl(const QString& url) { url_ = url; }
    qlonglong BubbleId() const { return bubble_id_; }
    void SetBubbleId(qlonglong bubble_id) { bubble_id_ = bubble_id; }
    bool Suspended() const { return suspended_; }
    void SetSuspended(bool suspended) { suspended_ = suspended; }
    bool Expanded() const { return expanded_; }
    void SetExpanded(bool expanded) { expanded_ = expanded; }

    bool Persist();
    // Automatically deletes children
    bool Delete();

   private:
    void FromRecord(const QSqlRecord& record);

    qlonglong workspace_id_ = -1;
    qlonglong id_ = -1;
    qlonglong parent_id_ = -1;
    int pos_ = 0;
    QIcon icon_;
    QByteArray serialized_icon_;
    QString title_ = "";
    QString url_ = "";
    qlonglong bubble_id_ = -1;
    bool suspended_ = false;
    bool expanded_ = false;
  };

  // Ordered by name
  static QList<Workspace> Workspaces();
  // Ordered by index
  static QList<Workspace> OpenWorkspaces();
  // Ordered by last opened desc
  static QList<Workspace> RecentWorkspaces(QSet<qlonglong> excluding,
                                           int count);
  // Note, this lazily creates one if it doesn't exist
  static Workspace DefaultWorkspace();

  static QString NextUnusedWorkspaceName();

  static bool NameInUse(const QString& name);

  static bool UpdateOpenWorkspaces(QList<qlonglong> ids);

  explicit Workspace(qlonglong id = -1);
  explicit Workspace(const QSqlRecord& record);

  bool Exists() const { return id_ >= 0; }
  qlonglong Id() const { return id_; }

  QString Name() const { return name_; }
  void SetName(const QString& name) { name_ = name; }
  QString FriendlyName() const { return name_.isEmpty() ? "Default" : name_; }

  qlonglong LastOpened() const { return last_opened_; }
  void SetLastOpened(qlonglong msecs) { last_opened_ = msecs; }

  bool Persist();
  // Automatically deletes children
  bool Delete();

  // Ordered by pos only
  QList<WorkspacePage> AllChildren() const;
  // Ordered by pos
  QList<WorkspacePage> ChildrenOf(qlonglong parent_id = -1) const;

 private:
  void FromRecord(const QSqlRecord& record);

  qlonglong id_ = -1;
  QString name_ = "";
  qlonglong last_opened_ = -1;
};

}  // namespace doogie

#endif  // DOOGIE_WORKSPACE_H_
