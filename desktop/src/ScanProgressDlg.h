// ScanProgressDlg.h — Qt mirror of CScanProgressDlg (modal).
// Background TagLib metadata scanner with progress UI. The worker thread
// extracts artist/title/album/genre/duration and inserts into the local
// media library DB. v1 used a separately-compiled-with-/EHa thread to
// catch SEH exceptions from TagLib; Qt with -fexceptions is enough on
// POSIX since TagLib throws std::exceptions cleanly.

#pragma once
#include <QDialog>
#include <QStringList>
class QProgressBar;
class QLabel;
class QPushButton;
class QThread;

class ScanProgressDlg : public QDialog {
    Q_OBJECT
public:
    explicit ScanProgressDlg(const QStringList& paths, QWidget* parent = nullptr);

private:
    QStringList   m_paths;
    QProgressBar* m_progress = nullptr;
    QLabel*       m_current = nullptr;
    QLabel*       m_summary = nullptr;
    QPushButton*  m_cancel = nullptr;
};
