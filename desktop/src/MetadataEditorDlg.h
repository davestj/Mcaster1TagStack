// MetadataEditorDlg.h — Qt mirror of CMetadataEditorDlg (modal, IDD 1001).
// Edits ID3 / Vorbis tags on a single audio file and persists to the local
// media library.

#pragma once
#include <QDialog>
class QLineEdit;
class QLabel;

class MetadataEditorDlg : public QDialog {
    Q_OBJECT
public:
    explicit MetadataEditorDlg(QWidget* parent = nullptr);
    void setFilePath(const QString& path);

private:
    QLabel*    m_path = nullptr;
    QLineEdit* m_artist = nullptr;
    QLineEdit* m_title = nullptr;
    QLineEdit* m_album = nullptr;
    QLineEdit* m_genre = nullptr;
    QLineEdit* m_year = nullptr;
    QLineEdit* m_source = nullptr;
    QLabel*    m_duration = nullptr;
};
