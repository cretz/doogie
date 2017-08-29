#ifndef DOOGIE_CEF_H_
#define DOOGIE_CEF_H_

#include <QtWidgets>

#include "cef/cef_app_handler.h"
#include "cef/cef_base.h"

namespace doogie {

class Cef {
 public:
  Cef(int argc, char* argv[]);
  ~Cef();
  // If >= 0, this is a child and not the UI window
  int EarlyExitCode() const;
  void Tick() const;

  CefRefPtr<CefAppHandler> AppHandler() const;

  std::function<void()> Download(
      const QString& url,
      QIODevice* write_to,
      std::function<void(CefRefPtr<CefURLRequest> request,
                         QIODevice* device)> download_complete = nullptr,
      std::function<void(CefRefPtr<CefURLRequest> request,
                         QIODevice* device)> download_data = nullptr,
      std::function<void(CefRefPtr<CefURLRequest> request,
                         uint64 current,
                         uint64 total)> download_progress = nullptr) const;

  std::function<void()> Download(
      CefRefPtr<CefRequest> request,
      QIODevice* write_to,
      std::function<void(CefRefPtr<CefURLRequest> request,
                         QIODevice* device)> download_complete = nullptr,
      std::function<void(CefRefPtr<CefURLRequest> request,
                         QIODevice* device)> download_data = nullptr,
      std::function<void(CefRefPtr<CefURLRequest> request,
                         uint64 current,
                         uint64 total)> download_progress = nullptr) const;

  bool ShowCertDialog(CefRefPtr<CefX509Certificate> cert) const;

 private:
  class CallbackFullDownload : public CefURLRequestClient {
   public:
    CallbackFullDownload(
        QIODevice* write_to,
        std::function<void(CefRefPtr<CefURLRequest> request,
                           QIODevice* device)> download_complete,
        std::function<void(CefRefPtr<CefURLRequest> request,
                           QIODevice* device)> download_data,
        std::function<void(CefRefPtr<CefURLRequest> request,
                           uint64 current,
                           uint64 total)> download_progress);

    void OnDownloadProgress(CefRefPtr<CefURLRequest> request,
                            int64 current,
                            int64 total) override;
    void OnDownloadData(CefRefPtr<CefURLRequest> request,
                        const void* data,
                        size_t data_length) override;
    void OnRequestComplete(CefRefPtr<CefURLRequest> request) override;
    void OnUploadProgress(CefRefPtr<CefURLRequest>,
                          int64,
                          int64) override { }
    bool GetAuthCredentials(bool,
                            const CefString&,
                            int,
                            const CefString&,
                            const CefString&,
                            CefRefPtr<CefAuthCallback>) override {
      return false;
    }

   private:
    QIODevice* write_to_;
    std::function<void(CefRefPtr<CefURLRequest> request,
                       QIODevice* device)> download_complete_;
    std::function<void(CefRefPtr<CefURLRequest> request,
                       QIODevice* device)> download_data_;
    std::function<void(CefRefPtr<CefURLRequest> request,
                       uint64 current,
                       uint64 total)> download_progress_;

    IMPLEMENT_REFCOUNTING(CallbackFullDownload)
  };

  cef_main_args_t MainArgs(int argc, char* argv[]);

  QByteArray CefBinToByteArray(CefRefPtr<CefBinaryValue> bin) const;

  int early_exit_code_;
  CefRefPtr<CefAppHandler> app_handler_;
};

}  // namespace doogie

#endif  // DOOGIE_CEF_H_
