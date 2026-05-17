#include "DbMigrationWizardDlg.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

DbMigrationWizardDlg::DbMigrationWizardDlg(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Database Migration Wizard");
    resize(640, 460);

    auto* v = new QVBoxLayout(this);

    v->addWidget(new QLabel("Pending migrations (sql/NNN_*.sql, newest last):"));
    m_pending = new QComboBox();
    v->addWidget(m_pending);

    v->addWidget(new QLabel("Applied history (from migration_history table):"));
    m_applied = new QListWidget();
    v->addWidget(m_applied, 1);

    m_status = new QLabel("Click Refresh to scan sql/ and the migration_history table.");
    v->addWidget(m_status);

    auto* row = new QHBoxLayout();
    auto* refresh = new QPushButton("Refresh");
    m_btnOne = new QPushButton("Apply One");
    m_btnAll = new QPushButton("Apply All");
    row->addWidget(refresh);
    row->addStretch(1);
    row->addWidget(m_btnOne);
    row->addWidget(m_btnAll);
    v->addLayout(row);

    auto* bb = new QDialogButtonBox(QDialogButtonBox::Close);
    v->addWidget(bb);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(refresh,  &QPushButton::clicked, this, &DbMigrationWizardDlg::onRefresh);
    connect(m_btnOne, &QPushButton::clicked, this, &DbMigrationWizardDlg::onApplyOne);
    connect(m_btnAll, &QPushButton::clicked, this, &DbMigrationWizardDlg::onApplyAll);
}

void DbMigrationWizardDlg::onRefresh() {
    m_status->setText("(stub) — scan sql/ + query migration_history");
}

void DbMigrationWizardDlg::onApplyOne() {
    m_status->setText("(stub) — apply selected sql file via scripts/migrate.sh equivalent");
}

void DbMigrationWizardDlg::onApplyAll() {
    m_status->setText("(stub) — apply every pending migration in numeric order");
}
