#include "EventsPage.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QTableWidget>
#include <QTabWidget>
#include <QVBoxLayout>

EventsPage::EventsPage(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    m_tabs = new QTabWidget();
    m_tabs->addTab(buildEventsTab(), "Events");
    m_tabs->addTab(buildRunLogTab(), "Run Log");
    root->addWidget(m_tabs);
}

QWidget* EventsPage::buildEventsTab() {
    auto* w = new QWidget();
    auto* v = new QVBoxLayout(w);
    m_events = new QTableWidget(0, 6);
    m_events->setHorizontalHeaderLabels({"Name", "Trigger", "Schedule", "Enabled", "Next run", "Owner"});
    m_events->horizontalHeader()->setStretchLastSection(true);
    v->addWidget(m_events, 1);
    auto* row = new QHBoxLayout();
    row->addWidget(new QPushButton("New"));
    row->addWidget(new QPushButton("Edit"));
    row->addWidget(new QPushButton("Delete"));
    row->addWidget(new QPushButton("Toggle Enabled"));
    row->addStretch(1);
    v->addLayout(row);
    return w;
}

QWidget* EventsPage::buildRunLogTab() {
    auto* w = new QWidget();
    auto* v = new QVBoxLayout(w);
    m_runLog = new QTableWidget(0, 5);
    m_runLog->setHorizontalHeaderLabels({"When", "Event", "Result", "Duration", "Notes"});
    m_runLog->horizontalHeader()->setStretchLastSection(true);
    v->addWidget(m_runLog, 1);
    return w;
}
