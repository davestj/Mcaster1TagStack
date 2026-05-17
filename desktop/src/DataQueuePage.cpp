#include "DataQueuePage.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>
#include <QTabWidget>
#include <QVBoxLayout>

DataQueuePage::DataQueuePage(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    m_tabs = new QTabWidget();
    m_tabs->addTab(buildQueueTab(),   "Queue");
    m_tabs->addTab(buildHistoryTab(), "History");
    root->addWidget(m_tabs);
}

QWidget* DataQueuePage::buildQueueTab() {
    auto* w = new QWidget();
    auto* v = new QVBoxLayout(w);
    m_queue = new QTableWidget(0, 5);
    m_queue->setHorizontalHeaderLabels({"Path", "Server", "Mount", "Status", "Scheduled"});
    m_queue->horizontalHeader()->setStretchLastSection(true);
    v->addWidget(m_queue, 1);
    m_progress = new QProgressBar();
    m_progress->setRange(0, 100);
    v->addWidget(m_progress);
    auto* row = new QHBoxLayout();
    row->addWidget(new QPushButton("Pause"));
    row->addWidget(new QPushButton("Apply"));
    row->addStretch(1);
    row->addWidget(new QPushButton("Clear Queue"));
    v->addLayout(row);
    return w;
}

QWidget* DataQueuePage::buildHistoryTab() {
    auto* w = new QWidget();
    auto* v = new QVBoxLayout(w);
    m_history = new QTableWidget(0, 6);
    m_history->setHorizontalHeaderLabels({"Path", "Server", "Mount", "Result", "Sent", "Error"});
    m_history->horizontalHeader()->setStretchLastSection(true);
    v->addWidget(m_history, 1);
    auto* row = new QHBoxLayout();
    row->addWidget(new QPushButton("Re-add"));
    row->addStretch(1);
    row->addWidget(new QPushButton("Clear History"));
    v->addLayout(row);
    return w;
}
