#include "updater.h"

#include "profile.h"

namespace doogie {

Updater::Updater(const Cef& cef, QObject* parent)
    : QObject(parent), cef_(cef) {

  ApplyCrlFromFileAndScheduleUpdate();
}

const QString Updater::kCrlCheckUrl =
    "http://clients2.google.com/service/update2/crx"
    "?x=id%3Dhfnkpimlhhgieaddgfemjhofmfblmnib%26v%3D%26uc";

void Updater::ApplyCrlFromFileAndScheduleUpdate() const {
  auto path = CrlFilePath();
  auto msecs_to_next = 0ll;
  if (QFile(path).exists()) {
    qDebug() << "Loading CRL from" << path;
    CefLoadCRLSetsFile(CefString(path.toStdString()));
    // Now set a timer to do this again after certain amount of time
    auto next = QFileInfo(path).lastModified().
        addSecs(kCheckCrlUpdateFrequencySeconds);
    msecs_to_next = QDateTime::currentDateTime().msecsTo(next);
  }
  // At least wait a second
  if (msecs_to_next < 1000) msecs_to_next = 1000;
  qDebug() << "Scheduling next CRL update in ms:" << msecs_to_next;
  QTimer::singleShot(msecs_to_next, this, &Updater::CheckCrlUpdates);
}

QString Updater::CrlFilePath() const {
  return QDir::toNativeSeparators(
        QDir(Profile::kDoogieAppDataPath).filePath("crl-set"));
}

void Updater::CheckCrlUpdates() const {
  // First have to fetch the URL. Yes we ignore version and only use
  // file timestamp because it's easier to not store version state.
  auto buf = new QBuffer;
  buf->open(QIODevice::ReadWrite);
  cef_.Download(kCrlCheckUrl, buf,
                [=](CefRefPtr<CefURLRequest> req, QIODevice* device) {
    auto failed_retry = [=](const QString& reason) {
      qWarning() << "Failed CRL update, reason:" << reason;
      QTimer::singleShot(kCrlUpdateFailRetrySeconds * 1000,
                         this, &Updater::CheckCrlUpdates);
    };
    if (req->GetRequestStatus() != UR_SUCCESS) {
      failed_retry("Download failure");
      return;
    }

    // We'd do a lot better streaming this, but meh, it's not a big deal
    auto buf = qobject_cast<QBuffer*>(device);
    buf->seek(0);
    // It's mine to delete
    buf->deleteLater();
    auto data = reinterpret_cast<void*>(
          const_cast<char*>(buf->data().constData()));
    auto stream = CefStreamReader::CreateForData(data, buf->data().size());
    if (!stream) {
      failed_retry("Bad XML data");
      return;
    }
    // Find //gupdate/app/updatecheck
    CefRefPtr<CefXmlObject> elem = new CefXmlObject("temp");
    if (!elem->Load(stream,
                    XML_ENCODING_UTF8,
                    CefString("http://www.google.com/update2/response"),
                    nullptr)) {
      failed_retry("Bad xml");
      return;
    }
    elem = elem->FindChild("gupdate");
    if (elem) elem = elem->FindChild("app");
    if (elem) elem = elem->FindChild("updatecheck");
    QString crx_url;
    if (elem) {
      crx_url = QString::fromStdString(elem->GetAttributeValue("codebase"));
    }
    if (crx_url.isEmpty()) {
      failed_retry("No update info");
      return;
    }
    // Do the update back on the main thread
    Util::RunOnMainThread([=]() { UpdateCrl(crx_url); });
  });
}

void Updater::UpdateCrl(const QString& crx_url) const {
  qDebug() << "Updating CRLs from" << crx_url;

  // Get the CRX file in mem, load as ZIP, extract crl-set out of it and
  //  save it
  auto buf = new QBuffer;
  buf->open(QIODevice::ReadWrite);
  cef_.Download(crx_url, buf,
                [=](CefRefPtr<CefURLRequest> /*req*/, QIODevice* device) {
    auto failed_retry = [=](const QString& reason) {
      qWarning() << "Failed CRL update, reason:" << reason;
      QTimer::singleShot(kCrlUpdateFailRetrySeconds * 1000,
                         this, &Updater::CheckCrlUpdates);
    };
    // We'd do a lot better streaming this, but meh, it's not a big deal
    auto buf = qobject_cast<QBuffer*>(device);
    buf->seek(0);
    // It's mine to delete
    buf->deleteLater();
    auto data = reinterpret_cast<void*>(
          const_cast<char*>(buf->data().constData()));
    auto stream = CefStreamReader::CreateForData(data, buf->data().size());
    if (!stream) {
      failed_retry("Bad CRX data");
      return;
    }
    // Load the zip
    CefRefPtr<CefZipArchive> archive = new CefZipArchive();
    if (!archive ||
        archive->Load(stream, CefString(), false) <= 0 ||
        !archive->HasFile("crl-set")) {
      failed_retry("Bad CRX zip");
      return;
    }
    // Write crl-set to local file
    auto file = archive->GetFile("crl-set");
    QFile local_file(CrlFilePath());
    if (!local_file.open(QFile::WriteOnly)) {
      failed_retry("Failed local file open");
      return;
    }
    if (local_file.write(reinterpret_cast<const char*>(file->GetData()),
                         file->GetDataSize()) < 0) {
      failed_retry("Failed local file write");
      return;
    }
    local_file.close();
    // Apply back on main thread
    Util::RunOnMainThread([=]() { ApplyCrlFromFileAndScheduleUpdate(); });
  });
}

}   // namespace doogie
