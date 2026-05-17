#include "ScanProgressDlg.h"
#include "ApiClient.h"

#include <QDialogButtonBox>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QJsonObject>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

#ifdef HAVE_TAGLIB
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tpropertymap.h>
#endif

namespace {
constexpr const char* kAudioExts[] = {"mp3","flac","wav","ogg","m4a","aac","opus","wma","aiff","alac"};
bool looksAudio(const QString& path) {
    QString ext = QFileInfo(path).suffix().toLower();
    for (const auto* e : kAudioExts) if (ext == e) return true;
    return false;
}
}  // namespace

ScanProgressDlg::ScanProgressDlg(const QStringList& paths, QWidget* parent)
    : QDialog(parent), m_paths(paths) {
    setWindowTitle("Scanning media");
    resize(640, 220);

    auto* v = new QVBoxLayout(this);
    m_progress = new QProgressBar();
    m_progress->setRange(0, 0);  // indeterminate until we expand the file list
    v->addWidget(m_progress);
    m_current = new QLabel("Expanding file list…");
    m_current->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_current->setWordWrap(true);
    v->addWidget(m_current);
    m_summary = new QLabel();
    v->addWidget(m_summary);

    auto* bb = new QDialogButtonBox(QDialogButtonBox::Cancel);
    v->addWidget(bb);
    m_cancel = bb->button(QDialogButtonBox::Cancel);
    connect(m_cancel, &QPushButton::clicked, this, &ScanProgressDlg::onCancel);

    // Defer expansion + work to next tick so the dialog paints first.
    QTimer::singleShot(0, this, [this]() {
        // Expand directories recursively, keep file order stable.
        for (const auto& p : m_paths) {
            QFileInfo fi(p);
            if (fi.isDir()) {
                QDirIterator it(p, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
                while (it.hasNext()) {
                    auto f = it.next();
                    if (looksAudio(f)) m_expanded << f;
                }
            } else if (fi.isFile() && looksAudio(p)) {
                m_expanded << p;
            }
        }
        m_progress->setRange(0, m_expanded.size());
        m_summary->setText(QString("Found %1 audio file(s).").arg(m_expanded.size()));
        if (m_expanded.isEmpty()) {
            m_current->setText("(no audio files found)");
            m_cancel->setText("Close");
            return;
        }
        // Drive one-at-a-time via timer so the UI stays responsive.
        QTimer::singleShot(0, this, &ScanProgressDlg::onTick);
    });
}

void ScanProgressDlg::onCancel() {
    if (m_cursor >= m_expanded.size()) {  // already done
        accept();
        return;
    }
    m_cancelled = true;
    reject();
}

void ScanProgressDlg::onTick() {
    if (m_cancelled) return;
    if (m_cursor >= m_expanded.size()) {
        m_current->setText(QString("Done. %1 imported, %2 skipped.")
                               .arg(m_okCount).arg(m_skipCount));
        m_cancel->setText("Close");
        m_cancel->disconnect();
        connect(m_cancel, &QPushButton::clicked, this, &QDialog::accept);
        m_progress->setValue(m_expanded.size());
        return;
    }
    auto path = m_expanded.at(m_cursor++);
    m_current->setText(path);
    m_progress->setValue(m_cursor);
    processOne(path);
    QTimer::singleShot(0, this, &ScanProgressDlg::onTick);
}

void ScanProgressDlg::processOne(const QString& path) {
    QJsonObject payload;
    payload.insert("file_path", path);
    QFileInfo fi(path);
    payload.insert("format", fi.suffix().toLower());
    payload.insert("file_size", fi.size());

#ifdef HAVE_TAGLIB
    TagLib::FileRef f(path.toLocal8Bit().constData());
    if (!f.isNull() && f.tag()) {
        payload.insert("title",  QString::fromStdWString(
            std::wstring(f.tag()->title().toCWString())).trimmed());
        payload.insert("artist", QString::fromStdWString(
            std::wstring(f.tag()->artist().toCWString())).trimmed());
        payload.insert("album",  QString::fromStdWString(
            std::wstring(f.tag()->album().toCWString())).trimmed());
        payload.insert("genre",  QString::fromStdWString(
            std::wstring(f.tag()->genre().toCWString())).trimmed());
        if (f.tag()->year() > 0) payload.insert("year", static_cast<int>(f.tag()->year()));
    }
    if (!f.isNull() && f.audioProperties()) {
        payload.insert("duration_sec", f.audioProperties()->lengthInSeconds());
    }
#else
    // No TagLib at build time — file path + size only.
    if (payload.value("title").toString().isEmpty()) {
        payload.insert("title", fi.completeBaseName());
    }
#endif

    if (!m_api) {
        ++m_skipCount;
        return;
    }
    m_api->upsertMyMedia(payload);
    ++m_okCount;
}
