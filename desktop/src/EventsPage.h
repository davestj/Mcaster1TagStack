// EventsPage.h — broadcast event scheduler. Backed by /api/v1/me/events
// and /api/v1/me/events/:id/runs on the daemon. Two tabs: the events
// table (CRUD + toggle enabled) and the run-log table (last 100 runs of
// whichever event is selected).

#pragma once
#include <QJsonArray>
#include <QJsonObject>
#include <QWidget>
class ApiClient;
class QTabWidget;
class QTableWidget;
class QTableWidgetItem;
class QPushButton;
class QLabel;
class QLineEdit;
class QComboBox;
class QSpinBox;
class QCheckBox;

class EventsPage : public QWidget {
    Q_OBJECT
public:
    explicit EventsPage(ApiClient* api = nullptr, QWidget* parent = nullptr);

public slots:
    void refresh();

private slots:
    void onAdd();
    void onDelete();
    void onToggleEnabled();
    void onRowSelectionChanged();
    void onMyEventsReceived(const QJsonArray& events);
    void onMyEventCreated(qint64 eventId, const QString& name);
    void onMyEventUpdated(qint64 eventId, int updated);
    void onMyEventDeleted(qint64 eventId, int deleted);
    void onMyEventRunsReceived(qint64 eventId, const QJsonArray& runs);

private:
    QWidget* buildEventsTab();
    QWidget* buildRunLogTab();
    qint64 selectedEventId() const;
    bool   selectedEventEnabled() const;

    ApiClient*    m_api    = nullptr;
    QTabWidget*   m_tabs   = nullptr;
    QTableWidget* m_events = nullptr;
    QTableWidget* m_runLog = nullptr;
    QPushButton*  m_btnAdd     = nullptr;
    QPushButton*  m_btnToggle  = nullptr;
    QPushButton*  m_btnDelete  = nullptr;
    QPushButton*  m_btnRefresh = nullptr;
    QLabel*       m_status     = nullptr;
    QJsonArray    m_eventsRaw;
};
