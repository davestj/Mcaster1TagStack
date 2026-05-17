// SelectMountDlg.h — Qt mirror of CSelectMountDlg (modal, IDD 500).
// Shown after server Login() succeeds, before any ICY metadata pushes.

#pragma once
#include <QDialog>
#include <QStringList>
class QComboBox;

class SelectMountDlg : public QDialog {
    Q_OBJECT
public:
    explicit SelectMountDlg(QWidget* parent = nullptr);
    void setMounts(const QStringList& mounts);
    QString selectedMount() const;

private:
    QComboBox* m_combo = nullptr;
};
