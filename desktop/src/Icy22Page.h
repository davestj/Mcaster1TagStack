// Icy22Page.h — Qt mirror of CIcy22Page (NAV_ICY22, IDD 400).
//
// 4 content tabs spanning 11 ICY 2.2 field categories:
//   Tab 1 (Identity):    Station, Show, DJ
//   Tab 2 (Content):     Track, Podcast
//   Tab 3 (Engagement):  Social, Engagement, Broadcast
//   Tab 4 (Technical):   Notices, Audio, Flags
//
// Session tab bar at the top (multi-session compose). Server + Mount pickers
// above the field tabs. Save / Push / Queue / Production Mode buttons below.

#pragma once
#include <QJsonArray>
#include <QWidget>
class QTabBar;
class QTabWidget;
class QComboBox;
class QPushButton;
class QCheckBox;
class QLineEdit;
class QPlainTextEdit;
class QLabel;
class ApiClient;

class Icy22Page : public QWidget {
    Q_OBJECT
public:
    explicit Icy22Page(ApiClient* api = nullptr, QWidget* parent = nullptr);
    void setSession(const QString& serverLabel, const QString& mount);
    void refreshStations();

private slots:
    void onNewSession();
    void onRenameSession();
    void onDuplicateSession();
    void onDeleteSession();
    void onSave();
    void onPush();
    void onQueue();
    void onProductionToggled(bool on);
    void onMyStationsReceived(const QJsonArray& stations);
    void onNowPlayingAccepted(const QString& stationId, const QString& spinId);

private:
    QWidget* buildTabIdentity();
    QWidget* buildTabContent();
    QWidget* buildTabEngagement();
    QWidget* buildTabTechnical();

    ApiClient*   m_api            = nullptr;
    QTabBar*     m_sessionBar     = nullptr;
    QComboBox*   m_serverCombo    = nullptr;
    QComboBox*   m_mountCombo     = nullptr;
    QComboBox*   m_stationCombo   = nullptr;
    QTabWidget*  m_fieldTabs      = nullptr;
    QPushButton* m_btnSave        = nullptr;
    QPushButton* m_btnPush        = nullptr;
    QPushButton* m_btnQueue       = nullptr;
    QCheckBox*   m_chkProduction  = nullptr;
    QLabel*      m_statusLabel    = nullptr;

    // Identity tab — Station / Show / DJ
    QLineEdit*   m_stationName    = nullptr;
    QLineEdit*   m_stationGenre   = nullptr;
    QLineEdit*   m_stationUrl     = nullptr;
    QLineEdit*   m_showTitle      = nullptr;
    QLineEdit*   m_showHosts      = nullptr;
    QLineEdit*   m_djName         = nullptr;
    QLineEdit*   m_djHandle       = nullptr;

    // Content tab — Track / Podcast
    QLineEdit*   m_trackTitle     = nullptr;
    QLineEdit*   m_trackArtist    = nullptr;
    QLineEdit*   m_trackAlbum     = nullptr;
    QLineEdit*   m_trackIsrc      = nullptr;
    QLineEdit*   m_trackMbid      = nullptr;
    QLineEdit*   m_trackLabel     = nullptr;
    QLineEdit*   m_podcastEpisode = nullptr;
    QLineEdit*   m_podcastSeason  = nullptr;

    // Engagement tab — Social / Engagement / Broadcast
    QLineEdit*   m_socX           = nullptr;
    QLineEdit*   m_socIg          = nullptr;
    QLineEdit*   m_socFb          = nullptr;
    QLineEdit*   m_socYouTube     = nullptr;
    QLineEdit*   m_callToAction   = nullptr;
    QLineEdit*   m_broadcastBucket= nullptr;

    // Technical tab — Notices / Audio / Flags
    QPlainTextEdit* m_notices     = nullptr;
    QLineEdit*   m_audioBitrate   = nullptr;
    QLineEdit*   m_audioCodec     = nullptr;
    QCheckBox*   m_flagExplicit   = nullptr;
    QCheckBox*   m_flagLive       = nullptr;
    QCheckBox*   m_flagReplay     = nullptr;
};
