#include "LoggingPage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

LoggingPage::LoggingPage(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    m_tabs = new QTabWidget();
    m_tabs->addTab(buildConfigTab(), "Config");
    m_tabs->addTab(buildAppLogTab(), "App Log");
    m_tabs->addTab(buildDbLogTab(),  "MySQL Log");
    root->addWidget(m_tabs);
}

QWidget* LoggingPage::buildConfigTab() {
    auto* w = new QWidget();
    auto* f = new QFormLayout(w);
    m_cfgFileEnable = new QCheckBox("Enable file log");
    f->addRow(m_cfgFileEnable);
    m_cfgLevel = new QComboBox();
    m_cfgLevel->addItems({"trace", "debug", "info", "warn", "error"});
    f->addRow("Level:", m_cfgLevel);
    auto* row = new QHBoxLayout();
    m_cfgPath = new QLineEdit();
    row->addWidget(m_cfgPath, 1);
    row->addWidget(new QPushButton("Browse…"));
    f->addRow("Log file:", row);
    return w;
}

QWidget* LoggingPage::buildAppLogTab() {
    auto* w = new QWidget();
    auto* v = new QVBoxLayout(w);
    m_appLog = new QPlainTextEdit();
    m_appLog->setReadOnly(true);
    m_appLog->setPlaceholderText("Application log will stream here in real time.");
    v->addWidget(m_appLog, 1);
    auto* row = new QHBoxLayout();
    row->addWidget(new QPushButton("Clear"));
    row->addWidget(new QPushButton("Export…"));
    row->addStretch(1);
    v->addLayout(row);
    return w;
}

QWidget* LoggingPage::buildDbLogTab() {
    auto* w = new QWidget();
    auto* v = new QVBoxLayout(w);
    m_dbLog = new QPlainTextEdit();
    m_dbLog->setReadOnly(true);
    m_dbLog->setPlaceholderText("MySQL log (queries + errors) will stream here.");
    v->addWidget(m_dbLog, 1);
    auto* row = new QHBoxLayout();
    row->addWidget(new QPushButton("Clear"));
    row->addWidget(new QPushButton("Export…"));
    row->addStretch(1);
    v->addLayout(row);
    return w;
}
