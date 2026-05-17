// DbBackupDlg.h — Qt mirror of CDbBackupDlg (modal).
// Runs mysqldump against the local DB; streams stdout/stderr to a read-only
// text view.

#pragma once
#include <QDialog>
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QProcess;

class DbBackupDlg : public QDialog {
    Q_OBJECT
public:
    explicit DbBackupDlg(QWidget* parent = nullptr);

private slots:
    void onRun();

private:
    QLineEdit*      m_mysqldump = nullptr;
    QLineEdit*      m_outDir = nullptr;
    QLineEdit*      m_filename = nullptr;
    QPlainTextEdit* m_log = nullptr;
    QPushButton*    m_runBtn = nullptr;
    QProcess*       m_proc = nullptr;
};
