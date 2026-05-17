#include "MetadataEditorDlg.h"
#include "ApiClient.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

MetadataEditorDlg::MetadataEditorDlg(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Edit Metadata");
    resize(520, 320);

    auto* v = new QVBoxLayout(this);
    m_path = new QLabel();
    m_path->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_path->setWordWrap(true);
    v->addWidget(m_path);

    auto* f = new QFormLayout();
    m_artist = new QLineEdit();  f->addRow("Artist:",   m_artist);
    m_title  = new QLineEdit();  f->addRow("Title:",    m_title);
    m_album  = new QLineEdit();  f->addRow("Album:",    m_album);
    m_genre  = new QLineEdit();  f->addRow("Genre:",    m_genre);
    m_year   = new QLineEdit();  f->addRow("Year:",     m_year);
    m_source = new QLineEdit();  f->addRow("Format:",   m_source);
    m_duration = new QLabel("—"); f->addRow("Duration:", m_duration);
    v->addLayout(f);

    auto* bb = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    v->addWidget(bb);
    connect(bb, &QDialogButtonBox::accepted, this, &MetadataEditorDlg::onSave);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void MetadataEditorDlg::load(const QJsonObject& row) {
    m_mediaId  = row.value("id").toVariant().toLongLong();
    m_filePath = row.value("file_path").toString();
    m_path->setText("<b>File:</b> " + m_filePath);
    m_title->setText(row.value("title").toString());
    m_artist->setText(row.value("artist").toString());
    m_album->setText(row.value("album").toString());
    m_genre->setText(row.value("genre").toString());
    int yr = row.value("year").toInt();
    m_year->setText(yr > 0 ? QString::number(yr) : "");
    m_source->setText(row.value("format").toString());
    int sec = row.value("duration_sec").toInt();
    m_duration->setText(sec > 0 ? QString::asprintf("%d:%02d", sec / 60, sec % 60) : "—");
}

void MetadataEditorDlg::onSave() {
    if (!m_api || m_filePath.isEmpty()) {
        reject();
        return;
    }
    QJsonObject p;
    // file_path is the natural key — daemon upserts on it.
    p.insert("file_path",    m_filePath);
    p.insert("title",        m_title->text().trimmed());
    p.insert("artist",       m_artist->text().trimmed());
    p.insert("album",        m_album->text().trimmed());
    p.insert("genre",        m_genre->text().trimmed());
    p.insert("format",       m_source->text().trimmed());
    bool yok = false; int y = m_year->text().toInt(&yok);
    if (yok) p.insert("year", y);
    m_api->upsertMyMedia(p);
    accept();
}
