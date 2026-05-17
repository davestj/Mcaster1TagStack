#include "ScanProgressDlg.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>

ScanProgressDlg::ScanProgressDlg(const QStringList& paths, QWidget* parent)
    : QDialog(parent), m_paths(paths) {
    setWindowTitle("Scanning media");
    resize(520, 180);
    auto* v = new QVBoxLayout(this);
    m_progress = new QProgressBar();
    m_progress->setRange(0, paths.size() > 0 ? paths.size() : 1);
    v->addWidget(m_progress);
    m_current = new QLabel("Preparing…");
    m_current->setTextInteractionFlags(Qt::TextSelectableByMouse);
    v->addWidget(m_current);
    m_summary = new QLabel();
    v->addWidget(m_summary);
    auto* bb = new QDialogButtonBox(QDialogButtonBox::Cancel);
    v->addWidget(bb);
    m_cancel = bb->button(QDialogButtonBox::Cancel);
    connect(m_cancel, &QPushButton::clicked, this, &QDialog::reject);
    // TODO: kick off a QThread or QtConcurrent worker that walks m_paths,
    //       extracts TagLib metadata, and inserts into mcaster1_tagstack.media_files
    //       via the daemon's REST API (or libmariadb directly).
}
