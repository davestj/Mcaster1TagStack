#include "MetadataEditorDlg.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

MetadataEditorDlg::MetadataEditorDlg(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Edit Metadata");
    resize(520, 320);

    auto* v = new QVBoxLayout(this);
    m_path = new QLabel();
    m_path->setTextInteractionFlags(Qt::TextSelectableByMouse);
    v->addWidget(m_path);

    auto* f = new QFormLayout();
    m_artist = new QLineEdit();  f->addRow("Artist:",   m_artist);
    m_title  = new QLineEdit();  f->addRow("Title:",    m_title);
    m_album  = new QLineEdit();  f->addRow("Album:",    m_album);
    m_genre  = new QLineEdit();  f->addRow("Genre:",    m_genre);
    m_year   = new QLineEdit();  f->addRow("Year:",     m_year);
    m_source = new QLineEdit();  f->addRow("Source:",   m_source);
    m_duration = new QLabel("—"); f->addRow("Duration:", m_duration);
    v->addLayout(f);

    auto* bb = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    v->addWidget(bb);
    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void MetadataEditorDlg::setFilePath(const QString& path) {
    m_path->setText("<b>File:</b> " + path);
    // TODO: read existing tags via TagLib into the fields.
}
