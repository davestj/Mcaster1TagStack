#include "ServersPage.h"
#include "AddServerDlg.h"
#include "SelectMountDlg.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

ServersPage::ServersPage(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);

    m_serverTable = new QTableWidget(0, 5, this);
    m_serverTable->setHorizontalHeaderLabels(
        {"Name", "Type", "URL", "Mount", "Status"});
    m_serverTable->horizontalHeader()->setStretchLastSection(true);
    m_serverTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_serverTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    root->addWidget(m_serverTable, 1);

    auto* btnRow = new QHBoxLayout();
    m_btnAdd        = new QPushButton("Add…");
    m_btnEdit       = new QPushButton("Edit…");
    m_btnRemove     = new QPushButton("Remove");
    m_btnConnect    = new QPushButton("Connect");
    m_btnDisconnect = new QPushButton("Disconnect");
    btnRow->addWidget(m_btnAdd);
    btnRow->addWidget(m_btnEdit);
    btnRow->addWidget(m_btnRemove);
    btnRow->addStretch(1);
    btnRow->addWidget(m_btnConnect);
    btnRow->addWidget(m_btnDisconnect);
    root->addLayout(btnRow);

    m_status = new QLabel("No active session.");
    root->addWidget(m_status);

    connect(m_btnAdd,        &QPushButton::clicked, this, &ServersPage::onAdd);
    connect(m_btnEdit,       &QPushButton::clicked, this, &ServersPage::onEdit);
    connect(m_btnRemove,     &QPushButton::clicked, this, &ServersPage::onRemove);
    connect(m_btnConnect,    &QPushButton::clicked, this, &ServersPage::onConnect);
    connect(m_btnDisconnect, &QPushButton::clicked, this, &ServersPage::onDisconnect);
}

void ServersPage::onAdd() {
    AddServerDlg dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        // TODO: persist via DatabaseManager equivalent + reload table.
    }
}

void ServersPage::onEdit()      { /* TODO: open AddServerDlg in edit mode */ }
void ServersPage::onRemove()    { /* TODO: confirm + delete from servers table */ }

void ServersPage::onConnect() {
    // TODO: ServerSession::Login + GetListMounts
    SelectMountDlg dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        // TODO: notify MainWindow::setActiveSession via parent traversal or signal
        m_status->setText("Connected (stub).");
    }
}

void ServersPage::onDisconnect() {
    m_status->setText("Disconnected.");
}
