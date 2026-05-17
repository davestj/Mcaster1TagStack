// SettingsPage.h — Qt mirror of CSettingsPage (NAV_SETTINGS, IDD 300).
// 4 tabs: Servers, Application, Database, About.

#pragma once
#include <QWidget>
class QTabWidget;
class QTableWidget;
class QLineEdit;
class QCheckBox;
class QComboBox;
class QPushButton;

class SettingsPage : public QWidget {
    Q_OBJECT
public:
    explicit SettingsPage(QWidget* parent = nullptr);

private:
    QWidget* buildServersTab();
    QWidget* buildApplicationTab();
    QWidget* buildDatabaseTab();
    QWidget* buildAboutTab();

    QTabWidget*   m_tabs = nullptr;
    QTableWidget* m_servers = nullptr;
    QCheckBox*    m_enableLog = nullptr;
    QComboBox*    m_logLevel = nullptr;
    QLineEdit*    m_logPath = nullptr;
    QLineEdit*    m_dbHost = nullptr;
    QLineEdit*    m_dbUser = nullptr;
    QLineEdit*    m_dbPass = nullptr;
    QPushButton*  m_dbTest = nullptr;
    QPushButton*  m_dbInit = nullptr;
    QPushButton*  m_dbBackup = nullptr;
    QPushButton*  m_dbMigrate = nullptr;
};
