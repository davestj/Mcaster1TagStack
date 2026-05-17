// ServersPage.h — Qt mirror of CMcaster1TagStackDlg (NAV_SERVERS, IDD 102).
//
// Single-pane page: a table of configured servers + Add/Edit/Remove/Connect/
// Disconnect actions. On successful Connect, prompts SelectMountDlg, then
// notifies MainWindow::setActiveSession().

#pragma once
#include <QWidget>
class QTableWidget;
class QPushButton;
class QLabel;

class ServersPage : public QWidget {
    Q_OBJECT
public:
    explicit ServersPage(QWidget* parent = nullptr);

private slots:
    void onAdd();
    void onEdit();
    void onRemove();
    void onConnect();
    void onDisconnect();

private:
    QTableWidget* m_serverTable = nullptr;
    QPushButton*  m_btnAdd      = nullptr;
    QPushButton*  m_btnEdit     = nullptr;
    QPushButton*  m_btnRemove   = nullptr;
    QPushButton*  m_btnConnect  = nullptr;
    QPushButton*  m_btnDisconnect = nullptr;
    QLabel*       m_status      = nullptr;
};
