#include "SettingsPage.h"
#include "ApiClient.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTabWidget>
#include <QVBoxLayout>

SettingsPage::SettingsPage(ApiClient* api, QWidget* parent)
    : QWidget(parent), m_api(api) {
    auto* root = new QVBoxLayout(this);
    m_tabs = new QTabWidget();
    m_tabs->addTab(buildApplicationTab(), "Application");
    m_tabs->addTab(buildDaemonTab(),      "Daemon");
    m_tabs->addTab(buildAboutTab(),       "About");
    root->addWidget(m_tabs);
}

QWidget* SettingsPage::buildApplicationTab() {
    auto* w = new QWidget();
    auto* f = new QFormLayout(w);
    QSettings s;

    m_enableLog = new QCheckBox("Mirror app log to disk");
    m_enableLog->setChecked(s.value("logging/file_enabled", false).toBool());
    f->addRow(m_enableLog);

    m_logLevel = new QComboBox();
    m_logLevel->addItems({"debug", "info", "warn", "error"});
    m_logLevel->setCurrentText(s.value("logging/level", "info").toString());
    f->addRow("Log level:", m_logLevel);

    auto* row = new QHBoxLayout();
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                          + "/tagstack-desktop.log";
    m_logPath = new QLineEdit(s.value("logging/file_path", defaultPath).toString());
    row->addWidget(m_logPath, 1);
    auto* browse = new QPushButton("Browse…");
    row->addWidget(browse);
    f->addRow("Log file:", row);

    auto* save = new QPushButton("Save");
    f->addRow(save);

    connect(browse, &QPushButton::clicked, this, [this]() {
        QString fn = QFileDialog::getSaveFileName(this, "Choose log file", m_logPath->text());
        if (!fn.isEmpty()) m_logPath->setText(fn);
    });
    connect(save, &QPushButton::clicked, this, &SettingsPage::onSaveApplication);
    return w;
}

QWidget* SettingsPage::buildDaemonTab() {
    auto* w = new QWidget();
    auto* v = new QVBoxLayout(w);
    auto* info = new QLabel(
        "<p>The desktop talks to the TagStack daemon over HTTPS — it never "
        "opens a direct MySQL connection. Override the base URL here for "
        "development against a non-production daemon.</p>");
    info->setTextFormat(Qt::RichText);
    info->setWordWrap(true);
    v->addWidget(info);

    auto* f = new QFormLayout();
    m_daemonUrl = new QLineEdit(m_api ? m_api->baseUrl()
                                      : QString("https://tagstack.mcaster1.com:9890"));
    f->addRow("Daemon URL:", m_daemonUrl);
    v->addLayout(f);

    auto* row = new QHBoxLayout();
    auto* saveBtn = new QPushButton("Save URL");
    m_pingBtn  = new QPushButton("Test connection");
    m_pingStatus = new QLabel();
    row->addWidget(saveBtn);
    row->addWidget(m_pingBtn);
    row->addWidget(m_pingStatus, 1);
    v->addLayout(row);
    v->addStretch(1);

    connect(saveBtn,    &QPushButton::clicked, this, &SettingsPage::onSaveDaemonUrl);
    connect(m_pingBtn,  &QPushButton::clicked, this, &SettingsPage::onPingDaemon);

    if (m_api) {
        connect(m_api, &ApiClient::healthReceived,
                this, [this](const QJsonObject& h) {
            m_pingStatus->setText(QString("OK: v%1 db_ok=%2")
                .arg(h.value("version").toString())
                .arg(h.value("db_ok").toBool() ? "yes" : "no"));
        });
        connect(m_api, &ApiClient::networkError,
                this, [this](const QString& op, int s, const QString& msg) {
            if (op == "health") m_pingStatus->setText(
                QString("Network error %1: %2").arg(s).arg(msg));
        });
        connect(m_api, &ApiClient::apiError,
                this, [this](const QString& op, const QString& c, const QString& m) {
            if (op == "health") m_pingStatus->setText("[" + c + "] " + m);
        });
    }
    return w;
}

QWidget* SettingsPage::buildAboutTab() {
    auto* w = new QWidget();
    auto* v = new QVBoxLayout(w);
    auto* lbl = new QLabel(
        "<h3>Mcaster1 TagStack — desktop</h3>"
        "<p>Version 0.1.0</p>"
        "<p>&copy; 2026 Dave St. John &lt;davestj@gmail.com&gt;<br>"
        "Licensed under the Mcaster1 Custom Proprietary License with AI Attribution.</p>"
        "<p>This client talks to the TagStack daemon at "
        "<code>tagstack.mcaster1.com:9890</code> for everything — login, "
        "directory browsing, station management, ICY 2.2 metadata push, "
        "and the social push queue.</p>"
        "<p>Source: <code>github.com/davestj/Mcaster1TagStack</code></p>");
    lbl->setTextFormat(Qt::RichText);
    lbl->setOpenExternalLinks(true);
    lbl->setAlignment(Qt::AlignTop);
    lbl->setWordWrap(true);
    v->addWidget(lbl);
    v->addStretch(1);
    return w;
}

void SettingsPage::onSaveApplication() {
    QSettings s;
    s.setValue("logging/file_enabled", m_enableLog->isChecked());
    s.setValue("logging/level",        m_logLevel->currentText());
    s.setValue("logging/file_path",    m_logPath->text());
    qInfo() << "settings saved";
}

void SettingsPage::onSaveDaemonUrl() {
    if (!m_api) return;
    auto u = m_daemonUrl->text().trimmed();
    if (u.isEmpty()) return;
    m_api->setBaseUrl(u);
    QSettings s; s.setValue("daemon/base_url", u);
    m_pingStatus->setText("Saved. Click 'Test connection' to verify.");
}

void SettingsPage::onPingDaemon() {
    if (!m_api) return;
    m_pingStatus->setText("Pinging…");
    m_api->health();
}
