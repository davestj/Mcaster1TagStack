// DirectoryPage.h — listener-facing station directory.
//
// Left: search box + station table (server_name, genre, country, listeners,
// tag count) from /api/v1/directory/stations.
// Right: detail panel for the selected station — full metadata, my tags
// (private + public toggles), public tag cloud, and an "Add tag" composer
// that hits POST /api/v1/me/stations/:id/tags. There is no MFC v1 analogue
// — this is the new Phase 3 directory experience added on top of v1.

#pragma once
#include <QWidget>
class ApiClient;
class QLineEdit;
class QTableWidget;
class QTableWidgetItem;
class QListWidget;
class QListWidgetItem;
class QLabel;
class QPushButton;
class QSplitter;
class QComboBox;

class DirectoryPage : public QWidget {
    Q_OBJECT
public:
    explicit DirectoryPage(ApiClient* api, QWidget* parent = nullptr);

public slots:
    void refresh();  // re-runs the current filter against the API

private slots:
    void onSearchChanged();
    void onStationSelected();
    void onAddTagClicked();
    void onPushToSocialClicked();
    void onMyTagDoubleClicked(QListWidgetItem* it);

    void onDirectoryListReceived(const QJsonArray& stations);
    void onStationReceived(const QJsonObject& station);
    void onMyTagsReceived(const QString& stationId, const QJsonArray& tags);
    void onTagCloudReceived(const QString& stationId, const QJsonArray& cloud);
    void onTagAdded(const QString& stationId, const QString& tag);
    void onTagDeleted(const QString& stationId, const QString& tag, int deleted);

private:
    void loadStation(const QString& stationId);

    ApiClient* m_api = nullptr;
    QString    m_currentStationId;

    // Left
    QLineEdit*    m_search        = nullptr;
    QComboBox*    m_genreFilter   = nullptr;
    QComboBox*    m_countryFilter = nullptr;
    QPushButton*  m_refreshBtn    = nullptr;
    QLabel*       m_resultCount   = nullptr;
    QTableWidget* m_stationTable  = nullptr;

    // Right (detail)
    QLabel*       m_stationName    = nullptr;
    QLabel*       m_stationMeta    = nullptr;
    QListWidget*  m_myTags         = nullptr;
    QListWidget*  m_publicCloud    = nullptr;
    QLineEdit*    m_newTag         = nullptr;
    QComboBox*    m_newTagVisibility = nullptr;
    QPushButton*  m_btnAddTag      = nullptr;
    QPushButton*  m_btnPushSocial  = nullptr;
};
