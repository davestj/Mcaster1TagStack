// MediaLibraryPage.h — Qt mirror of CMediaLibraryPage (NAV_MEDIA, IDD 600).
// 3 tabs: Library, Playlist, Export.

#pragma once
#include <QWidget>
class QTabWidget;
class QTableWidget;
class QListWidget;
class QLineEdit;
class QPushButton;
class QComboBox;

class MediaLibraryPage : public QWidget {
    Q_OBJECT
public:
    explicit MediaLibraryPage(QWidget* parent = nullptr);

private slots:
    void onAddFiles();
    void onAddFolder();
    void onEditMetadata();
    void onOpenComposerPro();
    void onExport();

private:
    QWidget* buildLibraryTab();
    QWidget* buildPlaylistTab();
    QWidget* buildExportTab();

    QTabWidget*   m_tabs       = nullptr;

    // Library tab
    QTableWidget* m_libTable   = nullptr;
    QLineEdit*    m_libSearch  = nullptr;
    QPushButton*  m_libAddFiles= nullptr;
    QPushButton*  m_libAddFolder = nullptr;
    QPushButton*  m_libRemove  = nullptr;
    QPushButton*  m_libClear   = nullptr;

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
