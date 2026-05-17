#include "ServersPage.h"
#include "ApiClient.h"
#include "StationEditDlg.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

ServersPage::ServersPage(ApiClient* api, QWidget* parent)
    : QWidget(parent), m_api(api) {
    auto* root = new QVBoxLayout(this);

    auto* header = new QLabel(
        "<b>My Stations</b> &mdash; broadcast brands you own. ICY 2.2 pushes "
        "and now-playing updates are gated by station ownership.");
    header->setTextFormat(Qt::RichText);
    header->setWordWrap(true);
    root->addWidget(header);

    m_table = new QTableWidget(0, 6, this);
    m_table->setHorizontalHeaderLabels(
        {"Station ID", "Name", "Callsign", "Genre", "Market", "License"});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->setVisible(false);
    root->addWidget(m_table, 1);

    auto* btns = new QHBoxLayout();
    m_btnAdd     = new QPushButton("Add Station…");
    m_btnEdit    = new QPushButton("Edit…");
    m_btnRemove  = new QPushButton("Delete");
    m_btnRefresh = new QPushButton("Refresh");
    btns->addWidget(m_btnAdd);
    btns->addWidget(m_btnEdit);
    btns->addWidget(m_btnRemove);
    btns->addStretch(1);
    btns->addWidget(m_btnRefresh);
    root->addLayout(btns);

    m_status = new QLabel();
    root->addWidget(m_status);

    connect(m_btnAdd,     &QPushButton::clicked, this, &ServersPage::onAdd);
    connect(m_btnEdit,    &QPushButton::clicked, this, &ServersPage::onEdit);
    connect(m_btnRemove,  &QPushButton::clicked, this, &ServersPage::onRemove);
    connect(m_btnRefresh, &QPushButton::clicked, this, &ServersPage::refresh);
    connect(m_table,      &QTableWidget::itemDoubleClicked,
            this, [this](QTableWidgetItem*) { onEdit(); });

    if (m_api) {
        connect(m_api, &ApiClient::myStationsReceived,
                this, &ServersPage::onMyStationsReceived);
        connect(m_api, &ApiClient::myStationCreated,
                this, &ServersPage::onMyStationCreated);
        connect(m_api, &ApiClient::myStationUpdated,
                this, &ServersPage::onMyStationUpdated);
        connect(m_api, &ApiClient::myStationDeleted,
                this, &ServersPage::onMyStationDeleted);
        connect(m_api, &ApiClient::apiError,
                this, &ServersPage::onApiError);
        QTimer::singleShot(0, this, &ServersPage::refresh);
    }
}

void ServersPage::refresh() {
    if (m_api) m_api->listMyStations();
}

QJsonObject ServersPage::selectedStation() const {
    auto rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty()) return {};
    int r = rows.first().row();
    if (r < 0 || r >= m_stations.size()) return {};
    return m_stations.at(r).toObject();
}

void ServersPage::onAdd() {
    if (!m_api) return;
    StationEditDlg dlg(StationEditDlg::Mode::Create, this);
    if (dlg.exec() != QDialog::Accepted) return;
    auto p = dlg.payload();
    if (p.value("name").toString().isEmpty()) {
        QMessageBox::warning(this, "Add Station", "Name is required.");
        return;
    }
    m_api->createMyStation(p);
}

void ServersPage::onEdit() {
    if (!m_api) return;
    auto s = selectedStation();
    if (s.isEmpty()) {
        QMessageBox::information(this, "Edit Station", "Select a station first.");
        return;
    }
    StationEditDlg dlg(StationEditDlg::Mode::Edit, this);
    dlg.load(s);
    if (dlg.exec() != QDialog::Accepted) return;
    m_api->updateMyStation(s.value("id").toString(), dlg.payload());
}

void ServersPage::onRemove() {
    if (!m_api) return;
    auto s = selectedStation();
    if (s.isEmpty()) {
        QMessageBox::information(this, "Delete Station", "Select a station first.");
        return;
    }
    auto id = s.value("id").toString();
    if (QMessageBox::question(this, "Delete Station",
            QString("Delete station '%1'? This cannot be undone.").arg(id))
        != QMessageBox::Yes) return;
    m_api->deleteMyStation(id);
}

void ServersPage::onMyStationsReceived(const QJsonArray& stations) {
    m_stations = stations;
    m_table->setRowCount(0);
    m_table->setRowCount(stations.size());
    for (int i = 0; i < stations.size(); ++i) {
        auto o = stations.at(i).toObject();
        m_table->setItem(i, 0, new QTableWidgetItem(o.value("id").toString()));
        m_table->setItem(i, 1, new QTableWidgetItem(o.value("name").toString()));
        m_table->setItem(i, 2, new QTableWidgetItem(o.value("callsign").toString()));
        m_table->setItem(i, 3, new QTableWidgetItem(o.value("genre_primary").toString()));
        m_table->setItem(i, 4, new QTableWidgetItem(o.value("market").toString()));
        m_table->setItem(i, 5, new QTableWidgetItem(o.value("license_class").toString()));
    }
    m_status->setText(QString("%1 station(s).").arg(stations.size()));
}

void ServersPage::onMyStationCreated(const QString& stationId, const QString& name) {
    m_status->setText(QString("Created '%1' (%2). Refreshing…").arg(name, stationId));
    refresh();
}

void ServersPage::onMyStationUpdated(const QString& stationId, int updated) {
    m_status->setText(QString("Updated '%1' (%2 row(s)). Refreshing…")
                          .arg(stationId).arg(updated));
    refresh();
}

void ServersPage::onMyStationDeleted(const QString& stationId, int deleted) {
    m_status->setText(QString("Deleted '%1' (%2 row(s)). Refreshing…")
                          .arg(stationId).arg(deleted));
    refresh();
}

void ServersPage::onApiError(const QString& opName, const QString& code,
                             const QString& message) {
    if (!opName.startsWith("station.")) return;
    m_status->setText(QString("[%1] %2 — %3").arg(opName, code, message));
}
