// StationEditDlg.h — small modal form for creating or editing one broadcast
// station. Mirrors the daemon's POST/PUT /api/v1/me/stations contract.

#pragma once
#include <QDialog>
#include <QJsonObject>
class QLineEdit;
class QComboBox;
class QPlainTextEdit;

class StationEditDlg : public QDialog {
    Q_OBJECT
public:
    enum class Mode { Create, Edit };
    explicit StationEditDlg(Mode mode, QWidget* parent = nullptr);

    void load(const QJsonObject& station);  // populate fields from an existing row
    QJsonObject payload() const;             // body to send to the daemon

    QString stationId() const;               // editable only in Create mode

private:
    Mode            m_mode;
    QLineEdit*      m_id            = nullptr;
    QLineEdit*      m_name          = nullptr;
    QLineEdit*      m_callsign      = nullptr;
    QPlainTextEdit* m_description   = nullptr;
    QComboBox*      m_timezone      = nullptr;
    QLineEdit*      m_websiteUrl    = nullptr;
    QLineEdit*      m_logoUrl       = nullptr;
    QLineEdit*      m_genre         = nullptr;
    QLineEdit*      m_market        = nullptr;
    QComboBox*      m_licenseClass  = nullptr;
};
