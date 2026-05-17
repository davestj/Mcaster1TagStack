// MetadataEditorDlg.h — Qt mirror of CMetadataEditorDlg (modal, IDD 1001).
// Edits ID3 / Vorbis tags on a single audio file and persists to the local
// media library.

#pragma once
#include <QDialog>
#include <QJsonObject>
class ApiClient;
class QLineEdit;
class QLabel;

class MetadataEditorDlg : public QDialog {
    Q_OBJECT
public:
    explicit MetadataEditorDlg(QWidget* parent = nullptr);

    // Caller supplies the media row we're editing (id + current fields).
    void setApi(ApiClient* api) { m_api = api; }
    void load(const QJsonObject& mediaRow);

private slots:
    void onSave();

private:
    ApiClient* m_api = nullptr;
    qint64     m_mediaId = 0;
    QString    m_filePath;

    QLabel*    m_path = nullptr;
    QLineEdit* m_artist = nullptr;
    QLineEdit* m_title = nullptr;
    QLineEdit* m_album = nullptr;
    QLineEdit* m_genre = nullptr;
    QLineEdit* m_year = nullptr;
    QLineEdit* m_source = nullptr;
    QLabel*    m_duration = nullptr;
};
