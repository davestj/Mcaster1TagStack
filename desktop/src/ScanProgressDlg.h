// ScanProgressDlg.h — Qt mirror of CScanProgressDlg (modal).
// Background TagLib metadata scanner with progress UI. The worker thread
// extracts artist/title/album/genre/duration and inserts into the local
// media library DB. v1 used a separately-compiled-with-/EHa thread to
// catch SEH exceptions from TagLib; Qt with -fexceptions is enough on
// POSIX since TagLib throws std::exceptions cleanly.

#pragma once
#include <QDialog>
#include <QStringList>
class ApiClient;
class QProgressBar;
class QLabel;
class QPushButton;
class QThread;

class ScanProgressDlg : public QDialog {
    Q_OBJECT
public:
    explicit ScanProgressDlg(const QStringList& paths, QWidget* parent = nullptr);
    void setApi(ApiClient* api) { m_api = api; }

private slots:
    void onTick();
    void onCancel();

private:
    void processOne(const QString& path);

    ApiClient*    m_api      = nullptr;
    QStringList   m_paths;       // initially-supplied files OR folders
    QStringList   m_expanded;    // post-expansion file list
    int           m_cursor    = 0;
    int           m_okCount   = 0;
    int           m_skipCount = 0;
    bool          m_cancelled = false;
    QProgressBar* m_progress = nullptr;
    QLabel*       m_current = nullptr;
    QLabel*       m_summary = nullptr;
    QPushButton*  m_cancel = nullptr;
};
