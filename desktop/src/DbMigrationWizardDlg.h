// DbMigrationWizardDlg.h — Qt mirror of CDbMigrationWizardDlg (modal).
// Shows pending sql/NNN_*.sql files + already-applied history; lets the user
// apply one or all.

#pragma once
#include <QDialog>
class QComboBox;
class QListWidget;
class QLabel;
class QPushButton;

class DbMigrationWizardDlg : public QDialog {
    Q_OBJECT
public:
    explicit DbMigrationWizardDlg(QWidget* parent = nullptr);

private slots:
    void onApplyOne();
    void onApplyAll();
    void onRefresh();

private:
    QComboBox*   m_pending = nullptr;
    QListWidget* m_applied = nullptr;
    QLabel*      m_status = nullptr;
    QPushButton* m_btnOne = nullptr;
    QPushButton* m_btnAll = nullptr;
};
