#ifndef DOOGIE_WORKSPACE_H_
#define DOOGIE_WORKSPACE_H_

#include <QtSql>
#include <QtWidgets>

namespace doogie {

class Workspace {
 public:

  class WorkspacePage {
   public:
    WorkspacePage(qlonglong id = -1);
    WorkspacePage(const QSqlRecord& record);

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
    QString Bubble() const { return bubble_; }
    void SetBubble(const QString& bubble) { bubble_ = bubble; }
    bool Suspended() const { return suspended_; }
    void SetSuspended(bool suspended) { suspended_ = suspended; }
    bool Expanded() const { return expanded_; }
    void SetExpanded(bool expanded) { expanded_ = expanded; }

    bool Save();
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
    QString bubble_ = "";
    bool suspended_ = false;
    bool expanded_ = false;
  };

  // Ordered by name
  static QList<Workspace> Workspaces();
  // Ordered by last opened desc
  static QList<Workspace> RecentWorkspaces(QSet<qlonglong> excluding,
                                           int count);
  // Note, this lazily creates one if it doesn't exist
  static Workspace DefaultWorkspace();

  static QString NextUnusedWorkspaceName();

  static bool NameInUse(const QString& name);

  Workspace(qlonglong id = -1);
  Workspace(const QSqlRecord& record);

  bool Exists() const { return id_ >= 0; }
  qlonglong Id() const { return id_; }

  QString Name() const { return name_; }
  void SetName(const QString& name) { name_ = name; }
  QString FriendlyName() const { return name_.isEmpty() ? "Default" : name_; }

  qlonglong LastOpened() const { return last_opened_; }
  void SetLastOpened(qlonglong msecs) { last_opened_ = msecs; }

  bool Save();
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
