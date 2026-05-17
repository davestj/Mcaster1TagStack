// LoggingPage.h — Qt mirror of CLoggingPage (NAV_DEBUG, IDD 900).
// 3 tabs: Config, App Log, MySQL Log. The two log tabs in v1 used
// CConsoleRichEdit (black bg + colored text). Per the no-theme directive
// we use the default QPlainTextEdit appearance.

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

private:
    QWidget* buildConfigTab();
    QWidget* buildAppLogTab();
    QWidget* buildDbLogTab();

    QTabWidget*     m_tabs = nullptr;
    QCheckBox*      m_cfgFileEnable = nullptr;
    QComboBox*      m_cfgLevel = nullptr;
    QLineEdit*      m_cfgPath = nullptr;
    QPlainTextEdit* m_appLog = nullptr;
    QPlainTextEdit* m_dbLog = nullptr;
};
