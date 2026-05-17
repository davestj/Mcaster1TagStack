#include "DataQueuePage.h"
#include "ApiClient.h"

#include <QComboBox>
#include <QDateTime>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>

DataQueuePage::DataQueuePage(ApiClient* api, QWidget* parent)
    : QWidget(parent), m_api(api) {
    auto* root = new QVBoxLayout(this);

    auto* topRow = new QHBoxLayout();
    topRow->addWidget(new QLabel("Station:"));
    m_stationPicker = new QComboBox();
    m_stationPicker->setMinimumWidth(280);
    topRow->addWidget(m_stationPicker, 1);
    m_btnRefresh = new QPushButton("Refresh");
    topRow->addWidget(m_btnRefresh);
    root->addLayout(topRow);

    m_tabs = new QTabWidget();
    m_tabs->addTab(buildHistoryTab(), "Spin History");
    m_tabs->addTab(buildSocialTab(),  "Social Push Queue");
    root->addWidget(m_tabs, 1);

    m_status = new QLabel();
    root->addWidget(m_status);

    connect(m_stationPicker, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { onStationPicked(); });
    connect(m_btnRefresh, &QPushButton::clicked, this, &DataQueuePage::refresh);

    if (m_api) {
        connect(m_api, &ApiClient::myStationsReceived, this, &DataQueuePage::onMyStationsReceived);
        connect(m_api, &ApiClient::spinHistoryReceived, this, &DataQueuePage::onSpinHistoryReceived);
        connect(m_api, &ApiClient::socialQueueReceived, this, &DataQueuePage::onSocialQueueReceived);
        QTimer::singleShot(0, this, &DataQueuePage::refresh);
    }
}

QWidget* DataQueuePage::buildHistoryTab() {
    auto* w = new QWidget();
    auto* v = new QVBoxLayout(w);
    m_history = new QTableWidget(0, 6);
    m_history->setHorizontalHeaderLabels({"When", "Title", "Artist", "Album", "Duration", "Spin ID"});
    m_history->horizontalHeader()->setStretchLastSection(true);
    m_history->verticalHeader()->setVisible(false);
    v->addWidget(m_history, 1);
    return w;
}

QWidget* DataQueuePage::buildSocialTab() {
    auto* w = new QWidget();
    auto* v = new QVBoxLayout(w);
    m_queue = new QTableWidget(0, 8);
    m_queue->setHorizontalHeaderLabels(
        {"Job", "Platform", "Status", "Attempts", "Created", "Completed", "URL", "Error"});
    m_queue->horizontalHeader()->setStretchLastSection(true);
    m_queue->verticalHeader()->setVisible(false);
    v->addWidget(m_queue, 1);
    return w;
}

void DataQueuePage::refresh() {
    if (!m_api) return;
    m_api->listMyStations();    // refresh station picker
    m_api->listSocialQueue();   // social queue is global, not per-station
    onStationPicked();          // ask for spin history on the current selection
}

void DataQueuePage::onMyStationsReceived(const QJsonArray& stations) {
    auto prev = m_stationPicker->currentData().toString();
    m_stationPicker->clear();
    for (const auto& v : stations) {
        auto o = v.toObject();
        auto label = o.value("name").toString();
        auto cs    = o.value("callsign").toString();
        if (!cs.isEmpty()) label += " (" + cs + ")";
        m_stationPicker->addItem(label, o.value("id").toString());
    }
    if (!prev.isEmpty()) {
        int idx = m_stationPicker->findData(prev);
        if (idx >= 0) m_stationPicker->setCurrentIndex(idx);
    }
    if (m_stationPicker->count() == 0) {
        m_status->setText("No stations yet — create one in My Stations to see spin history.");
    }
}

void DataQueuePage::onStationPicked() {
    if (!m_api) return;
    auto id = m_stationPicker->currentData().toString();
    if (id.isEmpty()) {
        m_history->setRowCount(0);
        return;
    }
    m_api->listSpinHistory(id, 200);
}

void DataQueuePage::onSpinHistoryReceived(const QString& stationId, const QJsonArray& spins) {
    if (stationId != m_stationPicker->currentData().toString()) return;
    m_history->setRowCount(0);
    m_history->setRowCount(spins.size());
    for (int i = 0; i < spins.size(); ++i) {
        auto o = spins.at(i).toObject();
        auto when = QDateTime::fromSecsSinceEpoch(
            o.value("received_at").toVariant().toLongLong()).toString(Qt::ISODate);
        m_history->setItem(i, 0, new QTableWidgetItem(when));
        m_history->setItem(i, 1, new QTableWidgetItem(o.value("title").toString()));
        m_history->setItem(i, 2, new QTableWidgetItem(o.value("artist").toString()));
        m_history->setItem(i, 3, new QTableWidgetItem(o.value("album").toString()));
        int sec = o.value("duration_sec").toInt();
        m_history->setItem(i, 4, new QTableWidgetItem(
            sec > 0 ? QString::asprintf("%d:%02d", sec / 60, sec % 60) : "—"));
        m_history->setItem(i, 5, new QTableWidgetItem(o.value("spin_id").toString()));
    }
    m_status->setText(QString("%1 spin(s) on %2.").arg(spins.size()).arg(stationId));
}

void DataQueuePage::onSocialQueueReceived(const QJsonArray& queue) {
    m_queue->setRowCount(0);
    m_queue->setRowCount(queue.size());
    for (int i = 0; i < queue.size(); ++i) {
        auto o = queue.at(i).toObject();
        m_queue->setItem(i, 0, new QTableWidgetItem(QString::number(
            o.value("job_id").toVariant().toLongLong())));
        m_queue->setItem(i, 1, new QTableWidgetItem(o.value("platform").toString()));
        m_queue->setItem(i, 2, new QTableWidgetItem(o.value("status").toString()));
        m_queue->setItem(i, 3, new QTableWidgetItem(QString::number(o.value("attempts").toInt())));
        auto whenC = o.value("created_at").toVariant().toLongLong();
        m_queue->setItem(i, 4, new QTableWidgetItem(
            QDateTime::fromSecsSinceEpoch(whenC).toString(Qt::ISODate)));
        auto whenD = o.value("completed_at").toVariant().toLongLong();
        m_queue->setItem(i, 5, new QTableWidgetItem(
            whenD > 0 ? QDateTime::fromSecsSinceEpoch(whenD).toString(Qt::ISODate) : "—"));
        m_queue->setItem(i, 6, new QTableWidgetItem(o.value("platform_post_url").toString()));
        m_queue->setItem(i, 7, new QTableWidgetItem(o.value("last_error").toString()));
    }
}
