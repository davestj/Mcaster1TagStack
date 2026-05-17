#include "SelectMountDlg.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

SelectMountDlg::SelectMountDlg(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Select Mount");
    resize(380, 140);
    auto* root = new QVBoxLayout(this);
    root->addWidget(new QLabel("Choose an active mount on the server:"));
    m_combo = new QComboBox();
    root->addWidget(m_combo);
    auto* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    root->addWidget(bb);
    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void SelectMountDlg::setMounts(const QStringList& mounts) {
    m_combo->clear();
    m_combo->addItems(mounts);
}

QString SelectMountDlg::selectedMount() const {
    return m_combo->currentText();
}
