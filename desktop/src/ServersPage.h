// ServersPage.h — broadcast-station management page.
//
// Repurposed from the v1 "Servers" CMcaster1TagStackDlg (which managed ICY
// push endpoints) to instead manage the bearer's broadcast stations via
// /api/v1/me/stations. The nav label stays "My Stations" in MainWindow.
//
// Single-pane table + Add/Edit/Remove/Refresh. No mount picker here — that
// belonged to v1's push flow; in v2 the daemon owns metadata destination.

#pragma once
#include <QJsonArray>
#include <QJsonObject>
#include <QWidget>
class ApiClient;
class QTableWidget;
class QPushButton;
class QLabel;

class ServersPage : public QWidget {
    Q_OBJECT
public:
    explicit ServersPage(ApiClient* api = nullptr, QWidget* parent = nullptr);

public slots:
    void refresh();

private slots:
    void onAdd();
    void onEdit();
    void onRemove();
    void onMyStationsReceived(const QJsonArray& stations);
    void onMyStationCreated(const QString& stationId, const QString& name);
    void onMyStationUpdated(const QString& stationId, int updated);
    void onMyStationDeleted(const QString& stationId, int deleted);
    void onApiError(const QString& opName, const QString& code, const QString& message);

private:
    QJsonObject selectedStation() const;

    ApiClient*    m_api = nullptr;
    QTableWidget* m_table = nullptr;
    QPushButton*  m_btnAdd = nullptr;
    QPushButton*  m_btnEdit = nullptr;
    QPushButton*  m_btnRemove = nullptr;
    QPushButton*  m_btnRefresh = nullptr;
    QLabel*       m_status = nullptr;
    QJsonArray    m_stations;  // last received list (drives row index → station obj)
};
