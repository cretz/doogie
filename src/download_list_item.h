#ifndef DOOGIE_DOWNLOAD_LIST_ITEM_H_
#define DOOGIE_DOWNLOAD_LIST_ITEM_H_

#include <QtWidgets>

#include "download.h"

namespace doogie {

// Item present in the download list.
class DownloadListItem : public QListWidgetItem {
 public:
  DownloadListItem();
  void AfterAdded();
  // Returns false if this is not the right item for it
  bool UpdateDownload(const Download& download);
  void ApplyContextMenu(QMenu* menu);
  void DeleteAndRemoveIfDone();
  bool DownloadActive();
 private:
  Download download_;

  QLabel* name_label_;
  QProgressBar* progress_bar_;
  QLabel* sub_name_label_;
  QToolButton* pause_button_;
  QToolButton* resume_button_;
  QToolButton* cancel_button_;
  QToolButton* open_folder_button_;
};

}  // namespace doogie

#endif  // DOOGIE_DOWNLOAD_LIST_ITEM_H_
