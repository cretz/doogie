#include "downloads_dock.h"

#include "download_list_item.h"

namespace doogie {

DownloadsDock::DownloadsDock(BrowserStack* browser_stack, QWidget* parent)
    : QDockWidget("Downloads", parent) {

  setFeatures(QDockWidget::AllDockWidgetFeatures);

  auto layout = new QVBoxLayout;

  auto button_layout = new QHBoxLayout;
  auto remove_all_button = new QToolButton;
  remove_all_button->setText("Remove All Finished");
  remove_all_button->setAutoRaise(true);
  button_layout->addWidget(remove_all_button);
  button_layout->addStretch(1);
  layout->addLayout(button_layout);

  auto list = new QListWidget;
  list->setResizeMode(QListView::Adjust);
  list->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(list, &QListWidget::customContextMenuRequested,
          [=](const QPoint& pos) {
    QMenu menu;
    auto item = static_cast<DownloadListItem*>(list->itemAt(pos));
    if (item) {
      item->ApplyContextMenu(&menu);
      menu.addSeparator();
    }
    menu.addAction("Remove All Finished",
                   [=]() { remove_all_button->click(); });
    menu.exec(list->mapToGlobal(pos));
  });
  layout->addWidget(list, 1);

  auto widg = new QWidget;
  widg->setLayout(layout);
  setWidget(widg);

  auto add_or_update_download = [=](
      const Download& download,
      bool force_add = false) {
    if (!force_add) {
      for (int i = 0; i < list->count(); i++) {
        auto item = static_cast<DownloadListItem*>(list->item(i));
        if (item->UpdateDownload(download)) return;
      }
    }
    auto item = new DownloadListItem;
    list->insertItem(0, item);
    item->AfterAdded();
    item->UpdateDownload(download);
  };

  // Load all of the existing downloads
  for (auto& download : Download::Downloads()) {
    add_or_update_download(download, true);
  }

  connect(remove_all_button, &QToolButton::clicked, [=]() {
    // Get all items, then delete em
    QList<DownloadListItem*> to_delete;
    for (int i = 0; i < list->count(); i++) {
      to_delete.append(static_cast<DownloadListItem*>(list->item(i)));
    }
    for (auto item : to_delete) {
      item->DeleteAndRemoveIfDone();
    }
  });

  // Connect to browser stack to handle downloads
  connect(browser_stack, &BrowserStack::DownloadRequested,
      [=](const Download& download,
          CefRefPtr<CefBeforeDownloadCallback> callback) {
    auto suggested_dir =
        QSettings().value("downloadsDock/saveDir").toString();
    if (suggested_dir.isEmpty()) {
      suggested_dir = QStandardPaths::writableLocation(
            QStandardPaths::DownloadLocation);
    }
    auto suggested_file_name = download.SuggestedFileName();
    if (suggested_file_name.isEmpty()) {
      // Use the end of the URL then
      suggested_file_name = QUrl(download.OrigUrl()).fileName();
    }
    auto save_name = QFileDialog::getSaveFileName(
          nullptr, "Save Download As...",
          QDir(suggested_dir).filePath(suggested_file_name));
    if (save_name.isEmpty()) return;
    // We have to delete it if it's present
    {
      QFile file(save_name);
      if (file.exists() && !file.remove()) {
        QMessageBox::critical(nullptr, "Save Error",
                              "Unable to delete existing file");
        return;
      }
    }
    QSettings().setValue("downloadsDock/saveDir",
                         QFileInfo(save_name).dir().path());
    callback->Continue(CefString(QDir::toNativeSeparators(save_name).toStdString()), false);
  });
  connect(browser_stack, &BrowserStack::DownloadUpdated,
          [=](const Download& download) {
    // We ignore downloads w/out a path (they force em to start early)
    if (!download.Path().isEmpty()) add_or_update_download(download);
  });
}

}  // namespace doogie
