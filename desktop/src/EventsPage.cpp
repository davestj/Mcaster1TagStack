#include "EventsPage.h"
#include "ApiClient.h"

#include <QDateTime>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>

EventsPage::EventsPage(ApiClient* api, QWidget* parent)
    : QWidget(parent), m_api(api) {
    auto* root = new QVBoxLayout(this);
    m_tabs = new QTabWidget();
    m_tabs->addTab(buildEventsTab(), "Events");
    m_tabs->addTab(buildRunLogTab(), "Run Log");
    root->addWidget(m_tabs, 1);

    m_status = new QLabel();
    root->addWidget(m_status);

    if (m_api) {
        connect(m_api, &ApiClient::myEventsReceived,    this, &EventsPage::onMyEventsReceived);
        connect(m_api, &ApiClient::myEventCreated,      this, &EventsPage::onMyEventCreated);
        connect(m_api, &ApiClient::myEventUpdated,      this, &EventsPage::onMyEventUpdated);
        connect(m_api, &ApiClient::myEventDeleted,      this, &EventsPage::onMyEventDeleted);
        connect(m_api, &ApiClient::myEventRunsReceived, this, &EventsPage::onMyEventRunsReceived);
        QTimer::singleShot(0, this, &EventsPage::refresh);
    }
}

QWidget* EventsPage::buildEventsTab() {
    auto* w = new QWidget();
    auto* v = new QVBoxLayout(w);
    m_events = new QTableWidget(0, 7);
    m_events->setHorizontalHeaderLabels(
        {"ID", "Name", "Station", "Trigger", "Schedule", "Interval", "Enabled"});
    m_events->horizontalHeader()->setStretchLastSection(true);
    m_events->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_events->setSelectionMode(QAbstractItemView::SingleSelection);
    m_events->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_events->verticalHeader()->setVisible(false);
    v->addWidget(m_events, 1);

    auto* row = new QHBoxLayout();
    m_btnAdd     = new QPushButton("Add Event…");
    m_btnToggle  = new QPushButton("Toggle Enabled");
    m_btnDelete  = new QPushButton("Delete");
    m_btnRefresh = new QPushButton("Refresh");
    row->addWidget(m_btnAdd);
    row->addWidget(m_btnToggle);
    row->addWidget(m_btnDelete);
    row->addStretch(1);
    row->addWidget(m_btnRefresh);
    v->addLayout(row);

    connect(m_btnAdd,     &QPushButton::clicked, this, &EventsPage::onAdd);
    connect(m_btnToggle,  &QPushButton::clicked, this, &EventsPage::onToggleEnabled);
    connect(m_btnDelete,  &QPushButton::clicked, this, &EventsPage::onDelete);
    connect(m_btnRefresh, &QPushButton::clicked, this, &EventsPage::refresh);
    connect(m_events,     &QTableWidget::itemSelectionChanged,
            this, &EventsPage::onRowSelectionChanged);
    return w;
}

QWidget* EventsPage::buildRunLogTab() {
    auto* w = new QWidget();
    auto* v = new QVBoxLayout(w);
    v->addWidget(new QLabel("Latest runs for the selected event (most recent first)."));
    m_runLog = new QTableWidget(0, 4);
    m_runLog->setHorizontalHeaderLabels({"Run ID", "Ran At", "Status", "Result"});
    m_runLog->horizontalHeader()->setStretchLastSection(true);
    m_runLog->verticalHeader()->setVisible(false);
    v->addWidget(m_runLog, 1);
    return w;
}

void EventsPage::refresh() {
    if (m_api) m_api->listMyEvents();
}

qint64 EventsPage::selectedEventId() const {
    auto rows = m_events->selectionModel()->selectedRows();
    if (rows.isEmpty()) return 0;
    int r = rows.first().row();
    if (r < 0 || r >= m_eventsRaw.size()) return 0;
    return m_eventsRaw.at(r).toObject().value("id").toVariant().toLongLong();
}

bool EventsPage::selectedEventEnabled() const {
    auto rows = m_events->selectionModel()->selectedRows();
    if (rows.isEmpty()) return false;
    int r = rows.first().row();
    if (r < 0 || r >= m_eventsRaw.size()) return false;
    return m_eventsRaw.at(r).toObject().value("enabled").toBool();
}

void EventsPage::onAdd() {
    if (!m_api) return;
    bool ok = false;
    auto name = QInputDialog::getText(this, "New Event",
        "Event name:", QLineEdit::Normal, "", &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    auto stationId = QInputDialog::getText(this, "New Event",
        "Station ID (blank for none):", QLineEdit::Normal, "", &ok);
    QJsonObject p;
    p.insert("name",            name.trimmed());
    if (!stationId.trimmed().isEmpty()) p.insert("station_id", stationId.trimmed());
    p.insert("trigger_mode",    "manual");
    p.insert("days_of_week",    "1234567");
    p.insert("send_interval_s", 300);
    p.insert("dedup_window_m",  30);
    p.insert("enabled",         1);
    m_api->createMyEvent(p);
}

void EventsPage::onToggleEnabled() {
    if (!m_api) return;
    qint64 id = selectedEventId();
    if (id == 0) {
        QMessageBox::information(this, "Toggle Event", "Select an event first.");
        return;
    }
    QJsonObject p;
    p.insert("enabled", selectedEventEnabled() ? 0 : 1);
    m_api->updateMyEvent(id, p);
}

void EventsPage::onDelete() {
    if (!m_api) return;
    qint64 id = selectedEventId();
    if (id == 0) {
        QMessageBox::information(this, "Delete Event", "Select an event first.");
        return;
    }
    if (QMessageBox::question(this, "Delete Event",
            QString("Delete event #%1?").arg(id)) != QMessageBox::Yes) return;
    m_api->deleteMyEvent(id);
}

void EventsPage::onRowSelectionChanged() {
    qint64 id = selectedEventId();
    if (id != 0 && m_api) m_api->listMyEventRuns(id);
}

void EventsPage::onMyEventsReceived(const QJsonArray& events) {
    m_eventsRaw = events;
    m_events->setRowCount(0);
    m_events->setRowCount(events.size());
    for (int i = 0; i < events.size(); ++i) {
        auto o = events.at(i).toObject();
        m_events->setItem(i, 0, new QTableWidgetItem(QString::number(
            o.value("id").toVariant().toLongLong())));
        m_events->setItem(i, 1, new QTableWidgetItem(o.value("name").toString()));
        m_events->setItem(i, 2, new QTableWidgetItem(o.value("station_id").toString()));
        m_events->setItem(i, 3, new QTableWidgetItem(o.value("trigger_mode").toString()));
        QString sched;
        auto a = o.value("daypart_start").toString();
        auto b = o.value("daypart_end").toString();
        if (!a.isEmpty() || !b.isEmpty()) sched = a + " → " + b;
        sched += "  [" + o.value("days_of_week").toString() + "]";
        m_events->setItem(i, 4, new QTableWidgetItem(sched));
        m_events->setItem(i, 5, new QTableWidgetItem(
            QString::number(o.value("send_interval_s").toInt()) + "s"));
        m_events->setItem(i, 6, new QTableWidgetItem(
            o.value("enabled").toBool() ? "yes" : "no"));
    }
    m_status->setText(QString("%1 event(s).").arg(events.size()));
}

void EventsPage::onMyEventCreated(qint64 /*eventId*/, const QString& name) {
    m_status->setText("Created '" + name + "'. Refreshing…");
    refresh();
}

void EventsPage::onMyEventUpdated(qint64 eventId, int updated) {
    m_status->setText(QString("Event #%1 updated (%2 row(s)). Refreshing…")
                          .arg(eventId).arg(updated));
    refresh();
}

void EventsPage::onMyEventDeleted(qint64 eventId, int deleted) {
    m_status->setText(QString("Event #%1 deleted (%2 row(s)).")
                          .arg(eventId).arg(deleted));
    refresh();
}

void EventsPage::onMyEventRunsReceived(qint64 /*eventId*/, const QJsonArray& runs) {
    m_runLog->setRowCount(0);
    m_runLog->setRowCount(runs.size());
    for (int i = 0; i < runs.size(); ++i) {
        auto o = runs.at(i).toObject();
        m_runLog->setItem(i, 0, new QTableWidgetItem(QString::number(
            o.value("id").toVariant().toLongLong())));
        auto ts = o.value("ran_at").toVariant().toLongLong();
        m_runLog->setItem(i, 1, new QTableWidgetItem(
            QDateTime::fromSecsSinceEpoch(ts).toString(Qt::ISODate)));
        m_runLog->setItem(i, 2, new QTableWidgetItem(o.value("status").toString()));
        m_runLog->setItem(i, 3, new QTableWidgetItem(o.value("result_msg").toString()));
    }
}
