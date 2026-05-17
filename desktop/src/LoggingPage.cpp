#include "LoggingPage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTextStream>
#include <QVBoxLayout>

namespace {

// We need a singleton pointer the Qt message handler can reach. The handler
// itself can run on any thread, so we go through a queued signal to land the
// line on the GUI thread.
LoggingPage* g_logging = nullptr;
QtMessageHandler g_prevHandler = nullptr;

QString levelTag(QtMsgType t) {
    switch (t) {
        case QtDebugMsg:    return "DEBUG";
        case QtInfoMsg:     return "INFO";
        case QtWarningMsg:  return "WARN";
        case QtCriticalMsg: return "ERROR";
        case QtFatalMsg:    return "FATAL";
    }
    return "?";
}

void handler(QtMsgType type, const QMessageLogContext& ctx, const QString& msg) {
    QString line = QString("[%1] %2  %3")
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss.zzz"))
        .arg(levelTag(type), -5)
        .arg(msg);
    if (ctx.file && type >= QtWarningMsg) {
        line += QString("  (%1:%2)").arg(ctx.file).arg(ctx.line);
    }
    if (g_logging) g_logging->appendAppLine(line);

    // Optional on-disk mirror.
    QSettings s;
    if (s.value("logging/file_enabled", false).toBool()) {
        QString p = s.value("logging/file_path").toString();
        if (!p.isEmpty()) {
            QFile f(p);
            if (f.open(QIODevice::Append | QIODevice::Text)) {
                QTextStream ts(&f);
                ts << line << "\n";
            }
        }
    }

    // Chain to whatever was there before us (usually the default stderr writer).
    if (g_prevHandler) g_prevHandler(type, ctx, msg);
}

}  // namespace

LoggingPage::LoggingPage(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    m_tabs = new QTabWidget();
    m_tabs->addTab(buildConfigTab(), "Config");
    m_tabs->addTab(buildAppLogTab(), "App Log");
    m_tabs->addTab(buildDbLogTab(),  "MySQL Log");
    root->addWidget(m_tabs);

    g_logging = this;
    g_prevHandler = qInstallMessageHandler(handler);

    connect(this, &LoggingPage::appLineEmitted,
            this, [this](const QString& line) {
        m_appLog->appendPlainText(line);
    }, Qt::QueuedConnection);

    qInfo() << "Logging page online.";
}

LoggingPage::~LoggingPage() {
    g_logging = nullptr;
    qInstallMessageHandler(g_prevHandler);
}

void LoggingPage::appendAppLine(const QString& line) {
    emit appLineEmitted(line);
}

QWidget* LoggingPage::buildConfigTab() {
    auto* w = new QWidget();
    auto* f = new QFormLayout(w);

    QSettings s;
    m_cfgFileEnable = new QCheckBox("Mirror app log to disk");
    m_cfgFileEnable->setChecked(s.value("logging/file_enabled", false).toBool());
    f->addRow(m_cfgFileEnable);

    m_cfgLevel = new QComboBox();
    m_cfgLevel->addItems({"debug", "info", "warn", "error"});
    m_cfgLevel->setCurrentText(s.value("logging/level", "info").toString());
    f->addRow("Visible level:", m_cfgLevel);

    auto* row = new QHBoxLayout();
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                          + "/tagstack-desktop.log";
    m_cfgPath = new QLineEdit(s.value("logging/file_path", defaultPath).toString());
    row->addWidget(m_cfgPath, 1);
    m_browseBtn = new QPushButton("Browse…");
    row->addWidget(m_browseBtn);
    f->addRow("Log file:", row);

    connect(m_cfgFileEnable, &QCheckBox::toggled,         this, &LoggingPage::onFileEnableToggled);
    connect(m_cfgLevel,      QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LoggingPage::onLevelChanged);
    connect(m_cfgPath,       &QLineEdit::editingFinished, this, [this]() {
        QSettings s; s.setValue("logging/file_path", m_cfgPath->text());
    });
    connect(m_browseBtn,     &QPushButton::clicked,       this, &LoggingPage::onBrowseLogFile);
    return w;
}

QWidget* LoggingPage::buildAppLogTab() {
    auto* w = new QWidget();
    auto* v = new QVBoxLayout(w);
    m_appLog = new QPlainTextEdit();
    m_appLog->setReadOnly(true);
    m_appLog->setMaximumBlockCount(5000);  // ring-cap so it can't grow unbounded
    m_appLog->setPlaceholderText("App log stream — qDebug / qInfo / qWarning lands here.");
    v->addWidget(m_appLog, 1);
    auto* row = new QHBoxLayout();
    auto* clr = new QPushButton("Clear");
    auto* exp = new QPushButton("Export…");
    row->addWidget(clr);
    row->addWidget(exp);
    row->addStretch(1);
    v->addLayout(row);
    connect(clr, &QPushButton::clicked, this, &LoggingPage::onClearAppLog);
    connect(exp, &QPushButton::clicked, this, &LoggingPage::onExportAppLog);
    return w;
}

QWidget* LoggingPage::buildDbLogTab() {
    auto* w = new QWidget();
    auto* v = new QVBoxLayout(w);
    m_dbLog = new QPlainTextEdit();
    m_dbLog->setReadOnly(true);
    m_dbLog->setPlaceholderText(
        "Daemon MySQL log will stream here once the daemon exposes a "
        "log endpoint. The desktop talks to the daemon over the REST API, "
        "so we never open a direct MySQL connection — the daemon owns that.");
    v->addWidget(m_dbLog, 1);
    return w;
}

void LoggingPage::onClearAppLog() { m_appLog->clear(); }

void LoggingPage::onExportAppLog() {
    QString fn = QFileDialog::getSaveFileName(this, "Export app log", "tagstack-applog.txt");
    if (fn.isEmpty()) return;
    QFile f(fn);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream ts(&f);
    ts << m_appLog->toPlainText();
}

void LoggingPage::onLevelChanged(int /*idx*/) {
    QSettings s; s.setValue("logging/level", m_cfgLevel->currentText());
}

void LoggingPage::onFileEnableToggled(bool on) {
    QSettings s; s.setValue("logging/file_enabled", on);
}

void LoggingPage::onBrowseLogFile() {
    QString fn = QFileDialog::getSaveFileName(this, "Choose log file", m_cfgPath->text());
    if (fn.isEmpty()) return;
    m_cfgPath->setText(fn);
    QSettings s; s.setValue("logging/file_path", fn);
}
