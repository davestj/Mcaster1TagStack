#include "LoginDialog.h"
#include "ApiClient.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

LoginDialog::LoginDialog(ApiClient* api, QWidget* parent)
    : QDialog(parent), m_api(api) {
    setWindowTitle("Mcaster1 TagStack — Sign in");
    resize(420, 220);

    auto* v = new QVBoxLayout(this);

    auto* heading = new QLabel(
        "<h3>Sign in to TagStack</h3>"
        "<p>Use your casterclub.com account to access the TagStack directory, "
        "tag stations, and push to your social accounts.</p>");
    heading->setWordWrap(true);
    heading->setTextFormat(Qt::RichText);
    v->addWidget(heading);

    auto* f = new QFormLayout();
    m_username = new QLineEdit();
    m_username->setPlaceholderText("casterclub username");
    f->addRow("Username:", m_username);
    m_password = new QLineEdit();
    m_password->setEchoMode(QLineEdit::Password);
    m_password->setPlaceholderText("password");
    f->addRow("Password:", m_password);
    v->addLayout(f);

    m_status = new QLabel();
    m_status->setStyleSheet("");  // default
    m_status->setWordWrap(true);
    v->addWidget(m_status);

    auto* row = new QHBoxLayout();
    row->addStretch(1);
    auto* cancel = new QPushButton("Quit");
    m_loginBtn   = new QPushButton("Sign in");
    m_loginBtn->setDefault(true);
    row->addWidget(cancel);
    row->addWidget(m_loginBtn);
    v->addLayout(row);

    connect(cancel,     &QPushButton::clicked, this, &QDialog::reject);
    connect(m_loginBtn, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(m_password, &QLineEdit::returnPressed, this, &LoginDialog::onLoginClicked);

    connect(m_api, &ApiClient::loginSucceeded, this, &LoginDialog::onLoginSucceeded);
    connect(m_api, &ApiClient::apiError,       this, &LoginDialog::onApiError);
    connect(m_api, &ApiClient::networkError,   this, &LoginDialog::onNetworkError);
}

void LoginDialog::onLoginClicked() {
    auto user = m_username->text().trimmed();
    auto pass = m_password->text();
    if (user.isEmpty() || pass.isEmpty()) {
        m_status->setText("Username and password are required.");
        return;
    }
    m_loginBtn->setEnabled(false);
    m_status->setText("Signing in…");
    m_api->login(user, pass);
}

void LoginDialog::onLoginSucceeded() {
    accept();
}

void LoginDialog::onApiError(const QString& opName, const QString& code,
                             const QString& message) {
    if (opName != "login") return;
    m_loginBtn->setEnabled(true);
    m_status->setText(QString("[%1] %2").arg(code, message));
}

void LoginDialog::onNetworkError(const QString& opName, int httpStatus,
                                 const QString& message) {
    if (opName != "login") return;
    m_loginBtn->setEnabled(true);
    m_status->setText(QString("Network error (HTTP %1): %2").arg(httpStatus).arg(message));
}
