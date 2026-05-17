#include "DbBackupDlg.h"

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QVBoxLayout>

DbBackupDlg::DbBackupDlg(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Database Backup");
    resize(640, 420);

    auto* v = new QVBoxLayout(this);
    auto* f = new QFormLayout();

    auto* row1 = new QHBoxLayout();
    m_mysqldump = new QLineEdit("/usr/bin/mysqldump");
    auto* b1 = new QPushButton("Browse…");
    row1->addWidget(m_mysqldump, 1); row1->addWidget(b1);
    f->addRow("mysqldump path:", row1);

    auto* row2 = new QHBoxLayout();
    m_outDir = new QLineEdit();
    auto* b2 = new QPushButton("Browse…");
    row2->addWidget(m_outDir, 1); row2->addWidget(b2);
    f->addRow("Output directory:", row2);

    m_filename = new QLineEdit("mcaster1_tagstack_%timestamp.sql");
    f->addRow("Filename pattern:", m_filename);
    v->addLayout(f);

    m_log = new QPlainTextEdit();
    m_log->setReadOnly(true);
    v->addWidget(m_log, 1);

    auto* bb = new QDialogButtonBox();
    m_runBtn   = bb->addButton("Run", QDialogButtonBox::AcceptRole);
    auto* close = bb->addButton(QDialogButtonBox::Close);
    v->addWidget(bb);
    connect(m_runBtn, &QPushButton::clicked, this, &DbBackupDlg::onRun);
    connect(close,    &QPushButton::clicked, this, &QDialog::reject);
    connect(b2, &QPushButton::clicked, this, [this]() {
        QString d = QFileDialog::getExistingDirectory(this, "Output directory");
        if (!d.isEmpty()) m_outDir->setText(d);
    });
}

void DbBackupDlg::onRun() {
    if (!m_proc) m_proc = new QProcess(this);
    m_log->appendPlainText("Running mysqldump (stub) …");
    // TODO: actually invoke m_proc->start(m_mysqldump->text(), {...}); stream
    //       readyReadStandardOutput / Error to m_log.
}
