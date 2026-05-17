#include "AddServerDlg.h"

#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

AddServerDlg::AddServerDlg(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Add Server");
    resize(560, 360);

    auto* root = new QVBoxLayout(this);
    m_steps = new QStackedWidget(this);

    buildStep1();
    buildStep2();
    buildStep3();

    root->addWidget(m_steps, 1);

    auto* btnRow = new QHBoxLayout();
    m_btnBack = new QPushButton("Back");
    m_btnNext = new QPushButton("Next");
    auto* cancel = new QPushButton("Cancel");
    btnRow->addStretch(1);
    btnRow->addWidget(m_btnBack);
    btnRow->addWidget(m_btnNext);
    btnRow->addWidget(cancel);
    root->addLayout(btnRow);

    connect(m_btnBack, &QPushButton::clicked, this, &AddServerDlg::onBack);
    connect(m_btnNext, &QPushButton::clicked, this, &AddServerDlg::onNext);
    connect(cancel,    &QPushButton::clicked, this, &AddServerDlg::reject);

    m_btnBack->setEnabled(false);
}

void AddServerDlg::buildStep1() {
    auto* w = new QWidget();
    auto* v = new QVBoxLayout(w);
    v->addWidget(new QLabel("<b>Step 1.</b> Pick a server type."));
    m_typeCombo = new QComboBox();
    m_typeCombo->addItems({
        "Mcaster1DNAS", "Icecast2", "SHOUTcast v1", "SHOUTcast v2", "RTMP", "HTTP"
    });
    v->addWidget(m_typeCombo);
    v->addStretch(1);
    m_steps->addWidget(w);
}

void AddServerDlg::buildStep2() {
    auto* w = new QWidget();
    auto* f = new QFormLayout(w);
    m_name = new QLineEdit();  f->addRow("Name:",   m_name);
    m_url  = new QLineEdit();  f->addRow("URL:",    m_url);
    m_user = new QLineEdit();  f->addRow("User:",   m_user);
    m_pass = new QLineEdit();  m_pass->setEchoMode(QLineEdit::Password);
    f->addRow("Password:", m_pass);
    auto* row = new QHBoxLayout();
    auto* test = new QPushButton("Test Connection");
    m_testStatus = new QLabel();
    row->addWidget(test);
    row->addWidget(m_testStatus, 1);
    f->addRow(row);
    connect(test, &QPushButton::clicked, this, &AddServerDlg::onTestConnection);
    m_steps->addWidget(w);
}

void AddServerDlg::buildStep3() {
    auto* w = new QWidget();
    auto* v = new QVBoxLayout(w);
    v->addWidget(new QLabel("<b>Step 3.</b> Review and save."));
    m_summary = new QLabel();
    m_summary->setTextFormat(Qt::RichText);
    m_summary->setWordWrap(true);
    v->addWidget(m_summary);
    v->addStretch(1);
    m_steps->addWidget(w);
}

void AddServerDlg::onTestConnection() {
    // TODO: libcurl HEAD/auth probe against the URL.
    m_testStatus->setText("(test stub) — fill in libcurl probe");
}

void AddServerDlg::onNext() {
    int cur = m_steps->currentIndex();
    if (cur == 2) {
        accept();
        return;
    }
    m_steps->setCurrentIndex(cur + 1);
    m_btnBack->setEnabled(true);
    if (m_steps->currentIndex() == 2) {
        m_summary->setText(QString(
            "<table>"
            "<tr><td>Name:</td><td>%1</td></tr>"
            "<tr><td>Type:</td><td>%2</td></tr>"
            "<tr><td>URL:</td><td>%3</td></tr>"
            "<tr><td>User:</td><td>%4</td></tr>"
            "</table>").arg(m_name->text(), m_typeCombo->currentText(),
                            m_url->text(), m_user->text()));
        m_btnNext->setText("Save");
    } else {
        m_btnNext->setText("Next");
    }
}

void AddServerDlg::onBack() {
    int cur = m_steps->currentIndex();
    if (cur == 0) return;
    m_steps->setCurrentIndex(cur - 1);
    m_btnNext->setText("Next");
    if (m_steps->currentIndex() == 0) m_btnBack->setEnabled(false);
}
