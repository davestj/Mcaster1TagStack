// DataQueuePage.h — Qt mirror of CDataQueuePage (NAV_DATAQUEUE, IDD 1100).
// 2 tabs: Queue (pending + in-progress), History (sent / rejected / error).

#pragma once
#include <QWidget>
class QTabWidget;
class QTableWidget;
class QProgressBar;

class DataQueuePage : public QWidget {
    Q_OBJECT
public:
    explicit DataQueuePage(QWidget* parent = nullptr);

private:
    QWidget* buildQueueTab();
    QWidget* buildHistoryTab();

    QTabWidget*   m_tabs = nullptr;
    QTableWidget* m_queue = nullptr;
    QProgressBar* m_progress = nullptr;
    QTableWidget* m_history = nullptr;
};
