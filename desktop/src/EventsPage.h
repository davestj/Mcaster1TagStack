// EventsPage.h — Qt mirror of CEventsPage (NAV_EVENTS, IDD 1102).
// 2 tabs: Events (CRUD), Run Log (newest first).

#pragma once
#include <QWidget>
class QTabWidget;
class QTableWidget;

class EventsPage : public QWidget {
    Q_OBJECT
public:
    explicit EventsPage(QWidget* parent = nullptr);

private:
    QWidget* buildEventsTab();
    QWidget* buildRunLogTab();
    QTabWidget*   m_tabs = nullptr;
    QTableWidget* m_events = nullptr;
    QTableWidget* m_runLog = nullptr;
};
