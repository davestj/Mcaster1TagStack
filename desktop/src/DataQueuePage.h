// DataQueuePage.h — operational data feeds.
// Tab 1: Spin History — last 100 spins on whichever of my stations is picked.
// Tab 2: Social Queue — every queued social-push job (any status).

#pragma once
#include <QJsonArray>
#include <QJsonObject>
#include <QWidget>
class ApiClient;
class QTabWidget;
class QTableWidget;
class QComboBox;
class QPushButton;
class QLabel;

class DataQueuePage : public QWidget {
    Q_OBJECT
public:
    explicit DataQueuePage(ApiClient* api = nullptr, QWidget* parent = nullptr);

public slots:
    void refresh();

private slots:
    void onStationPicked();
    void onMyStationsReceived(const QJsonArray& stations);
    void onSpinHistoryReceived(const QString& stationId, const QJsonArray& spins);
    void onSocialQueueReceived(const QJsonArray& queue);

private:
    QWidget* buildHistoryTab();
    QWidget* buildSocialTab();

    ApiClient*    m_api = nullptr;
    QTabWidget*   m_tabs = nullptr;
    QComboBox*    m_stationPicker = nullptr;
    QTableWidget* m_history = nullptr;
    QTableWidget* m_queue = nullptr;
    QPushButton*  m_btnRefresh = nullptr;
    QLabel*       m_status = nullptr;
};
