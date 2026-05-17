// MediaLibraryPage.h — Qt mirror of CMediaLibraryPage (NAV_MEDIA, IDD 600).
// 3 tabs: Library, Playlist, Export.

#pragma once
#include <QJsonArray>
#include <QWidget>
class ApiClient;
class QTabWidget;
class QTableWidget;
class QListWidget;
class QLineEdit;
class QPushButton;
class QComboBox;
class QLabel;

class MediaLibraryPage : public QWidget {
    Q_OBJECT
public:
    explicit MediaLibraryPage(ApiClient* api = nullptr, QWidget* parent = nullptr);

public slots:
    void refresh();

private slots:
    void onAddFiles();
    void onAddFolder();
    void onEditMetadata();
    void onOpenComposerPro();
    void onExport();
    void onRemoveSelected();
    void onSearchChanged();
    void onMyMediaReceived(const QJsonArray& items);
    void onMyMediaUpserted(qint64 mediaId, const QString& filePath);
    void onMyMediaDeleted(qint64 mediaId, int deleted);

private:
    QWidget* buildLibraryTab();
    QWidget* buildPlaylistTab();
    QWidget* buildExportTab();

    ApiClient*    m_api        = nullptr;
    QTabWidget*   m_tabs       = nullptr;
    QJsonArray    m_libraryRaw;

    // Library tab
    QTableWidget* m_libTable   = nullptr;
    QLineEdit*    m_libSearch  = nullptr;
    QPushButton*  m_libAddFiles= nullptr;
    QPushButton*  m_libAddFolder = nullptr;
    QPushButton*  m_libRemove  = nullptr;
    QPushButton*  m_libClear   = nullptr;
    QLabel*       m_libStatus  = nullptr;

    // Playlist tab
    QListWidget*  m_plList     = nullptr;
    QTableWidget* m_plTracks   = nullptr;
    QPushButton*  m_plNew      = nullptr;
    QPushButton*  m_plSave     = nullptr;
    QPushButton*  m_plAdd      = nullptr;
    QPushButton*  m_plRem      = nullptr;
    QPushButton*  m_plUp       = nullptr;
    QPushButton*  m_plDown     = nullptr;
    QPushButton*  m_plDelete   = nullptr;
    QPushButton*  m_plComposer = nullptr;

    // Export tab
    QComboBox*    m_expFormat  = nullptr;
    QLineEdit*    m_expPath    = nullptr;
    QPushButton*  m_expBrowse  = nullptr;
    QPushButton*  m_expRun     = nullptr;
};
