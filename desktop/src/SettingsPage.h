// SettingsPage.h — application settings backed by QSettings + a daemon
// connection panel. v1 had a "Servers" tab in Settings (legacy ICY push
// list) — in v2 that lives in "My Stations", so this page now has only
// three tabs: Application, Daemon, About.

#pragma once
#include <QWidget>
class ApiClient;
class QTabWidget;
class QLineEdit;
class QCheckBox;
class QComboBox;
class QPushButton;
class QLabel;

class SettingsPage : public QWidget {
    Q_OBJECT
public:
    explicit SettingsPage(ApiClient* api = nullptr, QWidget* parent = nullptr);

private slots:
    void onSaveApplication();
    void onPingDaemon();
    void onSaveDaemonUrl();

private:
    QWidget* buildApplicationTab();
    QWidget* buildDaemonTab();
    QWidget* buildAboutTab();

    ApiClient*   m_api = nullptr;
    QTabWidget*  m_tabs = nullptr;
    QCheckBox*   m_enableLog = nullptr;
    QComboBox*   m_logLevel = nullptr;
    QLineEdit*   m_logPath = nullptr;
    QLineEdit*   m_daemonUrl = nullptr;
    QPushButton* m_pingBtn = nullptr;
    QLabel*      m_pingStatus = nullptr;
};
