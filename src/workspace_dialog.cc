#include "workspace_dialog.h"

namespace doogie {

WorkspaceDialog::WorkspaceDialog(QWidget* parent) : QDialog(parent) {
}

int WorkspaceDialog::execOpen(QList<Workspace> open_workspaces) {
  // Actually, we're not going to use this dialog here at all.
  // We're using an input dialog w/ drop down.
  QStringList select_from;
  auto workspaces = Workspace::Workspaces();
  for (auto& workspace : workspaces) {
    select_from.append(workspace.FriendlyName());
  }
  for (auto& open_workspace : open_workspaces) {
    select_from.removeOne(open_workspace.FriendlyName());
  }
  if (select_from.isEmpty()) {
    QMessageBox::information(nullptr, "Open Workspace",
                             "All workspaces are already open");
    return Rejected;
  }
  bool ok;
  auto res = QInputDialog::getItem(
        nullptr, "Open Workspace", "Workspace:", select_from, 0, false, &ok);
  if (!ok) return Rejected;
  for (auto& workspace : workspaces) {
    if (workspace.FriendlyName() == res) {
      selected_workspace_ = workspace;
      break;
    }
  }
  return Accepted;
}

int WorkspaceDialog::execManage(QList<Workspace> open_workspaces) {
  // Basically a name-editable (if not already open) list
  // with add/delete support
  QSet<qlonglong> open_ids;
  for (auto& open : open_workspaces) open_ids.insert(open.Id());

  setWindowTitle("Manage Workspaces");
  resize(500, height());
  auto layout = new QGridLayout;
  layout->setColumnStretch(0, 1);

  auto table = new QTableWidget;
  table->setColumnCount(2);
  table->setHorizontalHeaderLabels({ "Name", "Last Opened" });
  table->verticalHeader()->setVisible(false);
  table->setSelectionBehavior(QAbstractItemView::SelectRows);
  table->setSelectionMode(QAbstractItemView::ExtendedSelection);
  table->horizontalHeader()->setStretchLastSection(false);
  table->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::Stretch);
  table->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::ResizeToContents);
  auto workspaces = Workspace::Workspaces();
  QHash<qlonglong, Workspace> workspaces_by_id;
  for (auto& workspace : workspaces) {
    workspaces_by_id[workspace.Id()] = workspace;
    auto item = new QTableWidgetItem(workspace.FriendlyName());
    item->setData(Qt::UserRole + 1, workspace.Id());
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    if (open_ids.contains(workspace.Id())) {
      item->setText(item->text() + "*");
    } else {
      item->setFlags(item->flags() | Qt::ItemIsEditable);
    }
    auto row = table->rowCount();
    table->setRowCount(row + 1);
    table->setItem(row, 0, item);
    auto date_item = new QTableWidgetItem;
    date_item->setFlags(Qt::ItemIsEnabled);
    date_item->setData(Qt::EditRole, workspace.LastOpened());
    if (workspace.LastOpened() > 0) {
      auto date = QDateTime::fromMSecsSinceEpoch(
            workspace.LastOpened()).toLocalTime();
      date_item->setData(Qt::DisplayRole,
                         date.toString("ddd MM d yyyy hh:mm:ss ") +
                            date.timeZoneAbbreviation());
    } else {
      date_item->setData(Qt::DisplayRole, "");
    }
    table->setItem(row, 1, date_item);
  }
  table->setSortingEnabled(true);
  table->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  table->resizeColumnsToContents();
  layout->addWidget(table, 0, 0, 1, 3);
  layout->setRowStretch(0, 1);

  auto warn = new QLabel;
  if (!open_ids.isEmpty()) warn->setText("* Open workspaces cannot be edited");
  layout->addWidget(warn, 1, 0, 1, 3);

  auto add = new QPushButton("Add Workspace");
  layout->addWidget(add, 2, 0, 1, 1, Qt::AlignRight);
  auto edit = new QPushButton("Edit Selected Name");
  edit->setEnabled(false);
  layout->addWidget(edit, 2, 1);
  auto del = new QPushButton("Delete Selected");
  del->setEnabled(false);
  layout->addWidget(del, 2, 2);

  auto frame = new QFrame;
  frame->setFrameShape(QFrame::HLine);
  layout->addWidget(frame, 3, 0, 1, 3);

  auto ok = new QPushButton("OK");
  layout->addWidget(ok, 4, 1);
  connect(ok, &QPushButton::clicked, this, &WorkspaceDialog::accept);
  auto cancel = new QPushButton("Cancel");
  layout->addWidget(cancel, 4, 2);
  connect(cancel, &QPushButton::clicked, this, &WorkspaceDialog::reject);

  connect(add, &QPushButton::clicked, [=]() {
    table->setSortingEnabled(false);
    auto row = table->rowCount();
    table->setRowCount(row + 1);
    auto item = new QTableWidgetItem("");
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable |
                   Qt::ItemIsEditable);
    table->setItem(row, 0, item);
    table->editItem(item);
    table->setSortingEnabled(true);
  });
  connect(edit, &QPushButton::clicked, [=]() {
    table->editItem(table->selectedItems().first());
  });
  connect(del, &QPushButton::clicked, [=]() {
    for (auto item : table->selectedItems()) {
      table->removeRow(table->row(item));
    }
  });

  connect(table, &QTableWidget::itemSelectionChanged, [=]() {
    // If it's a single non-open one, then edit can be enabled
    auto edit_enabled = table->selectedItems().count() == 1;
    // If none of the selected are open, then del can be enabled
    auto delete_enabled = !table->selectedItems().isEmpty();
    for (auto item : table->selectedItems()) {
      auto id_data = item->data(Qt::UserRole + 1);
      if (id_data.type() == QVariant::LongLong) {
        if (open_ids.contains(id_data.toLongLong())) {
          edit_enabled = false;
          delete_enabled = false;
          break;
        }
      }
    }
    edit->setEnabled(edit_enabled);
    del->setEnabled(delete_enabled);
  });

  setLayout(layout);

  restoreGeometry(QSettings().value("manageWorkspaces/geom").toByteArray());

  bool failed_try_again;
  do {
    // Open it
    if (exec() == Rejected) {
      QSettings().setValue("manageWorkspaces/geom", saveGeometry());
      return Rejected;
    }

    // Validate and do the saving
    // No duplicate names
    QSet<QString> used_names;
    QSet<qlonglong> ids_still_there;
    QSet<QString> workspaces_to_add;
    QHash<qlonglong, QString> names_to_change;
    failed_try_again = false;
    for (int i = 0; i < table->rowCount(); i++) {
      auto item = table->item(i, 0);
      auto name = item->text();
      auto id_data = item->data(Qt::UserRole + 1);
      if (id_data.type() == QVariant::LongLong) {
        auto id = id_data.toLongLong();
        ids_still_there.insert(id);
        if (!open_ids.contains(id) &&
            name != workspaces_by_id[id].FriendlyName()) {
          names_to_change[id] = name;
        } else {
          // Use the old name (pre-asterisk)
          name = workspaces_by_id[id].FriendlyName();
        }
      } else {
        workspaces_to_add.insert(name);
      }

      if (used_names.contains(name)) {
        QMessageBox::warning(nullptr, "Manage Workspaces",
                             "Duplicate names found");
        failed_try_again = true;
        break;
      }
      used_names.insert(name);
    }
    if (!failed_try_again) {
      // Do the updating
      // Adds first
      for (auto str : workspaces_to_add) {
        Workspace workspace;
        workspace.SetName(str);
        workspace.Persist();
      }
      // Updates and deletes
      for (auto& workspace : workspaces) {
        if (!ids_still_there.contains(workspace.Id())) {
          workspace.Delete();
        } else if (names_to_change.contains(workspace.Id())) {
          workspace.SetName(names_to_change[workspace.Id()]);
          workspace.Persist();
        }
      }
    }
  } while (failed_try_again);
  QSettings().setValue("manageWorkspaces/geom", saveGeometry());

  delete layout;
  return Accepted;
}

}  // namespace doogie
