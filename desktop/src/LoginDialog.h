// LoginDialog.h — casterclub.com auth gate. Shown modally at startup if no
// stored token is found, or if /me returns 401. On success, the dialog
// accepts and MainWindow proceeds to build the main UI.

#pragma once
#include <QDialog>
class ApiClient;
class QLineEdit;
class QLabel;
class QPushButton;

class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(ApiClient* api, QWidget* parent = nullptr);

private slots:
    void onLoginClicked();
    void onLoginSucceeded();
    void onApiError(const QString& opName, const QString& code, const QString& message);
    void onNetworkError(const QString& opName, int httpStatus, const QString& message);

private:
    ApiClient*   m_api = nullptr;
    QLineEdit*   m_username = nullptr;
    QLineEdit*   m_password = nullptr;
    QPushButton* m_loginBtn = nullptr;
    QLabel*      m_status   = nullptr;
};
