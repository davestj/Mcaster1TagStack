// LoggingPage.h — live capture of qDebug/qInfo/qWarning/qCritical into the
// App Log tab. Installs a Qt message handler at construction; the handler
// also writes to an on-disk log file when QSettings("logging/file_enabled")
// is true. The MySQL Log tab is left as a placeholder until the daemon
// surfaces its own log stream over HTTP.

#pragma once
#include <QWidget>
class QTabWidget;
class QCheckBox;
class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;

class LoggingPage : public QWidget {
    Q_OBJECT
public:
    explicit LoggingPage(QWidget* parent = nullptr);
    ~LoggingPage() override;

    // Appends a line to the App Log tab (Qt message handler entry point).
    // Safe to call from any thread — uses a queued signal internally.
    void appendAppLine(const QString& line);

signals:
    void appLineEmitted(const QString& line);

private slots:
    void onClearAppLog();
    void onExportAppLog();
    void onLevelChanged(int idx);
    void onFileEnableToggled(bool on);
    void onBrowseLogFile();

private:
    QWidget* buildConfigTab();
    QWidget* buildAppLogTab();
    QWidget* buildDbLogTab();

    QTabWidget*     m_tabs = nullptr;
    QCheckBox*      m_cfgFileEnable = nullptr;
    QComboBox*      m_cfgLevel = nullptr;
    QLineEdit*      m_cfgPath = nullptr;
    QPushButton*    m_browseBtn = nullptr;
    QPlainTextEdit* m_appLog = nullptr;
    QPlainTextEdit* m_dbLog  = nullptr;
};
