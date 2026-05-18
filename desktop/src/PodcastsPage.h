// PodcastsPage.h — backed by /api/v1/me/podcasts on the daemon.
// Layout matches v1: 3 tabs (Episodes / Feed / Publish), but the picker
// at the top selects which podcast to operate on.

#pragma once
#include <QJsonArray>
#include <QJsonObject>
#include <QWidget>
class ApiClient;
class QTabWidget;
class QListWidget;
class QTableWidget;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QComboBox;
class QSpinBox;
class QLabel;

class PodcastsPage : public QWidget {
    Q_OBJECT
public:
    explicit PodcastsPage(ApiClient* api = nullptr, QWidget* parent = nullptr);

public slots:
    void refresh();

private slots:
    void onAddPodcast();
    void onDeletePodcast();
    void onAddEpisode();
    void onDeleteEpisode();
    void onCopyRssUrl();
    void onPodcastSelectionChanged();
    void onMyPodcastsReceived(const QJsonArray& podcasts);
    void onMyPodcastCreated(qint64 feedId, const QString& slug);
    void onMyPodcastDeleted(qint64 feedId, int deleted);
    void onMyPodcastEpisodesReceived(qint64 feedId, const QJsonArray& episodes);
    void onMyPodcastEpisodeCreated(qint64 feedId, qint64 episodeId, const QString& guid);
    void onMyPodcastEpisodeDeleted(qint64 feedId, qint64 episodeId, int deleted);

private:
    QWidget* buildEpisodesTab();
    QWidget* buildFeedTab();
    QWidget* buildPublishTab();
    QJsonObject selectedPodcast() const;
    qint64 selectedPodcastId() const;
    qint64 selectedEpisodeId() const;

    ApiClient*    m_api = nullptr;
    QJsonArray    m_podcastsRaw;
    QJsonArray    m_episodesRaw;
    qint64        m_currentFeedId = 0;

    QComboBox*    m_podcastPicker = nullptr;
    QPushButton*  m_btnAddPodcast = nullptr;
    QPushButton*  m_btnDeletePodcast = nullptr;
    QPushButton*  m_btnRefresh = nullptr;
    QTabWidget*   m_tabs = nullptr;

    QTableWidget* m_episodes = nullptr;
    QLineEdit*    m_epTitle = nullptr;
    QLineEdit*    m_epAudioUrl = nullptr;
    QPlainTextEdit* m_epDescription = nullptr;
    QSpinBox*     m_epEpisodeNumber = nullptr;
    QSpinBox*     m_epSeason = nullptr;
    QSpinBox*     m_epDurationSec = nullptr;
    QPushButton*  m_btnAddEpisode = nullptr;
    QPushButton*  m_btnDeleteEpisode = nullptr;

    QLabel*       m_feedSummary = nullptr;
    QLabel*       m_rssUrl = nullptr;
    QPushButton*  m_btnCopyRss = nullptr;
    QLabel*       m_status = nullptr;
};
