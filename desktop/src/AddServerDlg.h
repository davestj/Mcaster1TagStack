// AddServerDlg.h — Qt mirror of CAddServerDlg (modal, IDD 200).
// 3-step QWizard: type selection → credentials + test → confirm.

#pragma once
#include <QDialog>
class QStackedWidget;
class QComboBox;
class QLineEdit;
class QLabel;
class QPushButton;

class AddServerDlg : public QDialog {
    Q_OBJECT
public:
    explicit AddServerDlg(QWidget* parent = nullptr);

private slots:
    void onTestConnection();
    void onNext();
    void onBack();

private:
    void buildStep1();
    void buildStep2();
    void buildStep3();

    QStackedWidget* m_steps = nullptr;
    QComboBox*      m_typeCombo = nullptr;
    QLineEdit*      m_name = nullptr;
    QLineEdit*      m_url = nullptr;
    QLineEdit*      m_user = nullptr;
    QLineEdit*      m_pass = nullptr;
    QLabel*         m_testStatus = nullptr;
    QLabel*         m_summary = nullptr;
    QPushButton*    m_btnBack = nullptr;
    QPushButton*    m_btnNext = nullptr;
};
