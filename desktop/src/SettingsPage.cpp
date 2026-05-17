#include "SettingsPage.h"
#include "AddServerDlg.h"
#include "DbBackupDlg.h"
#include "DbMigrationWizardDlg.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTabWidget>
#include <QVBoxLayout>

SettingsPage::SettingsPage(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    m_tabs = new QTabWidget();
    m_tabs->addTab(buildServersTab(),     "Servers");
    m_tabs->addTab(buildApplicationTab(), "Application");
    m_tabs->addTab(buildDatabaseTab(),    "Database");
    m_tabs->addTab(buildAboutTab(),       "About");
    root->addWidget(m_tabs);
}

QWidget* SettingsPage::buildServersTab() {
    auto* w = new QWidget();
    auto* v = new QVBoxLayout(w);
    m_servers = new QTableWidget(0, 4);
    m_servers->setHorizontalHeaderLabels({"Name", "Type", "URL", "Mount"});
    m_servers->horizontalHeader()->setStretchLastSection(true);
    v->addWidget(m_servers, 1);
    auto* row = new QHBoxLayout();
    auto* add = new QPushButton("Add…");
    auto* edit = new QPushButton("Edit…");
    auto* rem = new QPushButton("Remove");
    row->addWidget(add); row->addWidget(edit); row->addWidget(rem); row->addStretch(1);
    v->addLayout(row);
    connect(add, &QPushButton::clicked, this, [this]() {
        AddServerDlg dlg(this); dlg.exec();
    });
    return w;
}

QWidget* SettingsPage::buildApplicationTab() {
    auto* w = new QWidget();
    auto* f = new QFormLayout(w);
    m_enableLog = new QCheckBox("Enable file logging");
    f->addRow(m_enableLog);
    m_logLevel = new QComboBox();
    m_logLevel->addItems({"trace", "debug", "info", "warn", "error"});
    f->addRow("Log level:", m_logLevel);
    auto* row = new QHBoxLayout();
    m_logPath = new QLineEdit();
    row->addWidget(m_logPath, 1);
    row->addWidget(new QPushButton("Browse…"));
    f->addRow("Log file:", row);
    return w;
}

QWidget* SettingsPage::buildDatabaseTab() {
    auto* w = new QWidget();
    auto* f = new QFormLayout(w);
    m_dbHost = new QLineEdit();  f->addRow("Host:",     m_dbHost);
    m_dbUser = new QLineEdit();  f->addRow("User:",     m_dbUser);
    m_dbPass = new QLineEdit();  m_dbPass->setEchoMode(QLineEdit::Password);
    f->addRow("Password:", m_dbPass);
    auto* row = new QHBoxLayout();
    m_dbTest    = new QPushButton("Test");
    m_dbInit    = new QPushButton("Initialize Schema");
    m_dbBackup  = new QPushButton("Backup…");
    m_dbMigrate = new QPushButton("Migration Wizard…");
    row->addWidget(m_dbTest);
    row->addWidget(m_dbInit);
    row->addWidget(m_dbBackup);
    row->addWidget(m_dbMigrate);
    row->addStretch(1);
    f->addRow(row);
    connect(m_dbBackup,  &QPushButton::clicked, this, [this]() {
        DbBackupDlg dlg(this); dlg.exec();
    });
    connect(m_dbMigrate, &QPushButton::clicked, this, [this]() {
        DbMigrationWizardDlg dlg(this); dlg.exec();
    });
    return w;
}

QWidget* SettingsPage::buildAboutTab() {
    auto* w = new QWidget();
    auto* v = new QVBoxLayout(w);
    auto* lbl = new QLabel(
        "<b>Mcaster1 TagStack</b><br>"
        "Version 0.1.0<br><br>"
        "© 2026 Dave St. John &lt;davestj@gmail.com&gt;<br>"
        "Licensed under the Mcaster1 Custom Proprietary License with AI Attribution.");
    lbl->setTextFormat(Qt::RichText);
    lbl->setAlignment(Qt::AlignTop);
    v->addWidget(lbl);
    v->addStretch(1);
    return w;
}
